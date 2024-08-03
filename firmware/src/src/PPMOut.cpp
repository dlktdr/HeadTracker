/*
 * This file is part of the Head Tracker distribution (https://github.com/dlktdr/headtracker)
 * Copyright (c) 2021 Cliff Blackburn
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdint.h>

#include "defines.h"
#include "io.h"
#include "serial.h"
#include "trackersettings.h"

LOG_MODULE_REGISTER(ppmout);

#if defined(CONFIG_SOC_SERIES_NRF52X) && defined(HAS_PPMOUT)
#include <nrfx_gpiote.h>
#include <nrfx_ppi.h>

#define PPMOUT_PPICH_MSK CONCAT(CONCAT(PPI_CHENSET_CH, PPMOUT_PPICH), _Msk)
#define PPMOUT_TIMER CONCAT(NRF_TIMER, PPMOUT_TIMER_CH)
#define PPMOUT_TIMER_IRQNO CONCAT(CONCAT(TIMER, PPMOUT_TIMER_CH), _IRQn)
#define PPMOUT_TMRCOMP_CH_MSK CONCAT(CONCAT(TIMER_INTENSET_COMPARE, PPMOUT_TMRCOMP_CH), _Msk)

volatile bool interrupt = false;

static volatile bool ppmoutstarted = false;
static volatile bool ppmoutinverted = false;

// Used in ISR
// Used to read data at once, read with isr disabled
static uint16_t ch_values[16];
static int ch_count;

static uint16_t framesync = TrackerSettings::PPM_MIN_FRAMESYNC;  // Minimum Frame Sync Pulse
static int32_t framelength;     // Ideal frame length
static uint16_t sync;            // Sync Pulse Length

// Local data - Only build with interrupts disabled
static uint32_t chsteps[35]{framesync, sync};

// ISR Values - Values from chsteps are copied here on step 0.
// prevent an update from happening mid stream.
static uint32_t isrchsteps[35]{framesync, sync};
static uint16_t chstepcnt = 1;
static uint16_t curstep = 0;
volatile bool buildingdata = false;

/* Builds an array with all the transition times
 */
void buildChannels()
{
  buildingdata = true;  // Prevent a read happing while this is building

  // Set user defined channel count, frame len, sync pulse
  ch_count = trkset.getPpmChCnt();
  sync = trkset.getPpmSync();
  framelength = trkset.getPpmFrame();

  int ch = 0;
  int i;
  uint32_t curtime = framesync;
  chsteps[0] = curtime;
  for (i = 1; i < ch_count * 2 + 1; i += 2) {
    curtime += sync;
    chsteps[i] = curtime;
    curtime += (ch_values[ch++] - sync);
    chsteps[i + 1] = curtime;
  }
  // Add Final Sync
  curtime += sync;
  chsteps[i++] = curtime;
  chstepcnt = i;
  // Now we know how long the train is. Try to make the entire frame == framelength
  // If possible it will add this to the frame sync pulse
  int ft = framelength - curtime;
  if (ft < 0)  // Not possible, no time left
    ft = 0;
  chsteps[i] = ft;  // Store at end of sequence
  buildingdata = false;
}

void resetChannels()
{
  // Set all channels to center
  for (int i = 0; i < 16; i++) ch_values[i] = 1500;
}

ISR_DIRECT_DECLARE(PPMTimerISR)
{
  ISR_DIRECT_HEADER();
  if (PPMOUT_TIMER->EVENTS_COMPARE[PPMOUT_TMRCOMP_CH] == 1) {
    // Clear event
    PPMOUT_TIMER->EVENTS_COMPARE[PPMOUT_TMRCOMP_CH] = 0;

    // Reset, don't get stuck in the wrong signal level
    if (curstep == 0) {
      if (ppmoutinverted)
        NRF_GPIOTE->TASKS_SET[PPMOUT_GPIOTE] = 1;
      else
        NRF_GPIOTE->TASKS_CLR[PPMOUT_GPIOTE] = 1;
    }

    curstep++;
    // Loop
    if (curstep >= chstepcnt) {
      if (!buildingdata) memcpy(isrchsteps, chsteps, sizeof(uint32_t) * 35);
      PPMOUT_TIMER->TASKS_CLEAR = 1;
      curstep = 0;
    }

    // Setup next capture event value
    PPMOUT_TIMER->CC[PPMOUT_TMRCOMP_CH] =
        isrchsteps[curstep] +
        isrchsteps[chstepcnt];  // Offset by the extra time required to make frame length right
  }
  ISR_DIRECT_FOOTER(1);
  return 0;
}

// Set pin to -1 to disable

void PpmOut_startStop(bool stop)
{
  int pin = PIN_TO_GPIN(PIN_NAME_TO_NUM(IO_PPMOUT));
  int port = PIN_TO_GPORT(PIN_NAME_TO_NUM(IO_PPMOUT));

  if (!ppmoutstarted) {
    resetChannels();
    buildChannels();
  }

  // Start by disabling

  // Stop Interrupts
  uint32_t key = irq_lock();

  // Disable Timer 3 Interrupt
  irq_disable(PPMOUT_TIMER_IRQNO);

  // Stop Timer
  PPMOUT_TIMER->TASKS_STOP = 1;

  // Stop Interrupt on peripherial
  PPMOUT_TIMER->INTENCLR = PPMOUT_TMRCOMP_CH_MSK;

  // Clear interrupt flag
  PPMOUT_TIMER->EVENTS_COMPARE[PPMOUT_TMRCOMP_CH] = 0;
  NRF_GPIOTE->CONFIG[PPMOUT_GPIOTE] = 0;  // Disable Config

  // Disable PPI
  NRF_PPI->CHENCLR = PPMOUT_PPICH_MSK;

  // If we want to enable it....
  if (stop == false) {
    // Setup GPOITE[7] to toggle output on every timer capture
    NRF_GPIOTE->CONFIG[PPMOUT_GPIOTE] =
        (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
        (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos) |
        (pin << GPIOTE_CONFIG_PSEL_Pos) | (port << GPIOTE_CONFIG_PORT_Pos);

    // High Drive PPM Output Pin
    if (port == 0)
      NRF_P0->PIN_CNF[pin] = (NRF_P0->PIN_CNF[pin] & ~GPIO_PIN_CNF_DRIVE_Msk) |
                             GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos;
    else if (port == 1)
      NRF_P1->PIN_CNF[pin] = (NRF_P1->PIN_CNF[pin] & ~GPIO_PIN_CNF_DRIVE_Msk) |
                             GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos;

    // Start Timers, they can stay running all the time.
    PPMOUT_TIMER->PRESCALER = 4;  // 16Mhz/2^(4) = 1Mhz = 1us Resolution, 1.048s Max@32bit
    PPMOUT_TIMER->MODE = TIMER_MODE_MODE_Timer << TIMER_MODE_MODE_Pos;
    PPMOUT_TIMER->BITMODE = TIMER_BITMODE_BITMODE_16Bit << TIMER_BITMODE_BITMODE_Pos;

    // On Compare equals Value, Toggle IO Pin
    NRF_PPI->CH[PPMOUT_PPICH].EEP = (uint32_t)&PPMOUT_TIMER->EVENTS_COMPARE[PPMOUT_TMRCOMP_CH];
    NRF_PPI->CH[PPMOUT_PPICH].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[PPMOUT_GPIOTE];

    // Enable PPI
    NRF_PPI->CHENSET = PPMOUT_PPICH_MSK;

    // Start
    IRQ_DIRECT_CONNECT(PPMOUT_TIMER_IRQNO, 0, PPMTimerISR, IRQ_ZERO_LATENCY);

    // Start timer
    memcpy(isrchsteps, chsteps, sizeof(uint32_t) * 32);
    PPMOUT_TIMER->CC[PPMOUT_TMRCOMP_CH] = framesync;
    curstep = 0;
    PPMOUT_TIMER->TASKS_CLEAR = 1;
    PPMOUT_TIMER->TASKS_START = 1;

    irq_enable(PPMOUT_TIMER_IRQNO);

    ppmoutstarted = true;

    // Enable timer interrupt
    PPMOUT_TIMER->EVENTS_COMPARE[PPMOUT_TMRCOMP_CH] = 0;
    PPMOUT_TIMER->INTENSET = PPMOUT_TMRCOMP_CH_MSK;
  }

  irq_unlock(key);
}

void PpmOut_execute()
{
  if(trkset.getPpmOutInvert() != ppmoutinverted) {
    ppmoutinverted = trkset.getPpmOutInvert();
  }
}

void PpmOut_setChannel(int chan, uint16_t val)
{
  if (chan >= 0 && chan <= ch_count && val >= TrackerSettings::MIN_PWM &&
      val <= TrackerSettings::MAX_PWM) {
    ch_values[chan] = val;
  }
  buildChannels();
}

int PpmOut_getChnCount() { return ch_count; }
int PpmOut_init()
{
  LOG_INF("Using pin %s", StrPinDescriptions[IO_PPMOUT]);
  PpmOut_startStop();
  return 0;
 }

#elif DT_NODE_EXISTS(DT_NODELABEL(ppm_output))
#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#warning "PPMOut using DT_NODELABEL(ppm_output) pin"

#define DELAY 2000000
#define ALARM_CHANNEL_ID 0

struct counter_alarm_cfg alarm_cfg;

#if defined(CONFIG_BOARD_SAMD20_XPRO)
#define TIMER DT_NODELABEL(tc4)
#elif defined(CONFIG_SOC_FAMILY_ATMEL_SAM)
#define TIMER DT_NODELABEL(tc0)
#elif defined(CONFIG_COUNTER_MICROCHIP_MCP7940N)
#define TIMER DT_NODELABEL(extrtc0)
#elif defined(CONFIG_COUNTER_NRF_RTC)
#define TIMER DT_NODELABEL(rtc0)
#elif defined(CONFIG_COUNTER_TIMER_STM32)
#define TIMER DT_INST(0, st_stm32_counter)
#elif defined(CONFIG_COUNTER_RTC_STM32)
#define TIMER DT_INST(0, st_stm32_rtc)
#elif defined(CONFIG_COUNTER_SMARTBOND_TIMER)
#define TIMER DT_NODELABEL(timer3)
#elif defined(CONFIG_COUNTER_NATIVE_POSIX)
#define TIMER DT_NODELABEL(counter0)
#elif defined(CONFIG_COUNTER_XLNX_AXI_TIMER)
#define TIMER DT_INST(0, xlnx_xps_timer_1_00_a)
#elif defined(CONFIG_COUNTER_TMR_ESP32)
#define TIMER DT_NODELABEL(timer0)
#elif defined(CONFIG_COUNTER_MCUX_CTIMER)
#define TIMER DT_NODELABEL(ctimer0)
#elif defined(CONFIG_COUNTER_NXP_S32_SYS_TIMER)
#define TIMER DT_NODELABEL(stm0)
#elif defined(CONFIG_COUNTER_TIMER_GD32)
#define TIMER DT_NODELABEL(timer0)
#elif defined(CONFIG_COUNTER_GECKO_RTCC)
#define TIMER DT_NODELABEL(rtcc0)
#elif defined(CONFIG_COUNTER_GECKO_STIMER)
#define TIMER DT_NODELABEL(stimer0)
#elif defined(CONFIG_COUNTER_INFINEON_CAT1)
#define TIMER DT_NODELABEL(counter0_0)
#elif defined(CONFIG_COUNTER_AMBIQ)
#define TIMER DT_NODELABEL(counter0)
#elif defined(CONFIG_COUNTER_SNPS_DW)
#define TIMER DT_NODELABEL(timer0)
#elif defined(CONFIG_COUNTER_TIMER_RPI_PICO)
#define TIMER DT_NODELABEL(timer)
#endif

int32_t jitter=0;
int32_t avgjitter=0;
uint32_t lastticks=0;
uint32_t maxjitter=0;
uint32_t difftick=0;

static void test_counter_interrupt_fn(const struct device *counter_dev,
				      uint8_t chan_id, uint32_t ticks,
				      void *user_data)
{
	struct counter_alarm_cfg *config = (counter_alarm_cfg *)user_data;
	uint32_t now_ticks;
	int err;

	err = counter_get_value(counter_dev, &now_ticks);
	if (err) {
		printk("Failed to read counter value (err %d)", err);
		return;
	}

  difftick = now_ticks - lastticks;
  lastticks = now_ticks;
  jitter = (int32_t)difftick - (int32_t)config->ticks;

	if (abs(jitter) > maxjitter) {
		maxjitter = abs(jitter);
	}

  if(maxjitter > 10000) {
    printk("Jitter high, %u\n", maxjitter);
    maxjitter = 0;
  }

  if(avgjitter == 0) {
    avgjitter = jitter;
  } else {
    avgjitter = (avgjitter + jitter) / 2;
  }

	printk("Diff: %u: Jitter %u: Avg %u: Max %u\n", (uint32_t)counter_ticks_to_us(counter_dev, difftick),
                                          (uint32_t)counter_ticks_to_us(counter_dev, jitter),
                                          (uint32_t)counter_ticks_to_us(counter_dev, avgjitter),
                                          (uint32_t)counter_ticks_to_us(counter_dev, maxjitter));

	config->ticks = config->ticks;

	printk("Set alarm in %u sec (%u ticks)\n",
	       (uint32_t)(counter_ticks_to_us(counter_dev,
					   config->ticks) / USEC_PER_SEC),
	       config->ticks);

	err = counter_set_channel_alarm(counter_dev, ALARM_CHANNEL_ID,
					(const counter_alarm_cfg*)user_data);
	if (err != 0) {
		printk("Alarm could not be set\n");
	}
}

int PpmOut_init()
{
	const struct device *const counter_dev = DEVICE_DT_GET(TIMER);
	int err;

	LOG_INF("Counter alarm sample");

	if (!device_is_ready(counter_dev)) {
		LOG_ERR("device not ready");
		return -1;
	}

  counter_start(counter_dev);

	alarm_cfg.flags = 0;
	alarm_cfg.ticks = counter_us_to_ticks(counter_dev, DELAY);
	alarm_cfg.callback = test_counter_interrupt_fn;
	alarm_cfg.user_data = &alarm_cfg;

  err = counter_set_channel_alarm(counter_dev, ALARM_CHANNEL_ID,
				&alarm_cfg);
	printk("Set alarm in %u sec (%u ticks)\n",
	       (uint32_t)(counter_ticks_to_us(counter_dev,
					   alarm_cfg.ticks) / USEC_PER_SEC),
	       alarm_cfg.ticks);

  return 0;
}

void PpmOut_setPin(int pinNum) {}
void PpmOut_setChannel(int chan, uint16_t val) {}
void PpmOut_execute() {}
int PpmOut_getChnCount() {return 0;}

#else

int PpmOut_init() {return -1;}
void PpmOut_setPin(int pinNum) {}
void PpmOut_setChannel(int chan, uint16_t val) {}
void PpmOut_execute() {}
int PpmOut_getChnCount() {return 0;}

#endif
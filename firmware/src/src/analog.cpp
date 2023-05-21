#include "analog.h"

#include <drivers/adc.h>
#include <string.h>
#include <zephyr.h>

#include "log.h"
#include "serial.h"

// Simple analog input method
// this just reads a sample then waits then returns it

// ADC Sampling Settings
// doc says that impedance of 800K == 40usec sample time

#define ADC_RESOLUTION 10
#define ADC_GAIN ADC_GAIN_1_6
#define ADC_REFERENCE ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40)
#define BUFFER_SIZE 6

static bool _IsInitialized = false;
static uint8_t _LastChannel = 250;
static int16_t m_sample_buffer[BUFFER_SIZE];

// the channel configuration with channel not yet filled in
static struct adc_channel_cfg m_1st_channel_cfg = {
    .gain = ADC_GAIN,
    .reference = ADC_REFERENCE,
    .acquisition_time = ADC_ACQUISITION_TIME,
    .channel_id = 0,  // gets set during init
    .differential = 0,
#if CONFIG_ADC_CONFIGURABLE_INPUTS
    .input_positive = 0,  // gets set during init
#endif
};

// initialize the adc channel
static const struct device *init_adc(int channel)
{
  int ret;
  const struct device *adc_dev = DEVICE_DT_GET(DT_ALIAS(adcctrl));
  if (!adc_dev) {
    LOGE("Could not get device binding for ADC");
    return 0;
  }
  if (_LastChannel != channel) {
    _IsInitialized = false;
    _LastChannel = channel;
  }

  if (adc_dev != NULL && !_IsInitialized) {
    // strangely channel_id gets the channel id and input_positive gets id+1
    m_1st_channel_cfg.channel_id = channel;
#if CONFIG_ADC_CONFIGURABLE_INPUTS
    m_1st_channel_cfg.input_positive = channel + 1;
#endif

    ret = adc_channel_setup(adc_dev, &m_1st_channel_cfg);

    if (ret != 0) {
      adc_dev = NULL;
    } else {
      _IsInitialized = true;  // we don't have any other analog users
    }
  }

  memset(m_sample_buffer, 0, sizeof(m_sample_buffer));
  return adc_dev;
}

// ------------------------------------------------
// read one channel of adc
// ------------------------------------------------
static int16_t readOneChannel(int channel)
{
  const struct adc_sequence sequence = {
      .options = NULL,            // extra samples and callback
      .channels = BIT(channel),   // bit mask of channels to read
      .buffer = m_sample_buffer,  // where to put samples read
      .buffer_size = sizeof(m_sample_buffer),
      .resolution = ADC_RESOLUTION,  // desired resolution
      .oversampling = 0,             // don't oversample
      .calibrate = 0                 // don't calibrate
  };

  int ret;
  int16_t sample_value = BAD_ANALOG_READ;
  const struct device *adc_dev = init_adc(channel);
  if (adc_dev) {
    ret = adc_read(adc_dev, &sequence);
    if (ret == 0) {
      sample_value = m_sample_buffer[0];
    }
  }

  return sample_value;
}

// ------------------------------------------------
// high level read adc channel and convert to float voltage
// ------------------------------------------------
float analogRead(int channel)
{
  int16_t sv = readOneChannel(channel);
  if (sv == BAD_ANALOG_READ) {
    return sv;
  }
  return (float)sv / 287.0;
}

#ifdef CONFIG_LOG_BACKEND_HTGUI

#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/logging/log_backend_std.h>
#include <zephyr/logging/log_core.h>
#include <serial.h>
#include "ucrc16lib.h"

#define _STDOUT_BUF_SIZE 256
#define _JSONOUT_BUF_SIZE _STDOUT_BUF_SIZE + 26
static char stdout_buff[_STDOUT_BUF_SIZE];
static char outbuf[_JSONOUT_BUF_SIZE];
static int n_pend = 0;
static bool isready = true;

static uint32_t log_format_current = CONFIG_LOG_BACKEND_HTGUI_OUTPUT_DEFAULT;

// Log message starts with 0x01 (start of heading), end with 0x03 (end of text)
// if either of these characters are in the message, they are escaped out with 0x1B

int logescape(const char *inbuf, char *outbuf, uint32_t len, uint32_t maxlen)
{
  uint32_t inpos = 0;
  uint32_t outpos = 0;
  while (inpos < len) {
    if( outpos >= maxlen ) {
      break;
    }
    if (inbuf[inpos] == 0x01 || inbuf[inpos] == 0x03 || inbuf[inpos] == 0x1B) {
      outbuf[outpos+1]= inbuf[inpos] ^ 0xFF;
      outbuf[outpos] = 0x1B;
      outpos++;
    } else {
      outbuf[outpos] = inbuf[inpos];
    }

    inpos++;
    outpos++;
  }
  return outpos+1;
}

uint16_t logescapeCRC(uint16_t crc)
{
  // Characters to escape out
  uint8_t crclow = crc & 0xFF;
  uint8_t crchigh = (crc >> 8) & 0xFF;
  if (crclow == 0x00 || crchigh == 0x01 || crclow == 0x02 || crclow == 0x03 || crclow == 0x06 || crclow == 0x15)
    crclow ^= 0xFF;  //?? why not..
  if (crchigh == 0x00 || crchigh == 0x01 || crchigh == 0x02 || crchigh == 0x03 || crchigh == 0x06 || crchigh == 0x15)
    crchigh ^= 0xFF;  //?? why not..
  return (uint16_t)crclow | ((uint16_t)crchigh << 8);
}

static void preprint_char(int c)
{
	int printnow = 0;

	if (c == '\r') {
		/* Discard carriage returns */
		return;
	}
	if (c != '\n') {
		stdout_buff[n_pend++] = c;
		stdout_buff[n_pend] = 0;
	} else {
		printnow = 1;
	}

	if (n_pend >= _STDOUT_BUF_SIZE - 1) {
		printnow = 1;
	}

	if (printnow) {
    if( isready == true ) {
      outbuf[0] = 0x01; // Start of Heading
      uint32_t br = logescape(stdout_buff, outbuf+1, n_pend, _JSONOUT_BUF_SIZE-4);
      outbuf[br] = 0x03; // End of Text
      outbuf[br+1] = '\r';
      outbuf[br+2] = '\n';
      serialWrite(outbuf, br+3);
    }
		n_pend = 0;
		stdout_buff[0] = 0;
	}
}

static uint8_t buf[_STDOUT_BUF_SIZE];

static int char_out(uint8_t *data, size_t length, void *ctx)
{
	for (size_t i = 0; i < length; i++) {
		preprint_char(data[i]);
	}

	return length;
}


LOG_OUTPUT_DEFINE(log_output_htgui, char_out, buf, sizeof(buf));

void process(const struct log_backend *const backend, union log_msg_generic *msg)
{
	uint32_t flags = log_backend_std_get_flags();

	log_format_func_t log_output_func = log_format_func_t_get(log_format_current);

	log_output_func(&log_output_htgui, &msg->log, flags);
}

void dropped(const struct log_backend *const backend, uint32_t cnt)
{

}

void panic(const struct log_backend *const backend)
{

}

void init(const struct log_backend *const backend)
{

}

int is_ready(const struct log_backend *const backend)
{
  //k_poll_signal_check(&serialThreadRunSignal, &signaled, &result);
  return  0;
}

int format_set(const struct log_backend *const backend, uint32_t log_type)
{
  log_format_current = log_type;
  return 0;
}

void notify(const struct log_backend *const backend, enum log_backend_evt event, union log_backend_evt_arg *arg)
{

}

const struct log_backend_api lbe = {
  .process = process,
  .dropped = NULL,
  .panic = panic,
  .init = init,
  .is_ready = is_ready,
  .format_set = format_set,
  .notify = NULL
  };

LOG_BACKEND_DEFINE(htlogbe, lbe, true);

#endif
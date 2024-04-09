#include <zephyr/logging/log_backend.h>

void process(const struct log_backend *const backend, union log_msg_generic *msg)
{

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
  return 0;
}

int format_set(const struct log_backend *const backend, uint32_t log_type)
{

}

void notify(const struct log_backend *const backend, enum log_backend_evt event, union log_backend_evt_arg *arg)
{

}

const struct log_backend_api lbe = {
  .process = process,
  .dropped = dropped,
  .panic = panic,
  .init = init,
  .is_ready = is_ready,
  .format_set = format_set,
  .notify = notify};

LOG_BACKEND_DEFINE(htlogbe, lbe, true);
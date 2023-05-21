// Undefine a conflicting name with Arduino JSON and Zephyr
#undef _current
#define ARDUINOJSON_USE_DOUBLE 0
#include "ArduinoJson.h"

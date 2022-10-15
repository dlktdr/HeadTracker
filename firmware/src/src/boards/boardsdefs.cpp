#include "../defines.h"

const char* StrPins[] = {
#define PIN(NAME, _PIN, DESC) #NAME,
  PIN_X
#undef PIN
};


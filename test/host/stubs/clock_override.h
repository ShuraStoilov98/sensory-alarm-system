#pragma once

// Force-include after the real time declarations, then redirect only firmware
// clock reads. Production sources and libc timezone conversion are unchanged.
#include "Arduino.h"
#define time firmwareTestTime

// flashstr.h

#ifndef _FLASHSTR_h
#define _FLASHSTR_h

#pragma once

#include "Arduino.h"

#ifndef pgmptr
typedef const __FlashStringHelper* pgmptr;
#endif
#ifndef PGMPTR
#define PGMPTR( pgm_ptr ) ( reinterpret_cast< pgmptr >( pgm_ptr ) )
#endif

//#define PGMCHAR const PROGMEM char

#endif

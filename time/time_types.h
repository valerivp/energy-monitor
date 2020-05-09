#pragma once

#include <stdint.h>
#include <stdlib.h>
/*#ifndef uint16_t
typedef unsigned short uint16_t;
#endif // !uint16_t

#ifndef uint8_t
typedef unsigned char uint8_t;
#endif // !uint8_t*/

#ifndef week_date
struct week_date {
	int year; /**< year number (Gregorian calendar) */
	int week; /**< week number (#1 is where first Thursday is in) */
	int day; /**< day within week */
};
#endif // !week_date


/*
typedef struct {
	int quot;            
	int rem;               
} div_t;*/

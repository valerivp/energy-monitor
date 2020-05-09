#include "time_types.h"

div_t div(int __numer, int __denom) {
	static div_t d;
	d.quot = __numer / __denom;
	d.rem = __numer % __denom;
	return d;
}

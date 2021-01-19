#include "bit_check_band.h"

void bitcheckint(int &value2check, int minbit, int maxbit)
{
    if (value2check < minbit)
        value2check = minbit;
    if (value2check > maxbit)
        value2check = maxbit;
}

void bitcheckfloat(float &value2check, float minbit, float maxbit)
{
    if (value2check < minbit)
        value2check = minbit;
    if (value2check > maxbit)
        value2check = maxbit;
}
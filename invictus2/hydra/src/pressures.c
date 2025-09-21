#include "pressures.h"

/// @brief Convert volts to bar
/// @param v Volts value
/// @param min_bar Minimum bar value
/// @param max_bar Maximum bar value
/// @return Pressure in bar
int mv_to_mbar(int mv, int32_t min_bar, int32_t max_bar)
{
   return (((mv * max_bar) / 10) + min_bar);
}

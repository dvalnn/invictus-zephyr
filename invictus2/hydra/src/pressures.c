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

int pressures_init(void)
{
    // Initialization code for pressure sensors would go here
    return 0;
}

void pressures_start(void)
{
   // Start reading pressure sensors and publishing to ZBUS
}


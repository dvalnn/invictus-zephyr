#ifndef PRESSURES_H
#define PRESSURES_H

#include <stdint.h>

#define N2_PRESSURE_MIN_BAR 0
#define N2_PRESSURE_MAX_BAR 250

#define N2O_PRESSURE_MIN_BAR 0
#define N2O_PRESSURE_MAX_BAR 100

int mv_to_mbar(int32_t mv, int32_t min_bar, int32_t max_bar);

#endif // PRESSURES_H
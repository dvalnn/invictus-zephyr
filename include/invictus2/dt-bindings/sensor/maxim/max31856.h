#ifndef INVICTUS2_DT_BINDINGS_MAX31856_H_
#define INVICTUS2_DT_BINDINGS_MAX31856_H_

/** Multiple types of thermocouples supported */
#define MAX31856_TCTYPE_B  0x0 // 0b0000
#define MAX31856_TCTYPE_E  0x1 // 0b0001
#define MAX31856_TCTYPE_J  0x2 // 0b0010
#define MAX31856_TCTYPE_K  0x3 // 0b0011
#define MAX31856_TCTYPE_N  0x4 // 0b0100
#define MAX31856_TCTYPE_R  0x5 // 0b0101
#define MAX31856_TCTYPE_S  0x6 // 0b0110
#define MAX31856_TCTYPE_T  0x7 // 0b0111
#define MAX31856_VMODE_G8  0x8 // 0b1000
#define MAX31856_VMODE_G32 0xb // 0b1100

/** Temperature averaging mode */
#define MAX31856_AVG_MODE_1  0x0
#define MAX31856_AVG_MODE_2  0x1
#define MAX31856_AVG_MODE_4  0x2
#define MAX31856_AVG_MODE_8  0x3
#define MAX31856_AVG_MODE_16 0x4

/** Open circuit fault detection mode */
#define MAX31856_OCFAULT_DET_DISABLED    0x0
// Once every 16 conversions, Rs < 5k Ohm
#define MAX31856_OCFAULT_DET_RS5k        0x1
// Once every 16 conversions, 5k < Rs < 40k, with time constant < 2ms
#define MAX31856_OCFAULT_DET_RS40k_LT2MS 0x2
// Once every 16 conversions, 5k < Rs < 40k, with time constant > 2ms
#define MAX31856_OCFAULT_DET_RS40k_GT2MS 0x3

#endif // INVICTUS2_DT_BINDINGS_MAX31856_H_

#ifndef MAX31856_H_
#define MAX31856_H_

#include <zephyr/types.h>
#include <zephyr/drivers/spi.h>

// Register 00h (Configuration 0):
// 0x00
// | (0 << 7) -> Continuous conversion mode (CMODE=0)
// | (0 << 6) -> No fault clear (FCLR=0)
// | (0 << 5) -> Open circuit detection disabled (OCDET=0)
// | (0 << 4) -> Internal Cold-Junction (CJ=0)
// | (0 << 3) -> Average mode disabled (AVG=0)
// | (0 << 2) -> Fault interrupt enabled (FAULT_INT=0)
// | (0 << 1) -> No bias voltage (BIAS=0)
// | (0 << 0) -> 60Hz filtering (50HZ=0)

#define MAX31856_CR0_REG         0x00 //< Config 0 register
#define MAX31856_CR0_AUTOCONVERT 7    //< Config 0 Auto convert flag
#define MAX31856_CR0_1SHOT       6    //< Config 0 one shot convert flag
#define MAX31856_CR0_OCFAULT     4    //< Config 0 open circuit fault 0 flag
#define MAX31856_CR0_CJ          3    //< Config 0 cold junction disable flag
#define MAX31856_CR0_FAULT       2    //< Config 0 fault mode flag
#define MAX31856_CR0_FAULTCLR    1    //< Config 0 fault clear flag
#define MAX31856_CR0_50HZ        0    //< Config 0 50Hz filter flag

// Register 01h (Configuration 1):
// 0x01
// | (0 << 7)   -> No fault checking (FAULT_CHECK=0)
// | (0 << 6-4) -> No averaging (AVG=0)
// | (0 << 3)   -> No VMODE
// | (thermocouple_type & 0x07) -> Set TC type
#define MAX31856_CR1_REG     0x01 //< Config 1 register
#define MAX31856_CR1_AVG_SEL 4    //< Config 1 fault check flag
#define MAX31856_CR1_TCTYPE  0    //< Config 1 VMODE flag

#define MAX31856_MASK_REG 0x02 //< Fault Mask register

#define MAX31856_CJHF_REG 0x03 //< Cold junction High temp fault register
#define MAX31856_CJLF_REG 0x04 //< Cold junction Low temp fault register

#define MAX31856_LTHFTH_REG 0x05 //< Linearized Temperature High Fault Threshold Register, MSB
#define MAX31856_LTHFTL_REG 0x06 //< Linearized Temperature High Fault Threshold Register, LSB

#define MAX31856_LTLFTH_REG 0x07 //< Linearized Temperature Low Fault Threshold Register, MSB
#define MAX31856_LTLFTL_REG 0x08 //< Linearized Temperature Low Fault Threshold Register, LSB

#define MAX31856_CJTO_REG 0x09 //< Cold-Junction Temperature Offset Register
#define MAX31856_CJTH_REG 0x0A //< Cold-Junction Temperature Register, MSB
#define MAX31856_CJTL_REG 0x0B //< Cold-Junction Temperature Register, LSB

#define MAX31856_LTCBH_REG 0x0C //< Linearized TC Temperature, Byte 2
#define MAX31856_LTCBM_REG 0x0D //< Linearized TC Temperature, Byte 1
#define MAX31856_LTCBL_REG 0x0E //< Linearized TC Temperature, Byte 0

#define MAX31856_SR_REG 0x0F //< Fault Status Register

#define MAX31856_FAULT_CJRANGE 0x80 //< Fault status Cold Junction Out-of-Range flag
#define MAX31856_FAULT_TCRANGE 0x40 //< Fault status Thermocouple Out-of-Range flag
#define MAX31856_FAULT_CJHIGH  0x20 //< Fault status Cold-Junction High Fault flag
#define MAX31856_FAULT_CJLOW   0x10 //< Fault status Cold-Junction Low Fault flag
#define MAX31856_FAULT_TCHIGH  0x08 //< Fault status Thermocouple Temperature High Fault flag
#define MAX31856_FAULT_TCLOW   0x04 //< Fault status Thermocouple Temperature Low Fault flag
#define MAX31856_FAULT_OVUV    0x02 //< Fault status Overvoltage or Undervoltage Input Fault flag
#define MAX31856_FAULT_OPEN    0x01 //< Fault status Thermocouple Open-Circuit Fault flag

// Structure for device configuration
struct max31856_config
{
    struct spi_dt_spec spi;
    struct gpio_dt_spec drdy_gpio;
    struct gpio_dt_spec fault_gpio;

    uint8_t thermocouple_type;
    uint8_t avg_mode;
    uint8_t oc_fault_mode;
    uint8_t cj_offset_raw;
    bool filter_50hz;

    bool has_drdy_gpio;
    bool has_fault_gpio;
};

struct max31856_data
{
    uint32_t thermocouple_temp;
    uint16_t cold_junction_temp;
};

#endif // MAX31856_H_

#ifndef _MAIN_SM_CONFIG_H_
#define _MAIN_SM_CONFIG_H_

#include <stdint.h>
#include "filling_sm_config.h"
#include "flight_sm_config.h"

struct sm_config {
    struct filling_sm_config filling_sm_config;
    struct flight_sm_config flight_sm_config;
};

#endif // _MAIN_SM_CONFIG_H_

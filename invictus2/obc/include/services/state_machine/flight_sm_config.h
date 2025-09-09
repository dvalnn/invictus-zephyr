#ifndef _FLIGHT_SM_CONFIG_H_
#define _FLIGHT_SM_CONFIG_H_

#include <stdint.h>

#define MIN_CHAMBER_LAUNCH_TEMP  650  // Minimum chamber temperature in Celsius for launch
#define MAIN_CHUTE_DEPLOY_ALTITUDE 450 // Altitude in meters for main chute deployment

struct flight_sm_config {
    uint16_t min_chamber_launch_temp; // Minimum chamber temperature in Celsius for launch
    uint16_t main_chute_deploy_altitude; // Altitude in meters for main chute deployment
};

#define DEFAULT_FLIGHT_SM_CONFIG(name)                                                              \
    struct flight_sm_config name = {                                                         \
        .min_chamber_launch_temp = MIN_CHAMBER_LAUNCH_TEMP,                                   \
        .main_chute_deploy_altitude = MAIN_CHUTE_DEPLOY_ALTITUDE,                             \
    }


#endif // _FLIGHT_SM_CONFIG_H_
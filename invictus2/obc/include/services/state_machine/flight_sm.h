#ifndef FLIGHT_SM_H
#define FLIGHT_SM_H

#include "data_models.h"
#include "main_sm.h"
#include "zephyr/smf.h"
#include <zephyr/logging/log.h>

void ignition_entry(void *o);
void ignition_run(void *o);
void ignition_exit(void *o);

void boost_entry(void *o);
void boost_run(void *o);
void boost_exit(void *o);

void coast_entry(void *o);
void coast_run(void *o);
void coast_exit(void *o);

void apogee_entry(void *o);
void apogee_run(void *o);
void apogee_exit(void *o);

void drogue_chute_entry(void *o);
void drogue_chute_run(void *o);
void drogue_chute_exit(void *o);

void main_chute_entry(void *o);
void main_chute_run(void *o);
void main_chute_exit(void *o);

void touchdown_entry(void *o);
void touchdown_run(void *o);
void touchdown_exit(void *o);

#endif // FLIGHT_SM_H
#ifndef FILLING_SM_H
#define FILLING_SM_H

#include "data_models.h"
#include "zephyr/smf.h"
#include <zephyr/logging/log.h>

void filling_copv_entry(void *o);
void filling_copv_run(void *o);
void filling_copv_exit(void *o);

void filling_copv_idle_entry(void *o);
void filling_copv_idle_run(void *o);
void filling_copv_idle_exit(void *o);

void filling_copv_fill_entry(void *o);
void filling_copv_fill_run(void *o);
void filling_copv_fill_exit(void *o);


void pre_press_entry(void *o);
void pre_press_run(void *o);
void pre_press_exit(void *o);

void pre_press_idle_entry(void *o);
void pre_press_idle_run(void *o);
void pre_press_idle_exit(void *o);

void pre_press_fill_entry(void *o);
void pre_press_fill_run(void *o);
void pre_press_fill_exit(void *o);

void pre_press_vent_entry(void *o);
void pre_press_vent_run(void *o);
void pre_press_vent_exit(void *o);


void filling_n2o_entry(void *o);
void filling_n2o_run(void *o);
void filling_n2o_exit(void *o);

void filling_n2o_idle_entry(void *o);
void filling_n2o_idle_run(void *o);
void filling_n2o_idle_exit(void *o);

void filling_n2o_fill_entry(void *o);
void filling_n2o_fill_run(void *o);
void filling_n2o_fill_exit(void *o);

void filling_n2o_vent_entry(void *o);
void filling_n2o_vent_run(void *o);
void filling_n2o_vent_exit(void *o);


void post_press_entry(void *o);
void post_press_run(void *o);
void post_press_exit(void *o);

void post_press_idle_entry(void *o);
void post_press_idle_run(void *o);
void post_press_idle_exit(void *o);

void post_press_fill_entry(void *o);
void post_press_fill_run(void *o);
void post_press_fill_exit(void *o);

void post_press_vent_entry(void *o);
void post_press_vent_run(void *o);
void post_press_vent_exit(void *o);

#endif // FILLING_SM_H
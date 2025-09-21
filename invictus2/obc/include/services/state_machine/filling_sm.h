#ifndef FILLING_SM_H
#define FILLING_SM_H

void safe_pause_entry(void *o);
void safe_pause_run(void *o);
void safe_pause_exit(void *o);

void safe_pause_idle_entry(void *o);
void safe_pause_idle_run(void *o);
void safe_pause_idle_exit(void *o);

void safe_pause_vent_entry(void *o);
void safe_pause_vent_run(void *o);
void safe_pause_vent_exit(void *o);

void fill_n2_entry(void *o);
void fill_n2_run(void *o);
void fill_n2_exit(void *o);

void fill_n2_idle_entry(void *o);
void fill_n2_idle_run(void *o);
void fill_n2_idle_exit(void *o);

void fill_n2_fill_entry(void *o);
void fill_n2_fill_run(void *o);
void fill_n2_fill_exit(void *o);

void fill_n2_vent_entry(void *o);
void fill_n2_vent_run(void *o);
void fill_n2_vent_exit(void *o);

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

void fill_n2o_entry(void *o);
void fill_n2o_run(void *o);
void fill_n2o_exit(void *o);

void fill_n2o_idle_entry(void *o);
void fill_n2o_idle_run(void *o);
void fill_n2o_idle_exit(void *o);

void fill_n2o_fill_entry(void *o);
void fill_n2o_fill_run(void *o);
void fill_n2o_fill_exit(void *o);

void fill_n2o_vent_entry(void *o);
void fill_n2o_vent_run(void *o);
void fill_n2o_vent_exit(void *o);

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

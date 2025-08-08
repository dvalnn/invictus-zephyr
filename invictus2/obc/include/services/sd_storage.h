#ifndef SD_STORAGE_H_
#define SD_STORAGE_H_

/* #include "zephyr/kernel.h" */

// TODO: Define these as KConfig Options
#ifndef SD_LISTENER_PRIO
#define SD_LISTENER_PRIO 2
#endif

#ifndef SD_WORKER_PRIO
#define SD_WORKER_PRIO 5
#endif

/* extern struct k_work_queue_config sd_worker_config; */

void sd_service_start(void);

#endif // SD_STORAGE_H_

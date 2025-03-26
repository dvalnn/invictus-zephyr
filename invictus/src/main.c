/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

int main(void) {
  while (1) {
    printk("Hello World! %s\n", CONFIG_BOARD_TARGET);
    k_timeout_t timeout = K_MSEC(1000);
    k_sleep(timeout);
  }

  return 0;
}

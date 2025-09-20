#include "zephyr/kernel.h"

int main()
{
    while (1) {
        printk("Hello, World!\n");
        k_sleep(K_MSEC(1000));
    }
}

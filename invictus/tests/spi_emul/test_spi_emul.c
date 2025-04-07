#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>

#define SPIOP SPI_WORD_SET(8) | SPI_TRANSFER_MSB
struct spi_dt_spec spispec = SPI_DT_SPEC_GET(DT_NODELABEL(fake_sensor), SPIOP, 0);

int main(void)
{

	int err = spi_is_ready_dt(&spispec);
	if (!err) {
		printk("Error: SPI device is not ready, err: %d", err);
		return 0;
	}

	return 0;
}

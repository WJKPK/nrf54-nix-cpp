#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

int main()
{
	LOG_INF("nRF54L15 C++ Application started");

	int counter = 0;

	while (true) {
		LOG_INF("Loop iteration: %d", counter);
		counter++;
		k_sleep(K_MSEC(1000));
	}

	return 0;
}

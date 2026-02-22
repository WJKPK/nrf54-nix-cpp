#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

int main()
{
	LOG_INF("nRF54L15 C++ Application started");

	while (true) {
		k_sleep(K_MSEC(1000));
	    LOG_INF("Loop...");
	}

	return 0;
}

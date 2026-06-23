#include <sys/lock.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "cmsis_os.h"

#include "logger.h"

static UART_HandleTypeDef *phuartx = NULL;
osMutexId_t printTX_mutex = NULL;

void __wrap___retarget_lock_acquire_recursive(_LOCK_T lock)
{
	if (osKernelGetState() == osKernelRunning) {
		osMutexAcquire(printTX_mutex, osWaitForever);
	}
}

void __wrap___retarget_lock_release_recursive(_LOCK_T lock)
{
	if (osKernelGetState() == osKernelRunning) {
		osMutexRelease(printTX_mutex);
	}
}

int retarget_init(UART_HandleTypeDef *huart, uart_init_t *init)
{
	int ret = -1;

	const osMutexAttr_t ThreadSafeMutex_attr = { "PrintfRecursiveMutex",
						     osMutexRecursive, NULL,
						     0 };

	do {
		if (huart == NULL)
			break;
		phuartx = huart;

		if (init)
			init();

		printTX_mutex = osMutexNew(&ThreadSafeMutex_attr);

		if (printTX_mutex == NULL)
			break;

		ret = 0;
	} while (0);

	return ret;
}

int _write(int file, char *ptr, int len)
{
	if (printTX_mutex)
		osMutexAcquire(printTX_mutex, portMAX_DELAY);

	if (phuartx)
		HAL_UART_Transmit(phuartx, (uint8_t *)ptr, len, HAL_MAX_DELAY);

	if (printTX_mutex)
		osMutexRelease(printTX_mutex);

	return len;
}

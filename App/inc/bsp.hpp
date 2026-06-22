#pragma once

#include <cstring>
#include "main.h"

#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "timers.h"
#include "cmsis_os2.h"

class Bsp {
    private:
	const StackType_t uxStackDepth;
	const UBaseType_t uxPriority;

    public:
	const char *name = nullptr;

	TaskHandle_t taskHandle = nullptr;

	virtual void hwInit(void) = 0;
	virtual void thread(void *arg) = 0;
	int32_t start(void);
	void readUniqueID(uint8_t *unique_id);

	Bsp(const char *mname, const StackType_t stack,
	    const UBaseType_t priority);
	~Bsp();
};

int32_t Bsp::start(void)
{
	system_init();
	hwInit();

	if (pdPASS != xTaskCreate(
			      [](void *arg) {
				      if (osKernelInitialize()) {
					      Error_Handler();
				      }
				      static_cast<Bsp *>(arg)->thread(arg);
			      },
			      name, uxStackDepth, this, uxPriority,
			      &taskHandle))
		Error_Handler();

	vTaskStartScheduler();

	return -1;
}

Bsp::Bsp(const char *mname, const StackType_t stack, const UBaseType_t priority)
	: uxStackDepth(stack)
	, uxPriority(priority)
	, name(mname)
{
}

Bsp::~Bsp()
{
}

void Bsp::readUniqueID(uint8_t *unique_id)
{
	uint32_t uid[3];
	uid[0] = HAL_GetUIDw0();
	uid[1] = HAL_GetUIDw1();
	uid[2] = HAL_GetUIDw2();

	std::memcpy(unique_id, uid, 12);

	std::memset(unique_id + 12, 0xAA, 4);
}

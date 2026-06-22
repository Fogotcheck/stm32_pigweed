#pragma once

#include "bsp.hpp"

class App : public Bsp {
    private:
	void hwInit(void);
	void thread(void *arg);
	void init(void);

    public:
	enum SERVICE_STACK_SIZE {
		S_MAIN_STACK = (configMINIMAL_STACK_SIZE << 4),

	};
	enum SERVICE_PRIORITY {
		S_MAIN_PRIO = osPriorityLow,
	};

	enum EVENTS {

		A_LIVE = (EventBits_t)(1 << 0),
		MOT_UPDATE = (EventBits_t)(1 << 1),
		SENSORS_SEND = (EventBits_t)(1 << 2),

		ALL_EVENTS = (EventBits_t)(A_LIVE | MOT_UPDATE | SENSORS_SEND)
	};

	App(const char *mname);
	~App();
};

App::App(const char *mname)
	: Bsp::Bsp(mname, S_MAIN_STACK, S_MAIN_PRIO)
{
}

App::~App()
{
}

void App::hwInit(void)
{
}

void App::thread(void *arg)
{
	while (1) {
	}
}

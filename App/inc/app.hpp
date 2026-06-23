#pragma once

#include <mutex>

#include "bsp.hpp"
#include "pw_alignment/alignment.h"
#include "pw_log/log.h"
#include <optional>
#include "logger.h"
#include "pw_thread/thread.h"
#include "pw_chrono/system_clock.h"
#include "pw_thread/sleep.h"
#include "pw_thread_freertos/context.h"
#include "pw_containers/intrusive_list.h"
#include "pw_sync/interrupt_spin_lock.h"
#include "pw_string/string.h"

struct LogMessage : public pw::IntrusiveList<LogMessage>::Item {
	pw::InlineString<48> payload;
	pw::chrono::SystemClock::time_point timestamp;
	uint32_t irq_counter;
};

class App : public Bsp {
    private:
	static constexpr size_t POOL_SIZE = 8;
	LogMessage pool_[POOL_SIZE];

	pw::IntrusiveList<LogMessage> ready_list_;
	pw::IntrusiveList<LogMessage> free_list_;

	pw::sync::InterruptSpinLock lock_;

	void hwInit(void);
	void thread(void *arg);
	void thread_2();
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

	LogMessage *allocate_from_isr()
	{
		std::lock_guard<pw::sync::InterruptSpinLock> lock(lock_);
		if (free_list_.empty()) {
			return nullptr;
		}
		LogMessage *item = &free_list_.front();
		free_list_.pop_front();
		return item;
	}

	void push_from_isr(LogMessage *item)
	{
		std::lock_guard<pw::sync::InterruptSpinLock> lock(lock_);
		ready_list_.push_back(*item);
	}

	std::optional<LogMessage *> pop_to_thread()
	{
		std::lock_guard<pw::sync::InterruptSpinLock> lock(lock_);
		if (ready_list_.empty()) {
			return std::nullopt;
		}
		LogMessage *item = &ready_list_.front();
		ready_list_.pop_front();
		return item;
	}

	void release_from_thread(LogMessage *item)
	{
		std::lock_guard<pw::sync::InterruptSpinLock> lock(lock_);
		free_list_.push_back(*item);
	}

	App(const char *mname);
	~App();
};

App::App(const char *mname)
	: Bsp::Bsp(mname, S_MAIN_STACK, S_MAIN_PRIO)
{
	for (size_t i = 0; i < POOL_SIZE; ++i) {
		free_list_.push_back(pool_[i]);
	}
}

App::~App()
{
}

void App::hwInit(void)
{
	extern UART_HandleTypeDef huart3;
	if (retarget_init(&huart3, MX_USART3_UART_Init))
		Error_Handler();
}

void App::thread(void *arg)
{
	pw::thread::freertos::Options options;
	options.set_name("LogConsumerTask");
	options.set_priority(S_MAIN_PRIO);
	options.set_stack_size(S_MAIN_STACK);

	pw::Thread my_new_thread(options, [this]() { this->thread_2(); });
	my_new_thread.detach();

	uint32_t evt_cnt = 0;

	while (1) {
		PW_LOG_INFO("--- Thread 1: ---");
		evt_cnt++;

		LogMessage *msg = this->allocate_from_isr();

		if (msg != nullptr) {
			msg->payload.clear();
			msg->payload.append("Hardware Interrupt Event!");
			msg->timestamp = pw::chrono::SystemClock::now();
			msg->irq_counter = evt_cnt;

			this->push_from_isr(msg);
		} else {
			PW_LOG_WARN("Pool is fool");
		}

		pw::this_thread::sleep_for(std::chrono::milliseconds(1500));
	}
}

void App::thread_2()
{
	while (1) {
		auto res = this->pop_to_thread();

		if (res.has_value()) {
			LogMessage *msg = res.value();

			PW_LOG_INFO("[Handle] reciev! ID: %u, MSG: %s",
				    (unsigned int)msg->irq_counter,
				    msg->payload.c_str());

			this->release_from_thread(msg);
		} else {
			pw::this_thread::sleep_for(
				std::chrono::milliseconds(100));
		}
	}
}

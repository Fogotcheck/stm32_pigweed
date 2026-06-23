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
#include "pw_unit_test/framework.h"
#include "pw_unit_test/logging_event_handler.h"

struct LogMessage : public pw::IntrusiveList<LogMessage>::Item {
	pw::InlineString<48> payload;
	pw::chrono::SystemClock::time_point timestamp;
	uint32_t irq_counter;
};

#include "stm32h7xx_hal.h"

inline void dwt_init()
{
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->LAR = 0xC5ACCE55;
	DWT->CYCCNT = 0;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

inline uint32_t dwt_get_cycles()
{
	return DWT->CYCCNT;
}

class App : public Bsp {
    private:
    public:
	static constexpr size_t POOL_SIZE = 8;

    private:
	LogMessage pool_[POOL_SIZE];

	pw::IntrusiveList<LogMessage> ready_list_;
	pw::IntrusiveList<LogMessage> free_list_;

	pw::sync::InterruptSpinLock lock_;

	void hwInit(void);
	void thread(void *arg);
	void thread_2();
	void tests();
	void init(void);

	friend class LogBufferTest_InitialState_Test;
	friend class LogBufferTest_AllocateAndRelease_Test;
	friend class LogBufferTest_PoolExhaustion_Test;

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

	size_t get_free_count()
	{
		std::lock_guard<pw::sync::InterruptSpinLock> lock(lock_);
		return free_list_.size();
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

	pw::Thread test_thread(options, [this]() { this->tests(); });
	test_thread.detach();

	pw::this_thread::sleep_for(std::chrono::milliseconds(5000));

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

void App::tests()
{
	PW_LOG_INFO("====== STARTING TARGET LOG TESTS ======");
	static pw::unit_test::LoggingEventHandler log_handler;
	pw::unit_test::RegisterEventHandler(&log_handler);
	RUN_ALL_TESTS();
	PW_LOG_INFO("====== TARGET LOG TESTS FINISHED ======");

	PW_LOG_INFO("====== STARTING PERFORMANCE BENCHMARK ======");

	dwt_init();

	App bench_app("BenchObj");
	constexpr uint32_t ITERATIONS = 10000;

	uint32_t start_loop = dwt_get_cycles();
	for (uint32_t i = 0; i < ITERATIONS; ++i) {
		asm volatile("" : : : "memory");
	}
	uint32_t end_loop = dwt_get_cycles();
	uint32_t loop_overhead = end_loop - start_loop;

	uint32_t start_bench = dwt_get_cycles();
	for (uint32_t i = 0; i < ITERATIONS; ++i) {
		LogMessage *msg = bench_app.allocate_from_isr();
		asm volatile("" : "+r"(msg));
		bench_app.release_from_thread(msg);
	}
	uint32_t end_bench = dwt_get_cycles();

	uint32_t total_cycles = (end_bench - start_bench) - loop_overhead;
	uint32_t cycles_per_op = total_cycles / ITERATIONS;

	uint32_t cpu_freq_mhz = HAL_RCC_GetSysClockFreq() / 1000000;
	float ns_per_op =
		(float)cycles_per_op * (1000.0f / (float)cpu_freq_mhz);

	PW_LOG_INFO("CPU Frequency: %u MHz", (unsigned int)cpu_freq_mhz);
	PW_LOG_INFO("Total cycles for %u iterations: %u",
		    (unsigned int)ITERATIONS, (unsigned int)total_cycles);
	PW_LOG_INFO("Average CPU cycles per (Allocate + Release): %u cycles",
		    (unsigned int)cycles_per_op);
	PW_LOG_INFO("Average time per operation: %u.%02u ns",
		    (unsigned int)ns_per_op,
		    (unsigned int)((ns_per_op - (int)ns_per_op) * 100));

	PW_LOG_INFO("====== PERFORMANCE BENCHMARK FINISHED ======");
}

TEST(LogBufferTest, InitialState)
{
	App test_app("TestObj");

	EXPECT_EQ(test_app.get_free_count(), App::POOL_SIZE);
}

TEST(LogBufferTest, AllocateAndRelease)
{
	App test_app("TestObj");

	LogMessage *msg = test_app.allocate_from_isr();
	ASSERT_NE(msg, nullptr);
	EXPECT_EQ(test_app.get_free_count(), App::POOL_SIZE - 1);

	test_app.release_from_thread(msg);
	EXPECT_EQ(test_app.get_free_count(), App::POOL_SIZE);
}

TEST(LogBufferTest, PoolExhaustion)
{
	App test_app("TestObj");

	for (size_t i = 0; i < App::POOL_SIZE; ++i) {
		ASSERT_NE(test_app.allocate_from_isr(), nullptr);
	}

	EXPECT_EQ(test_app.allocate_from_isr(), nullptr);
	EXPECT_EQ(test_app.get_free_count(), 0u);
}

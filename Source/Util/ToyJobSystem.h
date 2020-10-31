#pragma once

#include <deque>
#include <memory>
#include <functional>
#include <thread>

typedef int ToyJobId;
constexpr ToyJobId BAD_JOBID = -1;

class ToyJobSystem
{
public:
	static void init();
	static void shutdown();
	static ToyJobSystem* get() { return instance.get(); }

	ToyJobId schedule(const std::function<void()>& func, bool sync = false);
	bool isFinished(const ToyJobId& job) const;

private:
	struct Job
	{
		ToyJobId id;
		std::function<void()> func;
	};

	void run();

	static std::unique_ptr<ToyJobSystem> instance;
	bool isShuttingDown = false;

	std::deque<Job> jobQueue;
	std::unique_ptr<std::thread> workerThread;
};
#include "ToyJobSystem.h"

#include <assert.h>
#include <thread>
#include <mutex>

std::unique_ptr<ToyJobSystem> ToyJobSystem::instance;

static std::mutex mtx;
static std::condition_variable cv;

static ToyJobId nextJobId = 0;

void ToyJobSystem::init()
{
	instance = std::make_unique<ToyJobSystem>();
	instance->workerThread = std::make_unique<std::thread>([] { get()->run(); });
}

void ToyJobSystem::shutdown()
{
	instance->isShuttingDown = true;
	cv.notify_all();
	instance->workerThread->join();
	instance.reset();
}

ToyJobId ToyJobSystem::schedule(const std::function<void()>& func, bool sync)
{
	if (sync)
	{
		func();
		return BAD_JOBID;
	}
	//std::unique_lock<std::mutex> lck(mtx);
	Job job{nextJobId++, func};
	jobQueue.push_back(job);
	cv.notify_one();
	return job.id;
}

bool ToyJobSystem::isFinished(const ToyJobId& id) const
{
	return std::find_if(jobQueue.begin(), jobQueue.end(), [&id](const Job& j) { return j.id == id; }) == jobQueue.end();
}

void ToyJobSystem::run()
{
	while (true)
	{
		std::unique_lock<std::mutex> lck(mtx);
		cv.wait(lck, [&] {return !jobQueue.empty() || isShuttingDown; });
		if (isShuttingDown)
			break;
		assert(!jobQueue.empty());
		jobQueue.front().func();
		jobQueue.pop_front();
	}
}

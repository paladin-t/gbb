/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "datetime.h"
#include "platform.h"
#include "plus.h"
#include "text.h"
#include "work_queue.h"
#if WORK_QUEUE_THREAD_ENABLED
#	include <thread>
#endif /* WORK_QUEUE_THREAD_ENABLED */

/*
** {===========================================================================
** Macros and constants
*/

#if WORK_QUEUE_ENABLED
#	pragma message("Work queue enabled.")
#endif /* WORK_QUEUE_ENABLED */
#if WORK_QUEUE_THREAD_ENABLED
#	pragma message("Work queue thread enabled.")
#endif /* GAME_FEATURES_DEBUG_ENABLED */

/* ===========================================================================} */

/*
** {===========================================================================
** Schedulable tasks
*/

#if WORK_QUEUE_ENABLED
WorkTask::~WorkTask() {
	_handleRequest = nullptr;
	_handleResponse = nullptr;
	_operate = nullptr;
}

const WorkTask::RequestId &WorkTask::requestId(void) const {
	return _requestId;
}

WorkTask* WorkTask::requestId(RequestId id) {
	_requestId = id;

	return this;
}

const WorkTask::RequestHandler WorkTask::handleRequest(void) const {
	return _handleRequest;
}

void WorkTask::handleRequest(RequestHandler req) {
	_handleRequest = req;
}

const WorkTask::ResponseHandler WorkTask::handleResponse(void) const {
	return _handleResponse;
}

void WorkTask::handleResponse(ResponseHandler rsp) {
	_handleResponse = rsp;
}

const WorkTask::OperateHandler WorkTask::operate(void) const {
	return _operate;
}

void WorkTask::seal(void) {
	_timestamp = DateTime::ticks() - _timestamp;
}

void WorkTask::dispose(void) {
	// Do nothing.
}

long long WorkTask::cost(void) const {
	return _timestamp;
}

bool WorkTask::done(void) const {
	return true;
}

bool WorkTask::responsed(void) const {
	return true;
}

void WorkTask::responsed(bool rsp) {
	_responsed = rsp;
}

bool WorkTask::disassociated(void) const {
	return _disassociated;
}

void WorkTask::disassociated(bool dis) {
	_disassociated = dis;
}

WorkTask::WorkTask() {
	_disassociated = false;

	_timestamp = DateTime::ticks();
}

WorkTask::WorkTask(const WorkTask &other) {
	_disassociated = false;

	_timestamp = other._timestamp;
	_requestId = other._requestId;
	_handleRequest = other._handleRequest;
	_handleResponse = other._handleResponse;
	_operate = other._operate;
}

WorkTaskFunction::~WorkTaskFunction() {
	_operator = nullptr;
	_sealer = nullptr;
	_disposer = nullptr;
}

void WorkTaskFunction::seal(void) {
	_sealer(this, _intermedia);
	WorkTask::seal();
}

void WorkTaskFunction::dispose(void) {
	if (_disposer)
		_disposer(this, _intermedia);
}

bool WorkTaskFunction::done(void) const {
	return true;
}

bool WorkTaskFunction::responsed(void) const {
	return _responsed;
}

WorkTaskFunction* WorkTaskFunction::create(OperationHandler op, SealHandler seal, DisposeHandler dtor) {
	WorkTaskFunction* task = new WorkTaskFunction(op, seal, dtor);

	return task;
}

WorkTaskFunction::WorkTaskFunction(OperationHandler op, SealHandler seal, DisposeHandler dtor) : WorkTask() {
	_operator = op;
	_sealer = seal;
	_disposer = dtor;

	_operate = std::bind(&WorkTaskFunction::operate, this, std::placeholders::_1);
}

void WorkTaskFunction::operate(WorkTask* task) {
	_intermedia = _operator(task);
}

/* ===========================================================================} */

/*
** {===========================================================================
** Work queue
*/

class WorkQueueInternal : public WorkQueue {
public:
	virtual WorkTask* pop(void) = 0;

	virtual void pushResult(WorkTask* task) = 0;
	virtual WorkTask* popResult(void) = 0;
};

typedef std::list<WorkTask*> TaskList;

#if WORK_QUEUE_THREAD_ENABLED
typedef std::vector<std::thread> ThreadArray;

struct WorkQueueParams {
private:
	class WorkQueueImpl* _workQueue = nullptr;
	Mutex _requestLock;
	Mutex _responseLock;
	Mutex _quitLock;

public:
	WorkQueueParams() {
		_quitLock.lock();
	}
	~WorkQueueParams() {
	}

	class WorkQueueImpl* workQueue(void) {
		return _workQueue;
	}
	void workQueue(class WorkQueueImpl* impl) {
		_workQueue = impl;
	}

	int lockRequest(void) {
		_requestLock.lock();

		return 0;
	}
	int unlockRequest(void) {
		_requestLock.unlock();

		return 0;
	}
	int lockResponse(void) {
		_responseLock.lock();

		return 0;
	}
	int unlockResponse(void) {
		_responseLock.unlock();

		return 0;
	}

	void raiseQuit(void) {
		_quitLock.unlock();
	}
	bool needQuit(void) {
		if (_quitLock.tryLock()) {
			_quitLock.unlock();

			return true;
		} else {
			return false;
		}
	}
};

class WorkQueueImpl : public WorkQueueInternal {
private:
	WorkTask::RequestId _requestSeed = 0;
	WorkQueueParams* _params = nullptr;
	int _threadCount = 0;
	ThreadArray _threads;
	TaskList _taskList;
	TaskList _responseList;
	long _inQueue = 0;

public:
	WorkQueueImpl() {
	}
	virtual ~WorkQueueImpl() override {
	}

	virtual bool startup(const char* usage, int threadCount, bool rare) override {
		if (_params)
			return false;

		_threadCount = threadCount;

		_params = new WorkQueueParams();
		_params->workQueue(this);
		if (_threadCount <= 0)
			_threadCount = (int)threadsCount();
		for (int i = 0; i < _threadCount; ++i) {
			_threads.push_back(std::thread(_threadProc, usage, rare, i, (void*)_params));
		}

		return true;
	}
	virtual bool shutdown(void) override {
		if (!_params)
			return false;

		_params->raiseQuit();

		for (size_t i = 0; i < _threads.size(); ++i)
			_threads[i].join();
		_threads.clear();

		delete _params;
		_params = nullptr;

		return true;
	}

	virtual int threaded(void) const override {
		return _threadCount;
	}

	virtual WorkTask::RequestId push(WorkTask* task) override {
		_params->lockRequest();

		if (++_requestSeed == 0)
			++_requestSeed;
		WorkTask::RequestId reqid = _requestSeed;
		task->requestId(reqid);
		_taskList.push_back(task);
		if (task->handleRequest())
			task->handleRequest()(task);

		incInQueue();

		_params->unlockRequest();

		return reqid;
	}
	virtual WorkTask* pop(void) override {
		_params->lockRequest();

		WorkTask* result = nullptr;
		if (!_taskList.empty()) {
			result = _taskList.front();
			_taskList.pop_front();
		}

		_params->unlockRequest();

		return result;
	}

	virtual void pushResult(WorkTask* task) override {
		_params->lockResponse();

		_responseList.push_back(task);

		_params->unlockResponse();
	}
	virtual WorkTask* popResult(void) override {
		_params->lockResponse();

		WorkTask* result = nullptr;
		if (!_responseList.empty()) {
			result = _responseList.front();
			_responseList.pop_front();

			decInQueue();
		}

		_params->unlockResponse();

		return result;
	}

	virtual bool started(void) const override {
		return !!_params;
	}

	virtual long taskCount(void) const override {
		return getInQueue();
	}

	virtual bool allProcessed(void) const override {
		return getInQueue() == 0;
	}

	virtual void update(void) override {
		if (!allProcessed()) {
			WorkTask* task = popResult();
			if (task) {
				task->seal();
				if (task->handleResponse())
					task->handleResponse()(task);
				task->responsed(true);
				task->dispose();
				if (task->disassociated())
					delete task;
			}
		}
	}

private:
	unsigned threadsCount(void) const {
		return 1; // Constant number of threads.
	}

	long getInQueue(void) const {
		return _inQueue;
	}
	long incInQueue(void) {
		return ++_inQueue;
	}
	long decInQueue(void) {
		return --_inQueue;
	}

	static void* _threadProc(const char* usage, bool rare, int idx, void* params) {
		// Prepare.
		char name[32];
		snprintf(name, sizeof(name), "WORKER %s %d", usage, idx);
		Platform::threadName(name);

		Text::locale("C");

		WorkQueueParams* par = (WorkQueueParams*)params;
		WorkQueueImpl* que = par->workQueue();

		// Run until quit.
		constexpr const int STEP = 10;
		constexpr const int MAX_STEP = 640;
		const long long HALF_SEC = DateTime::fromSeconds(0.5);
		int step = STEP;
		long long lastActiveTimestamp = -1;
		while (!par->needQuit()) {
			WorkTask* task = que->pop();
			if (task) {
				if (rare) {
					if (lastActiveTimestamp != -1)
						lastActiveTimestamp = -1;
					if (step > STEP)                            // If the step value is not the default value then
						step = STEP;                            // reset to the default value.
				}

				task->operate()(task);
				if (task->done())
					que->pushResult(task);
				else
					que->push(task);
			} else {
				if (rare) {
					const long long now = DateTime::ticks();
					if (lastActiveTimestamp == -1)
						lastActiveTimestamp = now;
					if (now - lastActiveTimestamp > HALF_SEC) { // If has been free for 0.5f and
						if (step < MAX_STEP)                    // the sleep interval is less than ~0.64s then
							step *= 2;                          // double the sleep interval.
						lastActiveTimestamp = now;
					}
				}

				DateTime::sleep(step);
			}
		}

		// Purge.
		for (; ; ) {
			WorkTask* task = que->pop();
			if (task) {
				task->dispose();
				delete task;
			} else {
				break;
			}
		}
		for (; ; ) {
			WorkTask* task = que->popResult();
			if (task) {
				task->dispose();
				delete task;
			} else {
				break;
			}
		}

		// Finish.
		return nullptr;
	}
};
#else /* WORK_QUEUE_THREAD_ENABLED */
struct WorkQueueParams {
private:
	class WorkQueueImpl* _workQueue = nullptr;

public:
	WorkQueueParams() {
	}
	~WorkQueueParams() {
	}

	class WorkQueueImpl* workQueue(void) {
		return _workQueue;
	}
	void workQueue(class WorkQueueImpl* impl) {
		_workQueue = impl;
	}
};

class WorkQueueImpl : public WorkQueueInternal {
private:
	WorkTask::RequestId _requestSeed = 0;
	WorkQueueParams* _params = nullptr;
	TaskList _taskList;
	TaskList _responseList;
	long _inQueue = 0;

public:
	WorkQueueImpl() {
	}
	virtual ~WorkQueueImpl() override {
	}

	virtual bool startup(const char* /* usage */, int /* threadCount */, bool /* rare */) override {
		if (_params)
			return false;

		_params = new WorkQueueParams();
		_params->workQueue(this);

		return true;
	}
	virtual bool shutdown(void) override {
		if (!_params)
			return false;

		for (; ; ) {
			WorkTask* task = pop();
			if (task) {
				task->dispose();
				delete task;
			} else {
				break;
			}
		}
		for (; ; ) {
			WorkTask* task = popResult();
			if (task) {
				task->dispose();
				delete task;
			} else {
				break;
			}
		}

		delete _params;
		_params = nullptr;

		return true;
	}

	virtual int threaded(void) const override {
		return 0;
	}

	virtual WorkTask::RequestId push(WorkTask* task) override {
		if (++_requestSeed == 0)
			++_requestSeed;
		WorkTask::RequestId reqid = _requestSeed;
		task->requestId(reqid);
		_taskList.push_back(task);
		if (task->handleRequest())
			task->handleRequest()(task);

		incInQueue();

		_threadProc("IMMEDIATE", 0, _params);

		return reqid;
	}
	virtual WorkTask* pop(void) override {
		WorkTask* result = nullptr;
		if (!_taskList.empty()) {
			result = _taskList.front();
			_taskList.pop_front();
		}

		return result;
	}

	virtual void pushResult(WorkTask* task) override {
		_responseList.push_back(task);
	}
	virtual WorkTask* popResult(void) override {
		WorkTask* result = nullptr;
		if (!_responseList.empty()) {
			result = _responseList.front();
			_responseList.pop_front();

			decInQueue();
		}

		return result;
	}

	virtual bool started(void) const override {
		return !!_params;
	}

	virtual long taskCount(void) const override {
		return getInQueue();
	}

	virtual bool allProcessed(void) const override {
		return getInQueue() == 0;
	}

	virtual void update(void) override {
		if (!allProcessed()) {
			WorkTask* task = popResult();
			if (task) {
				task->seal();
				if (task->handleResponse())
					task->handleResponse()(task);
				task->responsed(true);
				task->dispose();
				if (task->disassociated())
					delete task;
			}
		}
	}

private:
	long getInQueue(void) const {
		return _inQueue;
	}
	long incInQueue(void) {
		return ++_inQueue;
	}
	long decInQueue(void) {
		return --_inQueue;
	}

	static void* _threadProc(const char* /* usage */, int /* idx */, void* params) {
		// Prepare.
		WorkQueueParams* par = (WorkQueueParams*)params;
		WorkQueueImpl* que = par->workQueue();

		// Run until quit.
		constexpr const int STEP = 10;
		do {
			WorkTask* task = que->pop();
			if (task) {
				task->operate()(task);
				if (task->done())
					que->pushResult(task);
				else
					que->push(task);
			} else {
				DateTime::sleep(STEP);
			}
		} while (false);

		// Finish.
		return nullptr;
	}
};
#endif /* WORK_QUEUE_THREAD_ENABLED */

WorkQueue::~WorkQueue() {
}

WorkQueue* WorkQueue::create(void) {
	WorkQueueImpl* result = new WorkQueueImpl();

	return result;
}

void WorkQueue::destroy(WorkQueue* ptr) {
	WorkQueueImpl* impl = static_cast<WorkQueueImpl*>(ptr);
	delete impl;
}
#endif /* WORK_QUEUE_ENABLED */

/* ===========================================================================} */

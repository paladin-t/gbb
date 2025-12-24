/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __WORK_QUEUE_H__
#define __WORK_QUEUE_H__

#include "../gbbasic.h"
#include <functional>

/*
** {===========================================================================
** Macros and constants
*/

#ifndef WORK_QUEUE_ENABLED
#	define WORK_QUEUE_ENABLED 1
#endif /* WORK_QUEUE_ENABLED */

#ifndef WORK_QUEUE_THREAD_ENABLED
#	if defined GBBASIC_OS_WIN
#		define WORK_QUEUE_THREAD_ENABLED GBBASIC_MULTITHREAD_ENABLED
#	elif defined GBBASIC_OS_MAC
#		define WORK_QUEUE_THREAD_ENABLED GBBASIC_MULTITHREAD_ENABLED
#	elif defined GBBASIC_OS_LINUX
#		define WORK_QUEUE_THREAD_ENABLED GBBASIC_MULTITHREAD_ENABLED
#	elif defined GBBASIC_OS_IOS
#		define WORK_QUEUE_THREAD_ENABLED GBBASIC_MULTITHREAD_ENABLED
#	elif defined GBBASIC_OS_ANDROID
#		define WORK_QUEUE_THREAD_ENABLED GBBASIC_MULTITHREAD_ENABLED
#	elif defined GBBASIC_OS_HTML
#		define WORK_QUEUE_THREAD_ENABLED GBBASIC_MULTITHREAD_ENABLED
#	else /* Platform macro. */
#		define WORK_QUEUE_THREAD_ENABLED GBBASIC_MULTITHREAD_ENABLED
#	endif /* Platform macro. */
#endif /* WORK_QUEUE_THREAD_ENABLED */

/* ===========================================================================} */

/*
** {===========================================================================
** Schedulable tasks
*/

#if WORK_QUEUE_ENABLED

/**
 * @brief Schedulable task.
 */
class WorkTask {
public:
	typedef unsigned long RequestId;

	typedef std::function<void(WorkTask*)> RequestHandler;
	typedef std::function<void(WorkTask*)> ResponseHandler;
	typedef std::function<void(WorkTask*)> OperateHandler;

protected:
	long long _timestamp = 0;
	RequestId _requestId = 0;
	RequestHandler _handleRequest = nullptr;
	ResponseHandler _handleResponse = nullptr;
	OperateHandler _operate = nullptr;
	bool _responsed = false;
	bool _disassociated = false;

public:
	virtual ~WorkTask();

	const RequestId &requestId(void) const;
	WorkTask* requestId(RequestId id);

	const RequestHandler handleRequest(void) const;
	void handleRequest(RequestHandler req);
	const ResponseHandler handleResponse(void) const;
	void handleResponse(ResponseHandler rsp);
	const OperateHandler operate(void) const;
	virtual void seal(void);
	virtual void dispose(void);
	long long cost(void) const;

	virtual bool done(void) const;

	virtual bool responsed(void) const;
	virtual void responsed(bool rsp);

	virtual bool disassociated(void) const;
	virtual void disassociated(bool dis);

protected:
	WorkTask();
	WorkTask(const WorkTask &other);
};

/**
 * @brief Schedulable task with custom function.
 */
class WorkTaskFunction : public WorkTask {
public:
	typedef std::function<uintptr_t(WorkTask*)> OperationHandler;
	typedef std::function<void(WorkTask*, uintptr_t)> SealHandler;
	typedef std::function<void(WorkTask*, uintptr_t)> DisposeHandler;

protected:
	OperationHandler _operator = nullptr;
	uintptr_t _intermedia = 0;
	SealHandler _sealer = nullptr;
	DisposeHandler _disposer = nullptr;

public:
	virtual ~WorkTaskFunction() override;

	virtual void seal(void) override;
	virtual void dispose(void) override;

	virtual bool done(void) const override;

	virtual bool responsed(void) const override;

	/**
	 * @param[in] op Called on work thread.
	 * @param[in] seal Called on main thread.
	 * @param[in] dtor Called on main thread.
	 */
	static WorkTaskFunction* create(OperationHandler op, SealHandler seal, DisposeHandler dtor);

private:
	WorkTaskFunction(OperationHandler op, SealHandler seal, DisposeHandler dtor);

	void operate(WorkTask* task);
};

/* ===========================================================================} */

/*
** {===========================================================================
** Work queue
*/

/**
 * @brief Asynchronized work queue which schedules tasks.
 */
class WorkQueue {
public:
	virtual ~WorkQueue();

	virtual bool startup(const char* usage, int threadCount, bool rare = false) = 0;
	virtual bool shutdown(void) = 0;

	virtual int threaded(void) const = 0;

	virtual WorkTask::RequestId push(WorkTask* task) = 0;

	virtual bool started(void) const = 0;

	virtual long taskCount(void) const = 0;

	virtual bool allProcessed(void) const = 0;

	virtual void update(void) = 0;

	static WorkQueue* create(void);
	static void destroy(WorkQueue* ptr);
};

#endif /* WORK_QUEUE_ENABLED */

/* ===========================================================================} */

#endif /* __WORK_QUEUE_H__ */

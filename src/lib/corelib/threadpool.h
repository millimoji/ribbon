#pragma once
#ifndef _RIBBON_THREADPOOL_H_
#define _RIBBON_THREADPOOL_H_
namespace Ribbon {

	class SingleSemaphore
	{
	public:
		SingleSemaphore() :
			m_signal(false)
		{}
		void wait()
		{
			bool varTrue = true;
			if (!m_signal.compare_exchange_strong(varTrue, false)) {
				std::unique_lock<std::mutex> threadLocalLock(m_mutex);
				m_cv.wait(threadLocalLock, [&] {
					bool varTrue = true;
					return m_signal.compare_exchange_strong(varTrue, false);
				});
			}
		}
		void signal()
		{
			m_signal = true;
			m_cv.notify_one();
		}

		SingleSemaphore(const SingleSemaphore&) = delete;
		SingleSemaphore(SingleSemaphore&&) = delete;
		SingleSemaphore& operator = (const SingleSemaphore&) = delete;
		SingleSemaphore& operator = (SingleSemaphore&&) = delete;

	private:
		std::mutex m_mutex;
		std::condition_variable m_cv;
		std::atomic_bool m_signal;
	};

	struct IAsyncTask
	{
		enum class State { waiting, running, finished };
		virtual void Run() = 0;
		virtual State GetState() const = 0;
	};

	template <class T>
	class AsyncTask :
		public IAsyncTask,
		public std::enable_shared_from_this<AsyncTask<T>>
	{
	public:
		AsyncTask(const std::function<T()>& taskBody) :
			m_state(State::waiting),
			m_taskBody(taskBody)
		{}
		virtual ~AsyncTask() {}

		State GetState() const override
		{
			return m_state;
		}
		T GetResult() const
		{
			if (m_state != State::finished) {
				m_exit.wait();
			}
			return m_result;
		}
		void Run() override
		{
			m_state = State::running;
			try {
				m_result = m_taskBody();
			}
			catch (...) {
				// todo: log
			}
			m_state = State::finished;
			m_exit.signal();
		}

	protected:
		mutable SingleSemaphore m_exit;
		std::atomic<State> m_state;
		std::function<T()> m_taskBody;
		T m_result;

		AsyncTask() = delete;
		AsyncTask(const AsyncTask&) = delete;
		AsyncTask& operator = (const AsyncTask&) = delete;
		AsyncTask(AsyncTask&&) = delete;
		AsyncTask& operator = (AsyncTask&&) = delete;
	};

	class TaskQueue
	{
	public:
		TaskQueue() :
			m_waitingThreads(0)
		{}
		~TaskQueue() {}

		int PushTask(const std::shared_ptr<IAsyncTask>& task) // waiting threads
		{
			std::lock_guard<std::mutex> lock(m_taskMutex);
			m_tasks.emplace_back(task);
			int waitingThread = m_waitingThreads;
			m_semaphore.signal();
			return waitingThread;
		}
		std::shared_ptr<IAsyncTask> TryGetTask()
		{
			std::shared_ptr<IAsyncTask> gotTask;
			{
				std::lock_guard<std::mutex> lock(m_taskMutex);
				if (m_tasks.size() > 0) {
					gotTask = m_tasks.front();
					m_tasks.pop_front();
				}
			}
			return gotTask;
		}
		std::shared_ptr<IAsyncTask> GetTask()
		{
			std::shared_ptr<IAsyncTask> gotTask = TryGetTask();

			if (gotTask) return gotTask;

			++m_waitingThreads;
			m_semaphore.wait();
			--m_waitingThreads;

			return TryGetTask(); // try to get again.
		}
		void FlushWaitingThreads()
		{
			while (m_waitingThreads > 0) {
				m_semaphore.signal();
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}
	private:
		SingleSemaphore m_semaphore;
		std::mutex m_taskMutex;
		std::deque<std::shared_ptr<IAsyncTask>> m_tasks;
		std::atomic_int m_waitingThreads;

		//
		TaskQueue(const TaskQueue&) = delete;
		TaskQueue(TaskQueue&&) = delete;
		TaskQueue& operator = (const TaskQueue&) = delete;
		TaskQueue& operator = (TaskQueue&&) = delete;
	};

class ThreadPool
{
public:
	ThreadPool() :
		m_activeThreads(0)
	{
		m_maxThreadCount = static_cast<int>(std::thread::hardware_concurrency()) * 3 / 4; // use 3/4
		m_maxThreadCount = std::max(m_maxThreadCount, 2); // at least 2 threads
	}

	~ThreadPool()
	{
		Clear();
	}

	void SetMaxThreads(int maxThreads)
	{
		m_maxThreadCount = maxThreads;
	}

	void RequestRaw(const std::shared_ptr<IAsyncTask>& atask)
	{
		if (m_taskQueue.PushTask(atask) == 0) { // no threads to run
			if (m_threads.size() < static_cast<size_t>(m_maxThreadCount)) {
				m_threads.push_back(std::thread([&]() {
					++m_activeThreads;
					ThreadFunction();
					--m_activeThreads;
				}));
			}
		}
	}
	template <class T>
	std::shared_ptr<AsyncTask<T>> Request(const std::function<T ()>& taskBody)
	{
		auto atask = std::make_shared<AsyncTask<T>>(taskBody);
		RequestRaw(atask);
		return atask;
	}

	void Clear()
	{
		m_exitFlag = true;

		while (m_activeThreads > 0) {
			m_taskQueue.FlushWaitingThreads();
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		for (auto& thread : m_threads) {
			thread.join();
		}
		m_threads.clear();

		m_exitFlag = false;
	}

	void ThreadFunction()
	{
		for (;;) {
			if (m_exitFlag) {
				return;
			}

			std::shared_ptr<IAsyncTask> asyncTask = m_taskQueue.GetTask();

			if (asyncTask) {
				try {
					asyncTask->Run();
				}
				catch (...) {
					// TODO: log
				}
			}
		}
	}

	//
	TaskQueue m_taskQueue;
	volatile bool m_exitFlag = false;
	int m_maxThreadCount;
	std::atomic_int m_activeThreads;
	std::vector<std::thread> m_threads;

	//
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool(ThreadPool&&) = delete;
	ThreadPool& operator = (const ThreadPool&) = delete;
};

extern ThreadPool GlobalThreadPool;

} // Ribbon
#endif // _RIBBON_THREADPOOL_H_

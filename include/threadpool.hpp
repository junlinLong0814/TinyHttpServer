#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include <thread>
#include <vector>
#include <list>
#include <mutex>
#include <memory>
#include <condition_variable>
#include <stdexcept>
#include <functional>
#include <future>
#include "MyLog.h"

template <class T>
class threadPool
{
public:
	threadPool(int threadNumber = 4, int maxTasks = 10000);
	~threadPool();
	/*添加任务
	万能引用，引用折叠，完美转发*/
	template<class F,class ...Args>
	bool addTask(F&& f,Args&&... args);
private:
	void workFun();
private:
	std::vector<std::thread*> pWorkers_;		//保存线程
	//std::list<T*> tasksBuf_;					//任务缓冲
	std::list<std::function<void()>> tasksBuf_;	//任务缓冲，装载人物类运行函数
	std::mutex mTasksBuf_;						//互斥访问任务队列
	std::condition_variable condition_;			//新任务到来时唤醒空闲线程
	int maxTasks_;								//最大任务数量
	bool stop_;									//是否关闭
};

template <class T>
threadPool<T>::threadPool(int threadNumber, int maxTasks) :maxTasks_(maxTasks), stop_(false)
{
	if (threadNumber <= 0 || maxTasks <= 0)
	{
		throw std::runtime_error("constructor args error");
	}
	for (int i = 0; i<threadNumber; ++i)
	{
		std::thread* pt = new std::thread(&threadPool<T>::workFun, this);
		pWorkers_.push_back(pt);
		//pt->detach();
	}
	LOG_INFO("%d threads be create!\n", threadNumber);
}

template <class T>
void threadPool<T>::workFun()
{
	for (;;)
	{
		std::function<void()> task;
		{
			std::unique_lock<std::mutex> lock(mTasksBuf_);
			/*任务队列非空 或 线程池要结束时 唤醒 线程*/
			condition_.wait(lock ,
				[this]() {return stop_ || !tasksBuf_.empty(); });

			if (stop_ && tasksBuf_.empty()) return;
			/*取队首任务*/

			task = tasksBuf_.front();
			tasksBuf_.pop_front();
		}
		if (!task) { continue; }

		task();
	}
}

// template <class T>
// bool threadPool<T>::addTask(T* task)
// {
// 	{
// 		std::unique_lock<std::mutex> lock(mTasksBuf_);
// 		tasksBuf_.push_back(task);
// 	}
// 	condition_.notify_one();
// 	return true;
// }

template<class T>
template<class F,class...Args>
bool threadPool<T>::addTask(F&& f,Args&&...args)
{
	using return_type = typename std::result_of<F(Args...)>::type;
	using Task = std::function<return_type()>;

	Task task = std::bind(std::forward<F>(f),std::forward<Args>(args)...);
	{
		std::unique_lock<std::mutex> lock(mTasksBuf_);
		tasksBuf_.push_back(task);
	}
	condition_.notify_one();
	return true;
}


template <class T>
threadPool<T>::~threadPool()
{
	{
		std::unique_lock<std::mutex> lock(mTasksBuf_);
		stop_ = true;
	}
	condition_.notify_all();
	for (auto pthread : pWorkers_)
	{
		pthread->join();
	}
	for (auto pthread : pWorkers_)
	{
		delete pthread;
		pthread = nullptr;
	}
}
#endif

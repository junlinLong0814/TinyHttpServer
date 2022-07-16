#include "MySemaphore.h"

mySemaphore::mySemaphore(int count):count_(count)
{

}

void mySemaphore::semSignal()
{
	std::unique_lock<std::mutex> lock(mutex_);
	++count_;
	if(count_<=0)
	{
		cv_.notify_one();
	}
}

void mySemaphore::semWait()
{
	std::unique_lock<std::mutex> lock(mutex_);
	--count_;
	cv_.wait(lock,[this](){return this->count_>0;});
}

mySemaphore::~mySemaphore()
{

}

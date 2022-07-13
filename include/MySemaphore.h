#ifndef _MY_SEMAPHORE_HPP_
#define _MY_SEMAPHORE_HPP_

#include <condition_variable>
#include <mutex>

class mySemaphore
{
public:
	explicit mySemaphore(int count=1);
	~mySemaphore();


	void semWait();
	void semSignal();

	/*因为mutex 和 con_var不能=,所以不能用赋值构造,权宜之计*/
	inline void setCount(int count) {count_=count;}
private:
	int count_;					//代表资源数量
	std::mutex mutex_;			//互斥访问count
	std::condition_variable cv_;	
	
};



#endif

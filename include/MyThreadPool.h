#ifndef _MYTHREADPOOL_H
#define _MYTHREADPOOL_H

#include <thread>
#include <vector>
#include <list>
#include <mutex>
#include <memory>
#include <condition_variable>
#include <stdexcept>
#include <functional>
#include <future>
#include <algorithm>
#include "MyLog.h"

class MyThreadPool
{
public:
    MyThreadPool() = default; 
    
    ~MyThreadPool()
    {
        {
            std::unique_lock<std::mutex> lock(mxTasksList);
            bStop = true;
        }
        cdNewTask.notify_all();
        for(std::shared_ptr<std::thread>& spThread : arrThreads)
        {
            spThread->join();
        }
        LOG_INFO("ThreadPool dtor");
    }

    int threadPoolInit(int nThreadNum = 4,int nMaxTasks = 10000)
    {
        if(nThreadNum <= 0 || nMaxTasks <= 0)
        {
            nThreadNum = std::max(1,nThreadNum);
            nMaxTasks = std::max(1,nMaxTasks);
        }
        nThreadNum = nThreadNum;
        nMaxTasks = nMaxTasks;
        for(int i = 0 ; i < nThreadNum ; ++i)
        {
            std::shared_ptr<std::thread> spThread= std::make_shared<std::thread>(&MyThreadPool::threadWorkFunc,this);
            arrThreads.push_back(spThread);
        }
    }

    template<class F,class ...Args>
    bool addTask(F&& f,Args&&... args);
private:
    void threadWorkFunc()
    {
        for(; ;)
        {
            std::function<void()> fTask = nullptr;
            if(!bStop)
            {
                std::unique_lock<std::mutex> lock(mxTasksList);
                /*任务队列有待处理的任务 || 结束线程池*/
                cdNewTask.wait(lock,
                    [this](){ return bStop || !lTasksList.empty();});
            }
            

            if(bStop && lTasksList.empty()) return ;

            if(!lTasksList.empty())
            {
                fTask = lTasksList.front();
                lTasksList.pop_front();
            }
        
            if(fTask == nullptr) continue; 

            fTask();  
        }
    }
private:
    std::vector<std::shared_ptr<std::thread>> arrThreads;
    std::list<std::function<void()>> lTasksList;
    std::mutex mxTasksList;
    std::condition_variable cdNewTask;
    int nThreadNum;
    int nMaxTasks;
    bool bStop;
    
};






template<class F,class... Args>
bool MyThreadPool::addTask(F&& f,Args&&... args)
{
    using return_type = typename std::result_of<F(Args...)>::type;
    using Task = std::function<return_type()>;

    Task task = std::bind(std::forward<F>(f),std::forward<Args>(args)...);
    {
        std::unique_lock<std::mutex> lock(mxTasksList);
        lTasksList.push_back(task);
    }
    cdNewTask.notify_one();
    return true;
}


#endif
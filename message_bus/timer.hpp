#pragma once

#include <chrono>
#include <thread>
#include <atomic>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <functional>
class Timer
{
public:
    Timer() : _stopped(true), _try_to_stop(false) {}
    Timer(const Timer &t)
    {
        _stopped = t._stopped.load();         //原子操作
        _try_to_stop = t._try_to_stop.load(); //原子操作
    }
    ~Timer()
    {
        stop();
    }
    /**
     * @brief 开始定时器，间隔固定时间调用某函数
     *
     * @param interval 间隔固定时间
     * @param task 函数回调
     */
    void start(int interval, std::function<void()> task)
    {
        if (_stopped == false)
            return;

        _stopped = false;
        std::thread([this, interval, task]()
                    {
            while (!_try_to_stop)//没有尝试停止定时器
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(interval));
                task();
            }
            // 停止定时器
            {
                std::lock_guard<std::mutex> locker(_mutex);
                _stopped = true;
                _stop_cond.notify_one();
            } })
            .detach();
        /* detach 是 std::thread 类的一个成员函数，用于将线程对象与底层线程的关联断开，使得底层线程能够独立运行，不再与线程对象有任何关联。
        一旦调用了 detach，线程对象就不能再对底层线程进行任何操作，也不能获取线程的执行结果。
        当使用 detach 方法将线程对象与底层线程分离时，意味着不再关心该线程的执行结果和状态，也不会对该线程进行等待操作。
        线程的生命周期将完全由底层线程自行管理，如果底层线程在 detach 之后结束运行，它的资源将被系统自动回收。
        但需要注意的是，如果没有明确调用 detach 或 join 方法来结束线程，线程可能会成为“僵尸”线程，导致资源泄露和其他问题。
        总的来说，detach 的作用是将线程对象与底层线程分离，使得线程能够独立运行，并不再与线程对象有任何关联。。*/
    }
    /**
     * @brief 停止定时器
     *
     */
    void stop()
    {
        if (_stopped)
            return;
        if (_try_to_stop)
        {
            return;
        }
        _try_to_stop = true;
        {
            std::unique_lock<std::mutex> locker(_mutex);
            _stop_cond.wait(locker, [this]
                            { return _stopped == true; });
            if (_stopped == true)
                _try_to_stop = false;
        }
    }

    bool is_stopped()
    {
        return _stopped;
    }

private:
    std::atomic<bool> _stopped;     //定时器是否停止
    std::atomic<bool> _try_to_stop; //正在尝试停止
    std::mutex _mutex;
    std::condition_variable _stop_cond;
};

#pragma once

#include <map>
#include <vector>
#include <list>
#include <mutex>
#include "timer.hpp"

typedef std::function<void(std::string param1, int param2)> Callback_t;
typedef std::function<void()> TimeOutCallback_t;

enum CallbackType_t
{
    ALWAYS = 0, //不会删除此消息对应的条目
    ONCE
};

struct CallbackItem_t
{
    Callback_t callback = nullptr;
    TimeOutCallback_t timeOutCallback = nullptr;
    uint32_t timeoutInterval = 1000; // milliseconds
    uint64_t timeoutStamp = 0;       // microseconds
    std::vector<int> msgNumVec;
    CallbackType_t callbackType = ALWAYS;
};

class MessageBus
{
public:
    static MessageBus &instance()
    {
        static MessageBus ins;
        return ins;
    }
    void publish(int msg, std::string param1, int param2 = 0);
    void timeOutCheck();
    bool subscribe(CallbackItem_t);
    void reset();
    void stop();
    void start();

private:
    MessageBus() = default;
    MessageBus(const MessageBus &) = delete;            // 删除拷贝构造函数
    MessageBus &operator=(const MessageBus &) = delete; // 删除拷贝赋值运算符

    typedef std::shared_ptr<CallbackItem_t> CallbackItem_ptr;
    bool subscribe(int msg, CallbackItem_ptr);
    bool unsubscribe(int msg, CallbackItem_ptr);
    void regTimeOutCallback(CallbackItem_ptr);

    typedef std::map<int, std::vector<CallbackItem_ptr>> CallbackMap_t;
    CallbackMap_t _callbackMap; // 消息调度映射

    std::list<CallbackItem_ptr> _timeoutCheckList; // 超时检查列表

    std::mutex _timeoutCheckListMutex;
    std::mutex _callbackMapMutex;
    Timer _timer;
};

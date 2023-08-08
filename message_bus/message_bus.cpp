#include "message_bus.h"
#include "timer.hpp"
#include <memory>
#include <chrono>
#include <algorithm>

/**
 * @brief 获取当前UTC时间，转化为ms
 *
 * @return uint64_t 当前UTC时间，单位ms
 */
static uint64_t getTimeStamp()
{
    /**
     std::chrono::system_clock 是C++标准库提供的一个时钟类，用于测量相对于Unix纪元的时间点或时间段
     std::chrono::system_clock::now() 用于获取当前时刻的时间点。
     time_since_epoch() 用于获取当前时刻相对于Unix纪元的时间段
     std::chrono::duration_cast<std::chrono::microseconds>() 是一个用于时间段转换的函数模板，将时间段转换为微秒级别的时间段。
     */
    std::chrono::microseconds ms = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch());

    return ms.count();
}
/**
 * @brief 注册元素，主要是注册到消息映射表
 *
 * @param msg  消息编号
 * @param item  需要注册的元素
 * @return true
 * @return false
 */
bool MessageBus::subscribe(int msg, CallbackItem_ptr item)
{
    std::unique_lock<std::mutex> lck(_callbackMapMutex);
    if (_callbackMap.find(msg) != _callbackMap.end()) //找到对应的消息编号
    {
        /* 同一个消息可能对应多个元素 */
        auto &itemVec = _callbackMap[msg];
        if (find(itemVec.begin(), itemVec.end(), item) == itemVec.end()) //没有找到对应的元素
            itemVec.push_back(item);                                     //就把他放进去
    }
    else
    {
        /* 在映射表中放入新的元素 */
        std::vector<CallbackItem_ptr> newItemVec;
        newItemVec.push_back(item);
        _callbackMap.insert(CallbackMap_t::value_type(msg, newItemVec));
    }

    return true;
}
/**
 * @brief 注册条目
 *
 * @param src 被注册的条目
 * @return true
 * @return false
 */
bool MessageBus::subscribe(CallbackItem_t src)
{
    auto item_obj = std::make_shared<CallbackItem_t>(src);
    auto &item = item_obj;
    if (item->msgNumVec.empty()) //条目中没有命令编号，直接退出
        return false;

    std::unique_lock<std::mutex> lck(_timeoutCheckListMutex);
    for (auto &tmp : _timeoutCheckList)
    {
        if (tmp->msgNumVec == item->msgNumVec && tmp->callback.target<Callback_t>() == item->callback.target<Callback_t>() && tmp->timeOutCallback.target<TimeOutCallback_t>() == item->timeOutCallback.target<TimeOutCallback_t>())
        {
            return false; // 已经存在这个条目，直接退出，不注册
        }
    }
    lck.unlock();

    for (auto &m : item->msgNumVec)
    {
        if (!subscribe(m, item)) //注册到消息映射表中
            return false;        //不会执行
    }

    if (item->timeOutCallback != nullptr)
        regTimeOutCallback(item); //放入超时检查列表

    return true;
}
/**
 * @brief 消息映射表解除注册
 *
 * @param msg 消息编号
 * @param item 需要解除的条目
 * @return true
 * @return false
 */
bool MessageBus::unsubscribe(int msg, CallbackItem_ptr item)
{
    std::unique_lock<std::mutex> lck(_callbackMapMutex);
    if (_callbackMap.find(msg) != _callbackMap.end()) //找到消息对应的条目
    {
        auto &callbackItemVec = _callbackMap[msg];
        /* 从映射中找到对应德item */
        auto result_obj = find(callbackItemVec.begin(), callbackItemVec.end(), item);
        auto &result = result_obj;
        if (result != callbackItemVec.end()) //找到了
        {
            callbackItemVec.erase(result); //删除，注意这里只是删除了消息mag的map中的item，其他的条目没有删除
            return true;
        }
    }
    return false;
}
/**
 * @brief 注册条目到超时检测列表中
 *
 * @param item 被注册的条目
 */
void MessageBus::regTimeOutCallback(CallbackItem_ptr item)
{
    item->timeoutStamp = item->timeoutInterval * 1000 + getTimeStamp(); //超时时间

    std::unique_lock<std::mutex> lck(_timeoutCheckListMutex);

    /* 找到和item超时时间大的元素 */
    auto it = std::find_if(_timeoutCheckList.begin(), _timeoutCheckList.end(), [item](CallbackItem_ptr tmp)
                           { return (tmp->timeoutStamp > item->timeoutStamp); });
    if (it != _timeoutCheckList.end()) //找到了，就将item插入到it之前
        _timeoutCheckList.insert(it, item);
    else
        _timeoutCheckList.push_back(item); //找不到就放到最后
}
/**
 * @brief 消息总线启动
 *
 */
void MessageBus::start()
{
    if (_timer.is_stopped())
    {
        _timer.start(100, std::bind(&MessageBus::timeOutCheck, this));
    }
}
/**
 * @brief 向消息总线放入消息
 *
 * @param msg 消息编号
 * @param param1 参数1
 * @param param2 参数2
 */
void MessageBus::publish(int msg, std::string param1, int param2)
{
    std::unique_lock<std::mutex> lck1(_timeoutCheckListMutex);

    // 从_timeoutCheckList中删除消息的关联项。
    auto it_obj = _timeoutCheckList.begin();
    for (auto &it = it_obj; it != _timeoutCheckList.end(); ++it)
    {
        auto &item = *it;
        if (find(item->msgNumVec.begin(), item->msgNumVec.end(), msg) != item->msgNumVec.end()) //找了对应的元素
        {
            _timeoutCheckList.erase(it); //把这个元素删除
            /* 这个地方有问题啊，应该继续查找，删除所有存在msg的元素 */
            break;
        }
    }
    lck1.unlock();

    std::unique_lock<std::mutex> lck2(_callbackMapMutex);
    //从消息调度映射表中找msg消息
    if (_callbackMap.find(msg) == _callbackMap.end()) //没有找到
        return;
    auto _it_obj = _callbackMap[msg].begin();
    for (auto &it = _it_obj; it != _callbackMap[msg].end();)
    {
        auto &item = *it;

        if (item->callback != nullptr)
            item->callback(param1, param2); //调用callback函数

        if (item->callbackType == CallbackType_t::ALWAYS)
        {
            ++it;
            continue;
        }

        /*因为item->msgNumVec中的一条消息已经到达，所以需要删除item->msgNumVec中的其他消息。*/
        for (auto m : item->msgNumVec)
        {
            if (_callbackMap.find(m) != _callbackMap.end() && msg != m)
            {
                auto &itemVec = _callbackMap[m];
                auto result_obj = find(itemVec.begin(), itemVec.end(), item);
                auto &result = result_obj;
                if (result != itemVec.end())
                {
                    itemVec.erase(result);

                    // 如果消息的回调向量为空，则将消息从回调列表中删除。
                    if (itemVec.empty())
                        _callbackMap.erase(m);
                    continue;
                }
            }
        }
        it = _callbackMap[msg].erase(it);
    }
    // 如果消息的回调向量为空，则将消息从回调列表中删除。
    if (_callbackMap[msg].empty())
        _callbackMap.erase(msg);
    lck2.unlock();
}
/**
 * @brief 消息总线超时检查函数
 *
 */
void MessageBus::timeOutCheck()
{
    std::unique_lock<std::mutex> lck(_timeoutCheckListMutex);

    uint64_t currentTimeStamp = getTimeStamp();
    auto it_obj = _timeoutCheckList.begin();
    for (auto &it = it_obj; it != _timeoutCheckList.end();)
    {
        if ((*it)->timeoutStamp > currentTimeStamp) // not timeout
        {
            ++it;
            continue;
        }

        if ((*it)->timeOutCallback != nullptr)
            (*it)->timeOutCallback(); //执行超时函数

        for (auto m : (*it)->msgNumVec)
            unsubscribe(m, *it); //从映射表中解除注册

        it = _timeoutCheckList.erase(it); //从超时列表中删除条目
    }
}
/**
 * @brief 消息总线复位，清空左右注册的消息
 *
 */
void MessageBus::reset()
{
    std::unique_lock<std::mutex> lck1(_callbackMapMutex);
    std::unique_lock<std::mutex> lck2(_timeoutCheckListMutex);

    _timeoutCheckList.clear();
    _callbackMap.clear();
}
/**
 * @brief 消息总线停止
 *
 */
void MessageBus::stop()
{
    _timer.stop();
}
#include "message_bus.h"
#include "timer.hpp"
#include <memory>
#include <chrono>
#include <algorithm>

/**
 * @brief ��ȡ��ǰUTCʱ�䣬ת��Ϊms
 *
 * @return uint64_t ��ǰUTCʱ�䣬��λms
 */
static uint64_t getTimeStamp()
{
    /**
     std::chrono::system_clock ��C++��׼���ṩ��һ��ʱ���࣬���ڲ��������Unix��Ԫ��ʱ����ʱ���
     std::chrono::system_clock::now() ���ڻ�ȡ��ǰʱ�̵�ʱ��㡣
     time_since_epoch() ���ڻ�ȡ��ǰʱ�������Unix��Ԫ��ʱ���
     std::chrono::duration_cast<std::chrono::microseconds>() ��һ������ʱ���ת���ĺ���ģ�壬��ʱ���ת��Ϊ΢�뼶���ʱ��Ρ�
     */
    std::chrono::microseconds ms = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch());

    return ms.count();
}
/**
 * @brief ע��Ԫ�أ���Ҫ��ע�ᵽ��Ϣӳ���
 *
 * @param msg  ��Ϣ���
 * @param item  ��Ҫע���Ԫ��
 * @return true
 * @return false
 */
bool MessageBus::subscribe(int msg, CallbackItem_ptr item)
{
    std::unique_lock<std::mutex> lck(_callbackMapMutex);
    if (_callbackMap.find(msg) != _callbackMap.end()) //�ҵ���Ӧ����Ϣ���
    {
        /* ͬһ����Ϣ���ܶ�Ӧ���Ԫ�� */
        auto &itemVec = _callbackMap[msg];
        if (find(itemVec.begin(), itemVec.end(), item) == itemVec.end()) //û���ҵ���Ӧ��Ԫ��
            itemVec.push_back(item);                                     //�Ͱ����Ž�ȥ
    }
    else
    {
        /* ��ӳ����з����µ�Ԫ�� */
        std::vector<CallbackItem_ptr> newItemVec;
        newItemVec.push_back(item);
        _callbackMap.insert(CallbackMap_t::value_type(msg, newItemVec));
    }

    return true;
}
/**
 * @brief ע����Ŀ
 *
 * @param src ��ע�����Ŀ
 * @return true
 * @return false
 */
bool MessageBus::subscribe(CallbackItem_t src)
{
    auto item_obj = std::make_shared<CallbackItem_t>(src);
    auto &item = item_obj;
    if (item->msgNumVec.empty()) //��Ŀ��û�������ţ�ֱ���˳�
        return false;

    std::unique_lock<std::mutex> lck(_timeoutCheckListMutex);
    for (auto &tmp : _timeoutCheckList)
    {
        if (tmp->msgNumVec == item->msgNumVec && tmp->callback.target<Callback_t>() == item->callback.target<Callback_t>() && tmp->timeOutCallback.target<TimeOutCallback_t>() == item->timeOutCallback.target<TimeOutCallback_t>())
        {
            return false; // �Ѿ����������Ŀ��ֱ���˳�����ע��
        }
    }
    lck.unlock();

    for (auto &m : item->msgNumVec)
    {
        if (!subscribe(m, item)) //ע�ᵽ��Ϣӳ�����
            return false;        //����ִ��
    }

    if (item->timeOutCallback != nullptr)
        regTimeOutCallback(item); //���볬ʱ����б�

    return true;
}
/**
 * @brief ��Ϣӳ�����ע��
 *
 * @param msg ��Ϣ���
 * @param item ��Ҫ�������Ŀ
 * @return true
 * @return false
 */
bool MessageBus::unsubscribe(int msg, CallbackItem_ptr item)
{
    std::unique_lock<std::mutex> lck(_callbackMapMutex);
    if (_callbackMap.find(msg) != _callbackMap.end()) //�ҵ���Ϣ��Ӧ����Ŀ
    {
        auto &callbackItemVec = _callbackMap[msg];
        /* ��ӳ�����ҵ���Ӧ��item */
        auto result_obj = find(callbackItemVec.begin(), callbackItemVec.end(), item);
        auto &result = result_obj;
        if (result != callbackItemVec.end()) //�ҵ���
        {
            callbackItemVec.erase(result); //ɾ����ע������ֻ��ɾ������Ϣmag��map�е�item����������Ŀû��ɾ��
            return true;
        }
    }
    return false;
}
/**
 * @brief ע����Ŀ����ʱ����б���
 *
 * @param item ��ע�����Ŀ
 */
void MessageBus::regTimeOutCallback(CallbackItem_ptr item)
{
    item->timeoutStamp = item->timeoutInterval * 1000 + getTimeStamp(); //��ʱʱ��

    std::unique_lock<std::mutex> lck(_timeoutCheckListMutex);

    /* �ҵ���item��ʱʱ����Ԫ�� */
    auto it = std::find_if(_timeoutCheckList.begin(), _timeoutCheckList.end(), [item](CallbackItem_ptr tmp)
                           { return (tmp->timeoutStamp > item->timeoutStamp); });
    if (it != _timeoutCheckList.end()) //�ҵ��ˣ��ͽ�item���뵽it֮ǰ
        _timeoutCheckList.insert(it, item);
    else
        _timeoutCheckList.push_back(item); //�Ҳ����ͷŵ����
}
/**
 * @brief ��Ϣ��������
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
 * @brief ����Ϣ���߷�����Ϣ
 *
 * @param msg ��Ϣ���
 * @param param1 ����1
 * @param param2 ����2
 */
void MessageBus::publish(int msg, std::string param1, int param2)
{
    std::unique_lock<std::mutex> lck1(_timeoutCheckListMutex);

    // ��_timeoutCheckList��ɾ����Ϣ�Ĺ����
    auto it_obj = _timeoutCheckList.begin();
    for (auto &it = it_obj; it != _timeoutCheckList.end(); ++it)
    {
        auto &item = *it;
        if (find(item->msgNumVec.begin(), item->msgNumVec.end(), msg) != item->msgNumVec.end()) //���˶�Ӧ��Ԫ��
        {
            _timeoutCheckList.erase(it); //�����Ԫ��ɾ��
            /* ����ط������Ⱑ��Ӧ�ü������ң�ɾ�����д���msg��Ԫ�� */
            break;
        }
    }
    lck1.unlock();

    std::unique_lock<std::mutex> lck2(_callbackMapMutex);
    //����Ϣ����ӳ�������msg��Ϣ
    if (_callbackMap.find(msg) == _callbackMap.end()) //û���ҵ�
        return;
    auto _it_obj = _callbackMap[msg].begin();
    for (auto &it = _it_obj; it != _callbackMap[msg].end();)
    {
        auto &item = *it;

        if (item->callback != nullptr)
            item->callback(param1, param2); //����callback����

        if (item->callbackType == CallbackType_t::ALWAYS)
        {
            ++it;
            continue;
        }

        /*��Ϊitem->msgNumVec�е�һ����Ϣ�Ѿ����������Ҫɾ��item->msgNumVec�е�������Ϣ��*/
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

                    // �����Ϣ�Ļص�����Ϊ�գ�����Ϣ�ӻص��б���ɾ����
                    if (itemVec.empty())
                        _callbackMap.erase(m);
                    continue;
                }
            }
        }
        it = _callbackMap[msg].erase(it);
    }
    // �����Ϣ�Ļص�����Ϊ�գ�����Ϣ�ӻص��б���ɾ����
    if (_callbackMap[msg].empty())
        _callbackMap.erase(msg);
    lck2.unlock();
}
/**
 * @brief ��Ϣ���߳�ʱ��麯��
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
            (*it)->timeOutCallback(); //ִ�г�ʱ����

        for (auto m : (*it)->msgNumVec)
            unsubscribe(m, *it); //��ӳ����н��ע��

        it = _timeoutCheckList.erase(it); //�ӳ�ʱ�б���ɾ����Ŀ
    }
}
/**
 * @brief ��Ϣ���߸�λ���������ע�����Ϣ
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
 * @brief ��Ϣ����ֹͣ
 *
 */
void MessageBus::stop()
{
    _timer.stop();
}
## message_bus
+ 该模块一般用在客户端；
+ 基于消息的程序架构（如C/S架构），在发送一个request消息后可能会遇到这样的情况：
  + 等到response消息，其中response分为肯定回答和否定回答；
  + 在超时时间内没有收到回复。

业务往往需要等到响应结果以后再继续其他业务，设计方法是：采用回调的方法调用响应函数，维护一个定时器，不停检查是否有超时消息产生，并调用对应回调函数。

+ 需要使用到定时器，定时器默认间隔100ms。

+ 维护了一个超时消息的列表。

## 主要结构体

```cpp
typedef std::function<void(std::string param1, int param2)> Callback_t;
typedef std::function<void()> TimeOutCallback_t;
struct CallbackItem_t
{
    Callback_t callback = nullptr;  
    TimeOutCallback_t timeOutCallback = nullptr;
    uint32_t timeoutInterval = 1000;    // 超时时间，milliseconds
    uint64_t timeoutStamp = 0;  // 内部使用时间戳，用户无需填写，microseconds
    std::vector<int> msgNumVec; // 需要关注的消息号
    CallbackType_t callbackType = ALWAYS;
};
typedef std::shared_ptr<CallbackItem_t> CallbackItem_ptr;
typedef std::map<int, std::vector<CallbackItem_ptr>> CallbackMap_t;
CallbackMap_t _callbackMap;   // message dispatch map
std::list<CallbackItem_ptr> _timeoutCheckList;	// timeout check list
```

从注册开始计算时间，超时2s没有收到TEST_MESSAGE1，就删除onTest1，超时5s没有收到TEST_MESSAGE2或者TEST_MESSAGE3，就删除onTest2
``` cpp
    CallbackItem_t item1;
    item1.msgNumVec.push_back(TEST_MESSAGE1);
    item1.callback = onTest1;
    item1.timeoutInterval = 2000;
    item1.timeOutCallback = timeOutCallback1;

    item1.callbackType = CallbackType_t::ALWAYS;
    MessageBus::instance().subscribe(item1);

    CallbackItem_t item2;
    item2.msgNumVec.push_back(TEST_MESSAGE2);
    item2.msgNumVec.push_back(TEST_MESSAGE3);
    item2.callback = onTest2;
    item2.timeoutInterval = 5000;
    item2.timeOutCallback = timeOutCallback2;
    MessageBus::instance().subscribe(item2);
  ```

编译

在代码根目录下新建build目录，进入build目录执行
``` cpp
cmake ../
```
然后在build目录下执行
``` cpp
make
```
可执行文件生成在build/test_app目录下



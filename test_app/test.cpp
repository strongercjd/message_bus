#include "message_bus.h"

#include <iostream>
#include <string>

using namespace std;

enum TestMessage_t
{
    TEST_MESSAGE1 = 1,
    TEST_MESSAGE2,
    TEST_MESSAGE3,
    TEST_MESSAGE4,
    TEST_MESSAGE5
};

static void onTest1(std::string param1, int param2)
{
    cout << "onTest1 - " << endl;
    // << "param1:" << param1 << " - param2:" << param2 << endl;
}

static void onTest2(std::string param1, int param2)
{
    cout << "onTest2 - " << endl;
    // << "param1:" << param1 << " - param2:" << param2 << endl;
}

static void timeOutCallback1()
{
    cout << "TimeOutCallback1" << endl;
}

static void timeOutCallback2()
{
    cout << "TimeOutCallback2" << endl;
}

int main()
{
    MessageBus::instance().start();
    auto t = std::thread([]()
                         {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                        while (1)
                        {
                            srand((unsigned int)time(0));
                            int n = rand() % 4 + 1;
                            cout << '\n'
                                << "--- rand is: " << n << endl;
                            MessageBus::instance().publish(n, std::to_string(n), n);
                            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                        } });

    auto t1 = std::thread([]()
                          {
                            CallbackItem_t item1;
                        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                        while (1)
                        {
                            item1.msgNumVec.push_back(TEST_MESSAGE1);
                            item1.callback = onTest1;
                            item1.timeoutInterval = 2000;
                            item1.timeOutCallback = timeOutCallback1;

                            item1.callbackType = CallbackType_t::ALWAYS;
                            MessageBus::instance().subscribe(item1);
                            std::cout << "send message" << std::endl;
                            std::this_thread::sleep_for(std::chrono::milliseconds(3000));
                        } });

    // CallbackItem_t item1;
    // item1.msgNumVec.push_back(TEST_MESSAGE1);
    // item1.callback = onTest1;
    // item1.timeoutInterval = 2000;
    // item1.timeOutCallback = timeOutCallback1;

    // item1.callbackType = CallbackType_t::ALWAYS;
    // MessageBus::instance().subscribe(item1);

    // CallbackItem_t item2;
    // item2.msgNumVec.push_back(TEST_MESSAGE2);
    // item2.msgNumVec.push_back(TEST_MESSAGE3);
    // item2.callback = onTest2;
    // item2.timeoutInterval = 5000;
    // item2.timeOutCallback = timeOutCallback2;
    // MessageBus::instance().subscribe(item2);

    //
    char c(' ');
    while (c != 'q' && c != 'Q')
    {
        cout << "press q" << endl;
        cin >> c;
    }

    MessageBus::instance().stop();

    return 0;
}
#include <iostream>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <dlMutex.h>
#include <dlThread.h>
#include "ThreadSafePriorityQueue.h"
#include "BadQueue.h"
#include "DeadLocks.h"

#define BOOST_TEST_MAIN test

#include <boost/test/included/unit_test.hpp>

using namespace std;

BOOST_AUTO_TEST_CASE(testWithoutDeadlock) {
    cout << "test1:" << endl;
    dlMutex m1;
    dlMutex m2;
    boost::thread t([&] {
        try {
            unique_lock<dlMutex> lock1(m1);
            this_thread::sleep_for(chrono::seconds(2));
            lock1.unlock();
            unique_lock<dlMutex> lock2(m2);
        }
        catch (exception& e) {
            cout << e.what();
        }
    });
    boost::thread t1([&] {
        try {
            unique_lock<dlMutex> lock2(m2);
            this_thread::sleep_for(chrono::milliseconds(100));
            unique_lock<dlMutex> lock1(m1);
            this_thread::sleep_for(chrono::milliseconds(100));
            lock2.unlock();
        }
        catch (exception& e) {
            cout << e.what();
        }
    });
    t.join(); t1.join();
    cout << "Deadlock was not found" << endl;
}

BOOST_AUTO_TEST_CASE(testWithDeadlock1) {
    cout << "test2:" << endl;
    dlMutex m1;
    dlMutex m2;
    boost::thread t([&] {
        try {
            unique_lock<dlMutex> lock1(m1);
            this_thread::sleep_for(chrono::milliseconds(100));
            unique_lock<dlMutex> lock2(m2);
        }
        catch (std::exception& e) {
            cout << e.what();
        }
    });
    boost::thread t1([&] {
        try {
            unique_lock<dlMutex> lock2(m2);
            this_thread::sleep_for(chrono::milliseconds(100));
            unique_lock<dlMutex> lock1(m1);
        }
        catch (std::exception& e) {
            cout << e.what();
        }
    });
    t.join(); t1.join();
}


BOOST_AUTO_TEST_CASE(testWithDeadlock2) {
    cout << "test3:" << endl;
    dlMutex m1;
    dlMutex m2;
    dlMutex m3;
    boost::thread t1([&]{
        try {
            for (int i = 0; i < 10000; ++i) {
                unique_lock<dlMutex> lock1(m1);
                unique_lock<dlMutex> lock2(m2);
                unique_lock<dlMutex> lock3(m3);
                lock3.unlock();
                lock2.unlock();
                lock1.unlock();
            }
        }
        catch (exception& e) {
            cout << e.what();
        }
    });
    boost::thread t2([&]{
        try {
            for (int i = 0; i < 10000; ++i) {
                unique_lock<dlMutex> lock2(m2);
                unique_lock<dlMutex> lock3(m3);
                unique_lock<dlMutex> lock1(m1);
                lock1.unlock();
                lock3.unlock();
                lock2.unlock();
            }
        }
        catch (exception& e) {
            cout << e.what();
        }
    });
    t1.join();
    t2.join();
}

BOOST_AUTO_TEST_CASE(GoodPriorityQueueTest) {
    cout << "test4:" << endl << "PriorityQueueWithoutDeadlocks" << endl;
    vector<shared_ptr<boost::thread>> arr;
    ThreadSafePriorityQueue<int> q;
    for (int k = 0; k < 5; ++k) {
        arr.push_back(make_shared<boost::thread>([&]{
            try {
                for (int i = 0; i < 100; ++i) {
                    q.push(i, k * 100 + i);
                }
            }
            catch (exception& e) {
                cout << e.what() << endl;
            }
        }));
    }
    for (int k = 5; k < 10; ++k) {
        arr.push_back(make_shared<boost::thread>([&]{
            try {
                for (int i = 0; i < 100; ++i) {
                    int t;
                    q.pop(t);
                }
            }
            catch (exception& e) {
                cout << e.what() << endl;
            }
        }));
    }
    for (int k = 0; k < 10; ++k) {
        arr[k]->join();
    }
    cout << "Deadlock was not found" << endl;
}


BOOST_AUTO_TEST_CASE(BadPriorityQueueTest) {
    cout << "test5:" << endl << "PriorityQueueWithDeadlocks" << endl;

    vector<shared_ptr<boost::thread>> arr;
    DeadLockQueue<int> q(100);
    for (int k = 0; k < 5; ++k) {
        arr.push_back(make_shared<boost::thread>([&]{
            try {
                for (int i = 0; i < 100; ++i) {
                    q.go(i, true);
                }
            }
            catch (exception& e) {
                cout << e.what();
            }
        }));
    }
    for (int k = 0; k < 5; ++k) {
        arr.push_back(make_shared<boost::thread>([&]{
            try {
                for (int i = 0; i < 100; ++i) {
                    q.go(99-i, false);
                }
            }
            catch (exception& e) {
                cout << e.what();
            }
        }));
    }
    for (int k = 0; k < 10; ++k) {
        arr[k]->join();
    }
}

BOOST_AUTO_TEST_CASE(BadPriorityQueueTest2) {
    cout << "test6:" << endl << "PriorityQueueWithDeadlocks2" << endl;

    vector<shared_ptr<boost::thread>> arr;
    BadQueue<int> q(100);
    for (int k = 0; k < 5; ++k) {
        arr.push_back(make_shared<boost::thread>([&]{
            try {
                for (int i = 0; i < 100; ++i) {
                    q.up(i);
                }
            }
            catch (exception& e) {
                cout << e.what();
            }
        }));
    }
    for (int k = 0; k < 5; ++k) {
        arr.push_back(make_shared<boost::thread>([&]{
            try {
                for (int i = 0; i < 100; ++i) {
                    q.down(99 - i);
                }
            }
            catch (exception& e) {
                cout << e.what();
            }
        }));
    }
    for (int k = 0; k < 10; ++k) {
        arr[k]->join();
    }
}
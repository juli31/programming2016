#include <vector>
#include <string>
#include <mutex>
#include <iostream>
#include <atomic>
#include <exception>
#include <thread>
#include <assert.h>
#include <chrono>
 
class Futex
{
public:
    Futex();
    void lock();
    void unlock();
private:
    std::atomic<int> id;
    int get_thread_id();
};
 
std::mutex io;
 
Futex::Futex() : id(0) {}
 
int Futex::get_thread_id()
{
    assert(sizeof(std::thread::id)==sizeof(int));
    auto id = std::this_thread::get_id();
    int* ptr=(int*) &id;
    return (*ptr);
}
 
void Futex::lock()
{
    int x = get_thread_id();
    int y = 0;
    while (!id.compare_exchange_strong(y, x))
    {
        y = 0;
        std::this_thread::yield();
    }
}
 
void Futex::unlock()
{
    int x = get_thread_id();
    int y = 0;
    assert(id.compare_exchange_weak(x, y));
}
 
template<class MFutex>
void inc(int & value, int & thread_value, int &required_value, MFutex & mf)
{
    while (1)
    {
        std::lock_guard<MFutex> lg(mf);
        if(value < required_value)
        {
            (value)++;
            (thread_value)++;
        }
        else break;
    }
}
 
template <class MFutex>
std::vector<int> thread_inc(int required_value, int count_threads )
{
    MFutex mf;
    int value = 0;
    std::vector<std::thread> thr(count_threads);
    std::vector<int> value_thr(count_threads , 0);
    for(int i = 0 ;  i < count_threads ; ++i)
    {
        thr[i] = std::thread(inc<MFutex> , std::ref(value)  , std::ref(value_thr[i]), std::ref(required_value), std::ref(mf));
    }
    for(int i = 0 ;  i < count_threads ; ++i) thr[i].join();
    int sum = 0;
    for(int i = 0 ;  i < count_threads ; ++i) sum += value_thr[i];
    assert(value == required_value && value == sum);
    return value_thr;
}
 
template <class MFutex>
void task(std::string task_name, int count_threads, int required_value )
{
    std::vector<int> value_thr;
    std::cout << "Task_" << task_name << std::endl;
    auto begin_time = std::chrono::system_clock::now();
    value_thr = thread_inc<MFutex> (required_value, count_threads);
    auto end_time = std::chrono::system_clock::now();
    for(int i = 0 ;  i < count_threads ; ++i)
    {
        std::cout << "value in thread_" << i << ":" << value_thr[i] << std::endl;
    }
    std::cout << "time: " << std::chrono::duration<double>(end_time - begin_time).count() << "s" << std::endl;
}
int main()
{
    int count_threads;
    int required_value;
    task<Futex>("B", 10, 1500000);
    task<Futex>("C_0", 10, 9000000);
    task<Futex>((std::string)"C_a_futex",std::thread::hardware_concurrency() / 2, 1500000);
    task<std::mutex>("C_a_mutex", std::thread::hardware_concurrency() / 2, 1500000);
    task<Futex>("C_b_futex", std::thread::hardware_concurrency() * 2, 1500000);
    task<std::mutex>("C_b_mutex", std::thread::hardware_concurrency() * 2, 1500000);
    return 0;
}

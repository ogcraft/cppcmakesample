#include <algorithm>
#include <experimental/any>
#include <array>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <iterator>
#include <mutex>
#include <thread>
#include <vector>

using namespace std;
using namespace std::experimental;

// A simple non-thread safe ring buffer implementation.
template <typename T, size_t Size>
class RingBuffer {
public:
    RingBuffer()
        : readPos{0}
        , writePos{0}
        , numFull{0}
    {
    }

    void push(T t) {
        buffer[writePos] = t;

        writePos++;
        writePos %= Size;
        numFull++;
    }

    auto pop() {
        auto task = buffer[readPos];

        readPos++;
        readPos %= Size;
        numFull--;

        return task;
    }

    auto empty() const {
        return numFull == 0;
    }

    auto full() const {
        return numFull == Size;
    }

private:
    array<T, Size> buffer;

    size_t readPos;
    size_t writePos;
    size_t numFull;
};


using function_t = function<void (any&)>;

struct task_t {
    function_t function;
    any args;
};

class Dispatcher {
public:
    Dispatcher()
        : numWorking{0}
        , finish{false}
    {
    }

    // Creates additional workers.
    void spawnWorkers(int numWorkers) {
        generate_n(back_inserter(workers), numWorkers, [&]() {
            return thread{&Dispatcher::worker, this};
        });
    }

    // Waits until the buffer is empty and all workers have joined.
    void joinWorkers() {
        syncWorkers();

        {
            lock_guard<::mutex> lock(mutex);
            finish = true;
        }

        for (auto& t : workers) {
            t.join();
        }
    }

    // Waits until the buffer is empty and all workers are idle.
    void syncWorkers() {
        unique_lock<::mutex> lock(mutex);

        consumer_ready.wait(lock, [&]() {
            return buffer.empty() && allIdle();
        });
    }

    // Adds a new task to the queue; blocks if the buffer is already full.
    void addTask(function_t function, any args) {
        unique_lock<::mutex> lock(mutex);

        consumer_ready.wait(lock, [&]() {
            return !buffer.full();
        });

        buffer.push({function, args});

        this->work_available.notify_one();
    }

private:
    RingBuffer<task_t, 8> buffer;
    ::mutex mutex;

    condition_variable work_available;
    condition_variable consumer_ready;

    int numWorking;
    bool finish;

    vector<thread> workers;

    // True if all workers are idle.
    bool allIdle() const {
        return numWorking == 0;
    }

    void worker() {
        while (true) {
            task_t task;

            // Wait for work and fetch it.
            {
                unique_lock<::mutex> lock(mutex);

                while (buffer.empty()) {
                    if (work_available.wait_for(lock, 10ms) == cv_status::timeout) {
                        // Check every 10ms whether we should exit.
                        if (finish) {
                            return;
                        }
                    }
                }

                task = buffer.pop();
                numWorking++;
            }

            // Work.
            task.function(task.args);

            // Tell the dispatcher we're ready.
            {
                unique_lock<::mutex> lock(mutex);

                numWorking--;
                consumer_ready.notify_one();
            }
        }
    }
};


struct args1_t {
    int addUpTo;
};

struct args2_t {
    int multUpTo;
};

// Some job to do.
void firstThing(any& a) {
    // Work on args and return.
    args1_t args = any_cast<args1_t>(a);

    int sum = 0;

    for (auto i = 0; i < args.addUpTo; ++i) {
        sum += i;
    }

    cout << "Sum: " << sum << "\n";
}

// Some job to do.
void secondThing(any& a) {
    // Work on args and return.
    args2_t args = any_cast<args2_t>(a);

    int prod = 1;

    for (auto i = 1; i < args.multUpTo; ++i) {
        prod *= i;
    }

    cout << "Prod: " << prod << "\n";
}

int main() {

    cout << "Starting main() on " << std::thread::hardware_concurrency() << " cores" << "\n";

    Dispatcher disp;

    disp.spawnWorkers(4);

    for (int i = 0; i < 20; ++i) {
        disp.addTask(firstThing, args1_t{10 * i});
    }

    disp.syncWorkers();

    for (int i = 0; i < 10; ++i) {
        disp.addTask(secondThing, args2_t{i});
    }

    disp.joinWorkers();
}

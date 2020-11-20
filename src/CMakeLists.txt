set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

add_executable(producer_consumer producer_consumer.cpp)
target_link_libraries(producer_consumer Threads::Threads)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

add_executable(producer_consumer producer_consumer.cpp)
target_compile_features(producer_consumer PUBLIC cxx_std_17)
target_link_libraries(producer_consumer Threads::Threads)

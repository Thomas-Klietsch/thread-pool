### Introduction

Thread pool, in a single header file, with fallback to single thread mode.

### Usage

Further examples are included in the file 'main.cpp'.

Create a thread pool with 4 threads (workers):

    #include "./thread/pool.hpp"
	Thread::Pool thread_pool( 4 );

The use of the pool can either be "manual" or a range method.

#### Manual

Add tasks to the pool:

    for ( std::uint32_t i{ start }; end <= 100; ++i )
	    thread_pool.add_task( /* task function code */ );

Process and wait for all tasks to be completed (without this, the thread_pool will wait for new tasks forever):

    thread_pool.wait();

#### Range

Thread::Pool offers three methods to split an (positive) integer range into sub ranges, spread across the threads/workers. Methods use both of the start and end range integers.

Method 1: .for_each( start, end, /* task function code */ )
This is basically the same as the manual method, with an integer (from the range) passed to the task.

Method 2: .chunk( start, end, /* task function code */ )
Splits the range into equally sized sub ranges. However not at workers might get used.

Method 3: .sequence( start, end, /* task function code */ )
All workers gets a sub range, but sub range sizes may vary.

See Note below for difference between method 2 and 3.

#### Refresh

After the above is completed (Manual or Range), the thread_pool switches to single thread mode. If the thread_pool is used again, it can be made multi threaded with (four threads):

    thread_pool.refresh( 4 );

### Dependencies

- C++17

### Note

The difference in behaviour between .chunk() and .sequence(), depends on the range and threads.

If the range is an integer multiple of threads, there is no difference. For example range of 1-12 with 4 threads.

Example with different amount of workers used, thread pool with 4 workers and range of 1-9:

Chunk will be split into 3 workers, sub ranges: [1 3], [4 6], [7 9]

Sequence will use all threads, sub ranges: [1 3], [4 5], [6 7], [8 9]

Example with equal amount of workers used, thread pool with 4 workers and range of 1-10:

Chunk will be split into 3 workers, sub ranges: [1 3], [4 6], [7 9], [10 10]

Sequence will use all threads, sub ranges: [1 3], [4 5], [6 8], [9 10]

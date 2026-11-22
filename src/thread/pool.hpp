// Copyright (c) 2026 Thomas Klietsch, all rights reserved.
//
// Licensed under the GNU Lesser General Public License, version 3.0 or later
//
// This program is free software: you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation, either version 3 of
// the License, or ( at your option ) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General
// Public License along with this program.If not, see < https://www.gnu.org/licenses/>.

#pragma once

#include <algorithm>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace Thread {

	class Pool {

	private:

		// One worker per thread
		std::vector<std::thread> worker{};

		// Queue of tasks to be performed
		std::queue<std::function<void()>> queue{};

		// https://en.cppreference.com/w/cpp/thread/condition_variable/
		// std::condition_variable is a synchronization primitive used with a std::mutex
		// to block one or more threads until another thread both modifies a shared variable
		// (the condition) and notifies the std::condition_variable.
		std::condition_variable condition;

		// If true, workers stop getting tasks from the queue
		bool f_stop{ false };

		// Lock access and allow only one thread to work on internal data
		std::mutex mutex;

	public:

		// Create pool in single thread mode
		Pool() {};

		// Create pool in multithread mode (if request>1)
		Pool(
			std::uint32_t const request
		) {
			CreateWorkers( request );
		};

		~Pool() {
			// Single thread mode
			if ( worker.size() < 2u )
				return;

			{
				std::unique_lock<std::mutex> lock( mutex );
				condition.wait( lock,
					[this]
					{
						return queue.empty();
					} );
			}
			Stop();
		};

		// Use: thread_pool->add_task([] { code });
		// Add a task to the queue
		void add_task(
			std::function<void()> const& task
		) {
			if ( worker.size() < 2u ) {
				// Single thread mode
				task();
				return;
			}

			{
				std::unique_lock<std::mutex> lock( mutex );
				if ( !f_stop )
					queue.push( task );
			}
			condition.notify_one();
		};

		// Wait for all tasks to be completed.
		// Will turn pool into single thread mode when completed.
		void wait() {
			// Single thread mode
			if ( worker.size() < 2u )
				return;

			while ( is_busy() ) {
			};

			{
				std::unique_lock<std::mutex> lock( mutex );
				condition.wait( lock, [this]
					{
						return queue.empty();
					} );
			}
			Stop();
		};

		// Parallel processing of a range of values,
		// The range is split into one value per task.
		// The task lambda needs an integer argument:
		//   [&]( std::uint32_t A )
		template <typename T>
		void for_each(
			// Index of first element to process
			std::uint32_t begin,
			// Index of last element to process
			std::uint32_t end,
			// The task to perform
			T const& loop_task
		) {
			if ( begin > end )
				std::swap( begin, end );

			// Generate tasks
			for ( std::uint32_t i{ begin }; i <= end; ++i ) {
				add_task( [=]()
					{
						loop_task( i );
					} );
			}

			wait();
		};

		// Parallel processing of a range of values,
		// The range is split into two indices per task.
		// The task lambda needs two integer arguments:
		//   [&]( std::uint32_t first, std::uint32_t last )
		// Priority is equal work size (sub range) per task,
		//   all threads might not be used.
		template <typename T>
		void chunk(
			// Index of first element to process
			std::uint32_t begin,
			// Index of last element to process
			std::uint32_t end,
			// The task to perform
			T const& loop_task
		) {
			if ( begin > end )
				std::swap( begin, end );

			// Single thread mode
			if ( worker.size() < 2u )
				return loop_task( begin, end );

			std::uint32_t const chunk_size = ( end - begin ) / worker.size();

			// Generate tasks
			for ( std::uint32_t i{ begin }; i <= end; i += chunk_size + 1u ) {
				// Handle end state
				std::uint32_t const next = std::min( end, i + chunk_size );

				add_task( [=]()
					{
						loop_task( i, next );
					} );
			}

			wait();
		};

		// Parallel processing of a range of values,
		// The range is split into two indices per task.
		// The task lambda needs two integer arguments:
		//   [&]( std::uint32_t first, std::uint32_t last )
		// Priority is to use all threads,
		//   at least one work size (sub range) per task.
		template <typename T>
		void sequence(
			// Index of first element to process
			std::uint32_t begin,
			// Index of last element to process
			std::uint32_t end,
			// The task to perform
			T const& loop_task
		) {
			if ( begin > end )
				std::swap( begin, end );

			// Single thread mode
			if ( worker.size() < 2u )
				return loop_task( begin, end );

			// The range is split into intervals
			std::vector<std::uint32_t> interval( worker.size() + 1u );

			// Calculate the width of an interval.
			// +1u is account for the use of end interval - 1
			std::float_t const delta = static_cast<std::float_t>( end - begin + 1u ) / ( worker.size() );

			// Calculate the interval values
			for ( std::uint32_t i{ 0 }; i < worker.size() + 1u; ++i )
				interval[i] = std::ceil( i * delta ) + begin;

			// If range is smaller than workers, remove duplicate intervals
			std::vector<std::uint32_t>::iterator iterator = std::unique( interval.begin(), interval.end() );
			interval.resize( std::distance( interval.begin(), iterator ) );

			// Generate tasks
			for ( std::uint32_t i{ 0 }; i < interval.size() - 1u; ++i )
				add_task( [=]()
					{
						// Subtract one, otherwise workload would overlap
						loop_task( interval[i], interval[i + 1u] - 1u );
					} );

			wait();
		};

		// Create new threads/workers,
		// if the current thread pool is in single thread mode,
		void refresh(
			std::uint32_t const threads_request
		) {
			CreateWorkers( threads_request );
		};

	private:

		// Returns true, if there are task(s) in the queue
		bool is_busy() {
			std::unique_lock<std::mutex> lock( mutex );
			return !queue.empty();
		};

		void Stop() {
			{
				std::unique_lock<std::mutex> lock( mutex );
				f_stop = true;
			}
			condition.notify_all();

			for ( std::thread& thread : worker )
				thread.join();

			worker.clear();
		};

		// Create worker(s)
		void CreateWorkers(
			std::uint32_t const threads_request
		) {
			// Work queue is not empty, or a queue is already defined
			if ( is_busy() || worker.size() )
				return;

			// Clamp to a sensible thread count
			std::uint32_t const n_thread = std::clamp( threads_request, 1u, std::thread::hardware_concurrency() );

			// Same as single threading
			if ( n_thread < 2u )
				return;

			// Check if safe to create workers
			if ( mutex.try_lock() ) {
				worker.reserve( n_thread );

				f_stop = false;

				for ( std::uint32_t i{ 0u }; i < n_thread; ++i )
					worker.emplace_back(
						// Capture
						[this]
						{
							// Worker code start
							while ( true ) {
								std::function<void()> task;

								{
									std::unique_lock<std::mutex> lock( this->mutex );
									this->condition.wait( lock, [this]
										{
											return !this->queue.empty() || this->f_stop;
										} );

									if ( this->f_stop && this->queue.empty() )
										return;

									// Grab a task, and perform it
									task = std::move( this->queue.front() ); // Take ownership
									this->queue.pop(); // Remove from queue
								}

								task();
							}
							// Worker code end
						}
					); // end emplace_back

				// .try_lock() does not auto unlock at end of scope
				mutex.unlock();
			} // end try_lock
		};
	};

};

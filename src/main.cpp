#include <bits/chrono.h>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <mutex>

#include "./thread/pool.hpp"

// The use of the u suffix is probably pedantic.

void AddTask() {
	std::cout << "Example: .add_task()\n\n";

	std::cout << "Create thread pool with 4 threads\n";
	Thread::Pool thread_pool( 4u );
	std::mutex lock;

	for ( std::uint32_t i{ 1u }; i < 10u; ++i )
		thread_pool.add_task(
			// Lambda, with capture
			[&, i]()
			{
				// Task code
				{
					std::unique_lock<std::mutex> lock_guard( lock );
					std::cout << "First batch task: " << i << '\n';
				}
				// Simulated work load
				std::this_thread::sleep_for( std::chrono::milliseconds( 15u ) );
				// end of task code
			}
		); // end of .add_task

	std::cout << "Add different tasks to the same pool (possible async text output)\n";

	// More tasks can be added
	for ( std::uint32_t i{ 1u }; i < 10u; ++i )
		thread_pool.add_task(
			[&, i, a = 5u]()
			{
				{
					std::unique_lock<std::mutex> lock_guard( lock );
					std::cout << "Second batch task: " << i * a << '\n';
				}
				std::this_thread::sleep_for( std::chrono::milliseconds( 15u ) );
			}
		);

	// Waits for all tasks to be completed (required!)
	thread_pool.wait();
	// thread_pool is now in single thread mode

	std::cout << "End of example.\n\n";
};

void Refresh() {
	std::cout << "Example: .refresh()\n\n";

	std::cout << "Create thread pool with 4 threads\n";
	Thread::Pool thread_pool( 4u );
	std::mutex lock;

	for ( std::uint32_t i{ 1u }; i < 10u; ++i )
		thread_pool.add_task(
			[&, i]()
			{
				{
					std::unique_lock<std::mutex> lock_guard( lock );
					std::cout << "thread_pool, add task: " << i << '\n';
				}
				std::this_thread::sleep_for( std::chrono::milliseconds( 15u ) );
			}
		);

	thread_pool.wait();
	// thread_pool is now in single thread mode

	std::cout << "Refresh thread pool with 8 threads\n";
	// Refresh the thread_pool with threads
	thread_pool.refresh( 8u );

	for ( std::uint32_t i{ 1u }; i < 10u; ++i )
		thread_pool.add_task(
			[&, i]()
			{
				{
					std::unique_lock<std::mutex> lock_guard( lock );
					std::cout << "Refreshed thread_pool, add task: " << i << '\n';
				}
				std::this_thread::sleep_for( std::chrono::milliseconds( 15u ) );
			}
		);

	thread_pool.wait();
	// thread_pool is now in single thread mode

	std::cout << "End of example.\n\n";
};

void ForEach() {
	std::cout << "Example: .for_each(), 4 threads, range 1-9\n\n";

	std::mutex lock;
	Thread::Pool thread_pool( 4u );
	thread_pool.for_each( 1u, 9u,
		[&]( std::uint32_t iteration )
		{
			{
				std::unique_lock<std::mutex> lock_guard( lock );
				std::cout << "for_each: " << iteration << '\n';
			}
			std::this_thread::sleep_for( std::chrono::milliseconds( 15u ) );
		}
	);
	// thread_pool is now in single thread mode

	std::cout << "End of example.\n\n";
};

void Chunk() {
	std::cout << "Example: .chunk(), 4 threads, range 1-9\n\n";

	std::mutex lock;
	Thread::Pool thread_pool( 4u );
	thread_pool.chunk( 1u, 9u,
		[&]( std::uint32_t begin, std::uint32_t end )
		{
			{
				std::unique_lock<std::mutex> lock_guard( lock );
				std::cout << "chunk: " << begin << " " << end << '\n';
			}
			std::this_thread::sleep_for( std::chrono::milliseconds( 15u * ( end - begin + 1u ) ) );
		}
	);
	// thread_pool is now in single thread mode

	std::cout << "End of example.\n\n";
};

void Sequence() {
	std::cout << "Example: .sequence(), 4 threads, range 1-9\n\n";

	std::mutex lock;
	Thread::Pool thread_pool( 4u );
	thread_pool.sequence( 1u, 9u,
		[&]( std::uint32_t begin, std::uint32_t end )
		{
			{
				std::unique_lock<std::mutex> lock_guard( lock );
				std::cout << "sequence: " << begin << " " << end << '\n';
			}
			std::this_thread::sleep_for( std::chrono::milliseconds( 15u * ( end - begin + 1u ) ) );
		}
	);
	// thread_pool is now in single thread mode

	std::cout << "End of example.\n\n";
};

void TileRender() {
	std::cout << "Example: tile render\n\n";

	struct Image {
		// Dummy example data
		std::vector<std::uint32_t> pixel;
		Image(
			std::uint32_t size
		) {
			pixel = std::vector<std::uint32_t>( size, 0u );
		};
	};

	struct Tile {
		// Dummy example data
		std::uint32_t i{ 0u };
		Tile(
			std::uint32_t i
		)
			: i( i ) {};
	};

	class TileIntegrator {

	public:

		TileIntegrator() {};

		void process(
			Tile const& tile,
			Image& image
		) {
			// Do stuff with tile data and store in image
			// "Random" value
			std::uint32_t const i = ( ( tile.i * 313u + 571u ) >> 6u ) % 32u + 1u;
			// _IF_ tiles do not overlap, it is safe to write directly to image
			image.pixel[tile.i] = i;
		};
	};

	// Example uses dummy load of 32 tiles/pixels
	std::uint32_t n_pixel{ 32u };
	// Rendered image
	Image image( n_pixel );
	// Define all tiles.
	std::vector<Tile> tile;
	for ( std::uint32_t i{ 0u }; i < n_pixel; ++i )
		tile.push_back( Tile( i ) );

	Thread::Pool thread_pool( 4u );

	// For example path-tracing from camera
	for ( std::uint32_t i{ 0u }; i < tile.size(); ++i ) {
		thread_pool.add_task(
			[&, i]()
			{
				std::unique_ptr<TileIntegrator> p_integrator = std::make_unique<TileIntegrator>( /* arguments */ );
				p_integrator->process( tile[i], image );
			}
		);
	}
	// Wait for all tiles to be processed
	thread_pool.wait();

	// Save image
	std::cout << "Tile image: ";
	for ( auto pixel : image.pixel )
		std::cout << pixel << ' ';
	std::cout << std::endl;

	std::cout << "End of example.\n\n";
};

void IterationRender() {
	std::cout << "Example: iteration render\n\n";

	struct Image {
		// Dummy example data
		std::vector<std::uint32_t> pixel;
		Image(
			std::uint32_t size
		) {
			pixel = std::vector<std::uint32_t>( size, 0u );
		};

		std::mutex lock;

		void add_to_pixel(
			std::uint32_t const index,
			std::uint32_t const value
		) {
			// Out of bounds
			if ( index >= pixel.size() )
				return;

			std::unique_lock<std::mutex> lock_guard( lock );
			pixel[index] += value;
		};
	};

	class IterationIntegrator {

	public:

		IterationIntegrator() {};

		void process(
			std::uint32_t const& iteration,
			Image& image
		) {
			// Do stuff with tile data and store in image
			// "Random" pixel
			std::uint32_t const i = ( ( iteration * 313u + 571u ) >> 6u ) % 32u;
			image.add_to_pixel( i, 1u );
		};
	};

	// Rendered image
	Image image( 32u );

	Thread::Pool thread_pool( 4u );

	// 200 samples, random pixel write (f.ex. light-tracing)
	for ( std::uint32_t i{ 0u }; i < 200u; ++i ) {
		thread_pool.add_task(
			[&, i]()
			{
				std::unique_ptr<IterationIntegrator> p_integrator = std::make_unique<IterationIntegrator>( /* arguments */ );
				p_integrator->process( i, image );
			}
		);
	}

	// Wait for all tiles to be processed
	thread_pool.wait();

	// Save image
	std::cout << "Iteration image: ";
	for ( auto pixel : image.pixel )
		std::cout << pixel << ' ';
	std::cout << std::endl;

	std::cout << "End of example.\n\n";
};

void SingleThread() {
	std::cout << "Example: single thread mode\n\n";

	// Creating Thread::Pool in single thread mode.
	Thread::Pool thread_pool1;
	Thread::Pool thread_pool2{};
	Thread::Pool thread_pool3( 0u );
	Thread::Pool thread_pool4( 1u );
	// If CPU is single core, single thread (is it even possible to find one anymore?).
	if ( std::thread::hardware_concurrency() == 1 )
		Thread::Pool thread_pool5( 40'000u );

	// Refresh which leave the Thread::Pool unchanged in single thread mode.
	thread_pool1.refresh( 0u );
	thread_pool1.refresh( 1u );

	std::mutex lock;

	for ( std::uint32_t i{ 1u }; i < 10u; ++i ) {
		thread_pool1.add_task(
			[&, i]()
			{
				{
					std::unique_lock<std::mutex> lock_guard( lock );
					std::cout << "Single thread mode i: " << i << '\n';
				}
				std::this_thread::sleep_for( std::chrono::milliseconds( 10u ) );
			}
		);
	}

	std::cout << "End of example.\n\n";
};

void InvalidCapture() {
	std::cout << "Example: invalid capture\n\n";
	Thread::Pool thread_pool( 2u );
	std::mutex lock;

	for ( std::uint32_t i{ 1u }; i <= 9u; ++i ) {
		thread_pool.add_task(
			// Valid capture of i
			// [&, i]
			//
			// Invalid capture, as a reference,
			//   which also becomes undefined behaviour outside the for loop.
			[&]()
			{
				{
					std::unique_lock<std::mutex> lock_guard( lock );
					std::cout << "Invalid capture of i: " << i << '\n';
				}
				std::this_thread::sleep_for( std::chrono::milliseconds( 100u ) );
			}
		);
	}

	thread_pool.wait();
	// thread_pool is now in single thread mode

	std::cout << "End of example.\n\n";
};

int main(
	[[maybe_unused]] int argc,
	[[maybe_unused]] char* argv[]
) {
	std::cout << "Maximum number of threads: " << std::thread::hardware_concurrency() << "\n\n";

	AddTask();

	Refresh();

	ForEach();

	Chunk();

	Sequence();

	TileRender();

	IterationRender();

	SingleThread();

	InvalidCapture();

	return EXIT_SUCCESS;
};

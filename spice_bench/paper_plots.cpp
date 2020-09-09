#include <benchmark/benchmark.h>

#include <spice_bench/exp_range.h>

#include <spice/cuda/algorithm.h>
#include <spice/cuda/multi_snn.h>
#include <spice/cuda/util/dbuffer.h>
#include <spice/cuda/util/event.h>
#include <spice/models/brunel.h>
#include <spice/models/brunel_with_plasticity.h>
#include <spice/models/synth.h>
#include <spice/models/vogels_abbott.h>
#include <spice/util/adj_list.h>
#include <spice/util/type_traits.h>

#include <chrono>

using namespace spice;
using namespace spice::cuda;
using namespace spice::cuda::util;
using namespace spice::util;

using namespace std::chrono;


class timer
{
	time_point<high_resolution_clock> s;

public:
	timer() { s = high_resolution_clock::now(); }
	double time()
	{
		return duration_cast<microseconds>( high_resolution_clock::now() - s ).count() * 1e-6;
	}
};


static void gen( benchmark::State & state )
{
	size_ const NSYN = state.range( 0 );
	size_ const N = narrow_cast<size_>( std::sqrt( NSYN / 0.1 ) );

	state.counters["num_neurons"] = narrow_cast<double>( N );
	state.counters["num_syn"] = narrow_cast<double>( NSYN );

	try
	{
		layout desc( { N }, { { 0, 0, 0.1f } } );

		std::vector<int> l;
		adj_list::generate( desc, l );

		for( auto _ : state ) adj_list::generate( desc, l );
	}
	catch( std::exception & e )
	{
		std::printf( "%s\n", e.what() );
		return;
	}

	state.SetBytesProcessed( ( N * 4 ) * state.iterations() );
}
BENCHMARK( gen )->Unit( benchmark::kMillisecond )->ExpRange( 1'000'000, 2048'000'000 );

static void plot0_AdjGen( benchmark::State & state )
{
	float const P = 0.1f;
	size_ const NSYN = state.range( 0 );
	size_ const N = narrow_cast<size_>( std::sqrt( NSYN / P ) );

	state.counters["num_neurons"] = narrow_cast<double>( N );
	state.counters["num_syn"] = narrow_cast<double>( NSYN );

	try
	{
		layout desc( { N }, { { 0, 0, P } } );

		dbuffer<int> e( desc.size() * desc.max_degree() );

		time_event start, stop;
		for( auto _ : state )
		{
			generate_rnd_adj_list( desc, e.data() );
			start.record();
			for( int_ i = 0; i < 10; i++ ) generate_rnd_adj_list( desc, e.data() );
			// cudaMemsetAsync( e.data(), 0, 4 * NSYN );
			stop.record();
			stop.synchronize();

			state.SetIterationTime( stop.elapsed_time( start ) / 10 );
		}
	}
	catch( std::exception & e )
	{
		std::printf( "%s\n", e.what() );
		return;
	}

	state.SetBytesProcessed( ( NSYN * 4 ) * state.iterations() );
}
BENCHMARK( plot0_AdjGen )
    ->UseManualTime()
    ->Unit( benchmark::kMillisecond )
    ->ExpRange( 1'000'000, 2048'000'000 );


// Absolute runtime per iteration as a function of synapse count
template <typename Model>
static void plot2_RunTime( benchmark::State & state )
{
	size_ NSYN = state.range( 0 );

	uint_ n = 1000;
#if 0
	using net_t = cuda::snn<Model>;
#else
	using net_t = cuda::multi_snn<Model>;
	NSYN *= 2;
	n *= device::devices().size();
#endif

	float const P = 0.02f;
	size_ const N = narrow_cast<size_>( std::sqrt( NSYN / P ) );
	size_ const ITER = 1000;

	state.counters["num_neurons"] = narrow_cast<double>( N );
	state.counters["num_syn"] = narrow_cast<double>( NSYN );

	try
	{
		net_t net( { n, 0.0f }, 0.0001f, 8 );

		for( auto _ : state )
		{
			timer t;
			for( size_ i = 0; i < ITER / net.delay(); i++ ) net.step();

			if constexpr( std::is_same<net_t, cuda::multi_snn<Model>>::value )
				net.sync();
			else
				cudaDeviceSynchronize();

			state.SetIterationTime( t.time() / ITER );
		}
	}
	catch( std::exception & e )
	{
		std::printf( "%s\n", e.what() );
		return;
	}
}
BENCHMARK_TEMPLATE( plot2_RunTime, synth )
    ->UseManualTime()
    ->Unit( benchmark::kMicrosecond )
    ->ExpRange( 768'000'000, 768'000'000 );
/*BENCHMARK_TEMPLATE( plot2_RunTime, synth )
    ->UseManualTime()
    ->Unit( benchmark::kMicrosecond )
    ->ExpRange( 512'000'000, 512'000'000 );*/
/*BENCHMARK_TEMPLATE( plot2_RunTime, brunel_with_plasticity )
        ->UseManualTime()
        ->Unit( benchmark::kMicrosecond )
        ->ExpRange( 1'000'000, 16'000'000 );*/
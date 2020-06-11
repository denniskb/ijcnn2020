#include <benchmark/benchmark.h>

#include <spice/cuda/snn.h>
#include <spice/cuda/util/event.h>
#include <spice/models/synth.h>
#include <spice/models/vogels_abbott.h>
#include <spice_bench/exp_range.h>


using namespace spice;
using namespace spice::cuda;
using namespace spice::cuda::util;
using namespace spice::util;


static void cache_synth( benchmark::State & state )
{
	float const P = 0.001f;
	std::size_t const NSYN = state.range( 0 );
	std::size_t const N = narrow_cast<std::size_t>( std::sqrt( NSYN / P ) );

	state.counters["num_neurons"] = narrow_cast<double>( N );
	state.counters["num_syn"] = narrow_cast<double>( NSYN );

	try
	{
		cuda::snn<synth> net( {{N}, {{0, 0, P}}}, 1, 1 );

		std::vector<int> spikes;
		net.step( &spikes );
		state.counters["spikes_per_n"] = narrow_cast<double>( spikes.size() * P );

		event start, stop;
		for( auto _ : state )
		{
			start.record();
			for( int i = 0; i < 100; i++ )
				net.step();
			stop.record();
			stop.synchronize();

			state.SetIterationTime( stop.elapsed_time( start ) * 0.001 / 100 );
		}
	}
	catch( std::exception & e )
	{
		std::printf( "%s\n", e.what() );
		return;
	}
}

BENCHMARK( cache_synth )
    ->UseManualTime()
    ->Unit( benchmark::kMicrosecond )
    ->DenseRange( 100'000'000, 1'000'000'000, 100'000'000 );

static void cache_vogels( benchmark::State & state )
{
	float const P = 0.02f;
	std::size_t const NSYN = state.range( 0 );
	std::size_t const N = narrow_cast<std::size_t>( std::sqrt( NSYN / P ) );

	state.counters["num_neurons"] = narrow_cast<double>( N );
	state.counters["num_syn"] = narrow_cast<double>( NSYN );

	try
	{
		cuda::snn<vogels_abbott> net( {{N}, {{0, 0, P}}}, 1, 1 );

		std::vector<int> spikes;
		net.step( &spikes );
		state.counters["spikes_per_n"] = narrow_cast<double>( spikes.size() * P );

		event start, stop;
		for( auto _ : state )
		{
			start.record();
			for( int i = 0; i < 100; i++ )
				net.step();
			stop.record();
			stop.synchronize();

			state.SetIterationTime( stop.elapsed_time( start ) * 0.001 / 100 );
		}
	}
	catch( std::exception & e )
	{
		std::printf( "%s\n", e.what() );
		return;
	}
}
BENCHMARK( cache_vogels )
    ->UseManualTime()
    ->Unit( benchmark::kMicrosecond )
    ->DenseRange( 100'000'000, 1'000'000'000, 100'000'000 );
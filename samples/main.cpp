#include "../spice/cuda/snn.h"
#include "../spice/models/brunel.h"
#include "../spice/models/brunel_with_plasticity.h"
#include "../spice/models/vogels_abbott.h"

#include <fstream>
#include <iostream>


using namespace spice;
using namespace spice::util;


// Usage: samples.exe {brunel|brunel+|vogels} [neuron-count] [iter-count] [out-file]
int main( int const argc, char const ** argv )
{
	if( argc != 5 )
	{
		std::printf(
		    "Usage: spice_bench.exe {brunel|brunel+|vogels} [neuron-count] [iter-count] "
		    "[out-file]" );
		return EXIT_FAILURE;
	}

	unsigned const NNEURON = atoi( argv[2] );
	unsigned const NITER = atoi( argv[3] );

	// Lambda taking a snn and simulating it for NITER steps.
	// Neuron activations get written out to [out-file].
	auto run_sim = [=]( auto && net ) {
		std::ofstream file( argv[4] );
		file << net.num_neurons() << std::endl;

		std::size_t avgspikes = 0;
		std::vector<int> n;
		for( std::size_t i = 0; i < NITER; i++ )
		{
			// Advance snn by a single simulation step, store activations in 'n'.
			net.step( &n );
			avgspikes += n.size();

			// Ommit simulation steps without activations from output.
			// if( n.size() > 0 )
			{
				std::sort( n.begin(), n.end() );

				auto delim = "";
				for( int x : n )
				{
					file << delim << x;
					delim = ",";
				}
				file << std::endl;
			}

			if( i % 100 == 99 )
				std::cout << "\r" << 100 * ( i + 1 ) / NITER << "% done";
		}

		std::cout << "\nAvg. ratio of neurons firing: "
		          << (double)avgspikes / NITER / net.num_neurons() * 100 << "%";
	};

	try
	{
		// Initialize a snn with the brunel model
		if( !strcmp( argv[1], "brunel" ) )
			run_sim( cuda::snn<brunel>(
			    neuron_group( {NNEURON / 2, NNEURON / 2}, {{0, 1, 0.1f}, {1, 1, 0.1f}} ),
			    0.0001f,
			    15 ) );
		// Initialize a snn with the brunel model (with plasticity turned on)
		else if( !strcmp( argv[1], "brunel+" ) )
			run_sim( cuda::snn<brunel_with_plasticity>(
			    neuron_group( {NNEURON / 2, NNEURON / 2}, {{0, 1, 0.1f}, {1, 1, 0.1f}} ),
			    0.0001f,
			    15 ) );
		// Initialize a snn with the vogels&abbott model
		else if( !strcmp( argv[1], "vogels" ) )
			run_sim( cuda::snn<vogels_abbott>( NNEURON, 0.02f, 0.0001f, 8 ) );
	}
	catch( std::exception & e )
	{
		printf( "%s\n", e.what() );
		return EXIT_FAILURE;
	}

	return 0;
}
#include <spice/util/layout.h>

#include <spice/cuda/util/defs.h>
#include <spice/util/assert.h>
#include <spice/util/type_traits.h>

#include <cmath>
#include <numeric>


static constexpr std::size_t operator"" _sz( std::size_t n ) { return n; }

static std::size_t estimate_max_deg( std::vector<spice::util::layout::edge> const & connections )
{
	using namespace spice::util;

	std::size_t src = connections.empty() ? 0 : std::get<0>( connections.front() );

	std::size_t result = 0;
	double m = 0.0, s2 = 0.0;
	for( auto c : connections )
	{
		if( std::get<0>( c ) != src )
		{
			result = std::max( result, narrow_cast<std::size_t>( m + 3 * std::sqrt( s2 ) ) );
			src = std::get<0>( c );
			m = 0.0;
			s2 = 0.0;
		}

		auto const dst_range = static_cast<double>( std::get<3>( c ) - std::get<2>( c ) );
		m += dst_range * std::get<4>( c );
		s2 += dst_range * std::get<4>( c ) * ( 1.0 - std::get<4>( c ) );
	}

	return ( std::max( result, narrow_cast<std::size_t>( m + 3 * std::sqrt( s2 ) ) ) + WARP_SZ -
	         1 ) /
	       WARP_SZ * WARP_SZ;
}

static std::vector<std::tuple<std::size_t, std::size_t, float>> p2connect( float p )
{
	if( p )
		return { { 0, 0, p } };
	else
		return {};
}


namespace spice::util
{
layout::layout( std::size_t const num_neurons, float const connections )
    : layout( { num_neurons }, p2connect( connections ) )
{
	spice_assert( num_neurons > 0, "layout must contain at least 1 neuron" );
}

#pragma warning( push )
#pragma warning( disable : 4189 4457 ) // unreferenced variable 'gs' in assert, hidden variable
layout::layout(
    std::vector<std::size_t> const & pops,
    std::vector<std::tuple<std::size_t, std::size_t, float>> connections )
{
	spice_assert( pops.size() > 0, "layout must contain at least 1 (non-empty) population" );

	// Validate
	for( auto pop : pops )
		spice_assert(
		    pop > 0 && pop <= std::numeric_limits<int>::max(), "invalid population size" );

	for( auto c : connections )
	{
		spice_assert(
		    std::get<0>( c ) < pops.size() && std::get<1>( c ) < pops.size(),
		    "invalid index in connections matrix" );
		spice_assert(
		    std::get<2>( c ) > 0.0f && std::get<2>( c ) <= 1.0f, "invalid connect. prob." );
	}

	{
		auto const cmp = []( std::tuple<std::size_t, std::size_t, float> const & a,
		                     std::tuple<std::size_t, std::size_t, float> const & b ) {
			return std::get<0>( a ) < std::get<0>( b ) ||
			       std::get<0>( a ) == std::get<0>( b ) && std::get<1>( a ) < std::get<1>( b );
		};
		std::sort( connections.begin(), connections.end(), cmp );

		for( std::size_t i = 1; i < connections.size(); i++ )
		{
			spice_assert(
			    std::get<0>( connections[i] ) != std::get<0>( connections[i - 1] ) ||
			        std::get<1>( connections[i] ) != std::get<1>( connections[i - 1] ),
			    "no duplicate edges in connections list allowed" );
		}

		// Initialize
		auto const first = [&]( std::size_t i ) {
			spice_assert( i < pops.size(), "index out of range" );
			return std::accumulate( pops.begin(), pops.begin() + i, 0_sz );
		};
		auto const last = [&]( std::size_t i ) {
			spice_assert( i < pops.size(), "index out of range" );
			return std::accumulate( pops.begin(), pops.begin() + i + 1, 0_sz );
		};

		_n = std::accumulate( pops.begin(), pops.end(), 0_sz );

		for( auto c : connections )
		{
			_connections.push_back(
			    { narrow_int<int>( first( std::get<0>( c ) ) ),
			      narrow_int<int>( last( std::get<0>( c ) ) ),
			      narrow_int<int>( first( std::get<1>( c ) ) ),
			      narrow_int<int>( last( std::get<1>( c ) ) ),
			      std::get<2>( c ) } );
		}

		_max_degree = estimate_max_deg( _connections );
	}
}
#pragma warning( pop )

std::size_t layout::size() const { return _n; }
nonstd::span<layout::edge const> layout::connections() const { return _connections; }
std::size_t layout::max_degree() const { return _max_degree; }

layout::layout( std::size_t n, std::vector<edge> flat )
    : _n( n )
    , _connections( flat )
    , _max_degree( estimate_max_deg( flat ) )
{
}
} // namespace spice::util

#pragma once

#include <spice/util/assert.h>
#include <spice/util/host_defines.h>
#include <spice/util/stdint.h>
#include <spice/util/type_traits.h>

#include <cmath>


namespace spice
{
namespace util
{
HYBRID inline uint_ rotl32( uint_ x, uint_ k ) { return ( x << k ) | ( x >> ( 32 - k ) ); }
HYBRID
inline ulong_ rotl64( ulong_ x, ulong_ k ) { return ( x << k ) | ( x >> ( 64 - k ) ); }
HYBRID inline ulong_ hash( ulong_ x )
{
	spice_assert( x > 0 );

	x = ( x ^ ( x >> 30 ) ) * 0xbf58476d1ce4e5b9llu;
	x = ( x ^ ( x >> 27 ) ) * 0x94d049bb133111ebllu;
	x = x ^ ( x >> 31 );

	return x;
}
#ifndef __CUDA_ARCH__
inline float sinpif( float x ) { return std::sin( 3.14159265359f * x ); }
#endif

// http://prng.di.unimi.it/xoshiro128plus.c
class xoroshiro128p
{
public:
	using result_type = uint_;
	constexpr uint_ min() { return 0; }
	constexpr uint_ max() { return std::numeric_limits<uint_>::max(); }

	HYBRID inline explicit xoroshiro128p( ulong_ seed )
	{
		spice_assert( seed > 0 );

		auto h = hash( seed );
		s0 = (uint_)h;
		s1 = ( uint_ )( h >> 32 );

		h = hash( h );
		s2 = (uint_)h;
		s3 = ( uint_ )( h >> 32 );
	}

	HYBRID inline uint_ operator()()
	{
		auto result = s0 + s3;

		auto t = s1 << 9;

		s2 ^= s0;
		s3 ^= s1;
		s1 ^= s2;
		s0 ^= s3;

		s2 ^= t;

		s3 = rotl32( s3, 11 );

		return result;
	}

private:
	uint_ s0, s1, s2, s3;
};

// http://prng.di.unimi.it/xoshiro256starstar.c
class xoroshiro256ss
{
public:
	using result_type = ulong_;
	constexpr result_type min() { return 0; }
	constexpr result_type max() { return std::numeric_limits<ulong_>::max(); }

	HYBRID inline explicit xoroshiro256ss( ulong_ seed )
	{
		spice_assert( seed > 0 );

		s0 = hash( seed );
		s1 = hash( s0 );
		s2 = hash( s1 );
		s3 = hash( s2 );
	}

	HYBRID inline result_type operator()()
	{
		auto result = rotl64( s1 * 5, 7 ) * 9;

		auto t = s1 << 17;

		s2 ^= s0;
		s3 ^= s1;
		s1 ^= s2;
		s0 ^= s3;

		s2 ^= t;

		s3 = rotl64( s3, 45 );

		return result;
	}

private:
	ulong_ s0, s1, s2, s3;
};


// @return rand no. in [0, 1)
template <typename Gen>
HYBRID float uniform_left_inc( Gen & gen )
{
	return ( (uint_)gen() >> 8 ) / 16777216.0f;
}

// @return rand no. in (0, 1]
template <typename Gen>
HYBRID float uniform_right_inc( Gen & gen )
{
	return ( ( (uint_)gen() >> 8 ) + 1.0f ) / 16777216.0f;
}

template <typename Gen>
HYBRID float exprnd( Gen & gen )
{
	return -logf( uniform_right_inc( gen ) );
}

// @param m mu
// @param s sigma (standard deviation)
template <typename Gen>
HYBRID static float normrnd( Gen & gen, float m = 0.0f, float s = 1.0f )
{
	return fmaf(
	    sqrtf( -2 * logf( uniform_right_inc( gen ) ) ) * sinpif( 2 * uniform_left_inc( gen ) ),
	    s,
	    m );
}

template <typename Gen>
HYBRID int_ binornd( Gen & gen, int_ N, float p )
{
#ifndef __CUDA_ARCH__
	using std::max;
	using std::min;
#endif

	return min( N, max( 0, (int)lrintf( normrnd( gen, N * p, sqrtf( N * p * ( 1 - p ) ) ) ) ) );
}
} // namespace util
} // namespace spice

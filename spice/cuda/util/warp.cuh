#pragma once

#include <spice/cuda/util/utility.cuh>


namespace spice
{
namespace cuda
{
namespace util
{
class warp
{
public:
	template <typename T>
	static __device__ T inclusive_scan( T val, T & out_total, uint_ const mask )
	{
		spice_assert( mask );

#pragma unroll
		for( int_ delta = 1; delta < WARP_SZ; delta *= 2 )
		{
			T tmp = __shfl_up_sync( mask, val, delta );
			val += ( laneid() >= delta ) * tmp;
		}

		out_total = __shfl_sync( mask, val, 31 - __clz( mask ) );

		return val;
	}
};
} // namespace util
} // namespace cuda
} // namespace spice

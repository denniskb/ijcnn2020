#pragma once


namespace spice::util
{
template <typename Prec>
class kahan_sum
{
public:
	// Adds delta to the sum. Returns the actual delta added (delta - residue)
	Prec add( Prec delta )
	{
		auto y = delta - _c;
		auto t = _sum + y;
		_c = ( t - _sum ) - y;
		_sum = t;

		return y;
	}

	// Returns total sum
	operator Prec() const { return _sum; }

private:
	Prec _c = 0;
	Prec _sum = 0;
};
} // namespace spice::util
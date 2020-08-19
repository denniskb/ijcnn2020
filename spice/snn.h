#pragma once

#include <spice/snn_info.h>
#include <spice/util/adj_list.h>
#include <spice/util/layout.h>
#include <spice/util/numeric.h>

#include <functional>
#include <vector>


namespace spice
{
// Abstract base class for various backend implementations
// such as cpu::snn, cuda::snn, etc.
template <typename Model>
class snn
{
public:
	virtual ~snn() = default;

	virtual void step( std::vector<int> * out_spikes = nullptr ) = 0;

	virtual std::size_t num_neurons() const = 0;
	virtual std::size_t num_synapses() const = 0;
	float dt() const;
	int delay() const;
	snn_info info() const;

	// TODO: return variants instead (cpu::snn returns span, gpu::snn returns vector)
	// (edges, width)
	virtual std::pair<std::vector<int>, std::size_t> adj() const = 0;
	virtual std::vector<typename Model::neuron::tuple_t> neurons() const = 0;
	virtual std::vector<typename Model::synapse::tuple_t> synapses() const = 0;

protected:
	explicit snn( float dt, int delay = 1 );
	void _step( std::function<void( int, float )> impl );

private:
	float const _dt;
	int const _delay;
	int _i = 0;
	util::kahan_sum<float> _simtime;
};
} // namespace spice

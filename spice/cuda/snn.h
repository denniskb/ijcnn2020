#pragma once

#include <spice/cpu/snn.h>
#include <spice/cuda/util/dbuffer.h>
#include <spice/cuda/util/dvar.h>
#include <spice/snn.h>
#include <spice/util/circular_buffer.h>
#include <spice/util/meta.h>
#include <spice/util/span.hpp>
#include <spice/util/span2d.h>


namespace spice::cuda
{
template <typename Model>
class snn : public ::spice::snn<Model>
{
public:
	snn( spice::util::layout const & desc, float dt, int delay = 1, int first = 0, int last = -1 );
	snn( spice::cpu::snn<Model> const & net );

	void step( std::vector<int> * out_spikes = nullptr ) override;
	void step( int ** out_spikes, unsigned ** out_num_spikes );

	std::size_t num_neurons() const override;
	std::size_t num_synapses() const override;
	std::pair<std::vector<int>, std::size_t> adj() const override;
	std::vector<typename Model::neuron::tuple_t> neurons() const override;
	std::vector<typename Model::synapse::tuple_t> synapses() const override;

private:
	spice::util::soa_t<util::dbuffer, typename Model::neuron> _neurons;
	spice::util::soa_t<util::dbuffer, typename Model::synapse> _synapses;

	struct
	{
		util::dbuffer<int> edges;
		spice::util::adj_list adj;
		util::dbuffer<int> ages;
	} _graph;

	struct
	{
		// TODO: Optimize memory consumption (low-priority)
		util::dbuffer<int> ids_data;
		spice::util::span2d<int> ids;
		util::dbuffer<unsigned> counts;

		util::dbuffer<unsigned> history_data;
		spice::util::span2d<unsigned> history;

		util::dbuffer<int> updates;
		util::dvar<unsigned> num_updates;
	} _spikes;

	int const _first;
	int const _last;

	int MAX_HISTORY() const;

	void reserve( std::size_t num_neurons, std::size_t max_degree, int delay );
};
} // namespace spice::cuda

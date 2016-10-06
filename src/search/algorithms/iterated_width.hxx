
#pragma once

#include <functional>

#include <search/nodes/blind_search_node.hxx>
#include <search/components/single_novelty.hxx>
#include <search/drivers/registry.hxx>
#include <search/algorithms/aptk/breadth_first_search.hxx>
#include <ground_state_model.hxx>
#include <heuristics/novelty/novelty_features_configuration.hxx>

#include <aptk2/search/components/stl_unsorted_fifo_open_list.hxx>

#include <search/events.hxx>

namespace fs0 { class SearchStats; }

namespace fs0 { namespace drivers {

//! The original IW algorithm, adapted to FStrips
template <typename StateModelT>
class FS0IWAlgorithm {
public:
	using ActionT = typename StateModelT::ActionType;
	
	using PlanT = std::vector<typename StateModelT::ActionType::IdType>;
	
	//! IW uses a simple blind-search node
	using SearchNode = BlindSearchNode<State, ActionT>;
	
	//! IW uses a single novelty component as the open list evaluator
	using SearchNoveltyEvaluator = SingleNoveltyComponent<StateModelT, SearchNode>;
	
	//! IW uses an unsorted queue with a NoveltyEvaluator acceptor
	using OpenList = aptk::StlUnsortedFIFO<SearchNode, SearchNoveltyEvaluator>;
	
	//! The base algorithm for IW is a simple Breadth-First Search
	using BaseAlgorithm = lapkt::StlBreadthFirstSearch<SearchNode, StateModelT, OpenList>;
	
	FS0IWAlgorithm(const StateModelT& model, unsigned initial_max_width, unsigned final_max_width, const NoveltyFeaturesConfiguration& feature_configuration, SearchStats& stats)
		: _model(model), _algorithm(nullptr) ,_current_max_width(initial_max_width), _final_max_width(final_max_width), _feature_configuration(feature_configuration), _stats(stats)
	{
		using StatsT = StatsObserver<SearchNode>;
		_handlers.push_back(std::unique_ptr<StatsT>(new StatsT(_stats)));
		setup_base_algorithm(_current_max_width);
	}
	
	~FS0IWAlgorithm() = default;
	
	FS0IWAlgorithm(const FS0IWAlgorithm&) = default;
	FS0IWAlgorithm(FS0IWAlgorithm&&) = default;
	FS0IWAlgorithm& operator=(const FS0IWAlgorithm&) = default;
	FS0IWAlgorithm& operator=(FS0IWAlgorithm&&) = default;	
	
	bool search(const State& state, PlanT& solution);
	
	void setup_base_algorithm(unsigned max_width);
	
	//! Convenience method
	bool solve_model(PlanT& solution) { return search( _model.getTask().getInitialState(), solution ); }
	
	
protected:
	
	//!
	const StateModelT& _model;
	
	//!
	std::unique_ptr<BaseAlgorithm> _algorithm;
	
	//!
	unsigned _current_max_width;
	
	//!
	unsigned _final_max_width;

	//! Novelty evaluator configuration
	const NoveltyFeaturesConfiguration _feature_configuration;
	
	std::vector<std::unique_ptr<lapkt::events::EventHandler>> _handlers;
	
	//! A reference to the stats object managed by the parent.
	SearchStats& _stats;
};

// explicit instantiations
template class FS0IWAlgorithm<GroundStateModel>;
template class FS0IWAlgorithm<LiftedStateModel>;

} } // namespaces

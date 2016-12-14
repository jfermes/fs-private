
#pragma once

#include <search/algorithms/ehc_gbfs.hxx>
#include <search/nodes/heuristic_search_node.hxx>
#include <utils/config.hxx>

#include <search/algorithms/ehc.hxx>
#include <search/events.hxx>
#include <heuristics/relaxed_plan/smart_rpg.hxx>

namespace fs0 { class GroundStateModel; class SearchStats; }

namespace fs0 { namespace language { namespace fstrips { class Formula; } }}
namespace fs = fs0::language::fstrips;

namespace fs0 { namespace drivers {

//! An engine creator for the Greedy Best-First Search drivers coupled with our constrained RPG-based heuristics (constrained h_FF, constrained h_max)
//! The choice of the heuristic is done through template instantiation
class SmartEffectDriver : public Driver {
public:
	using NodeT = HeuristicSearchNode<State, GroundAction>;
	using Engine = std::unique_ptr<EHCThenGBFSSearch<fs0::gecode::SmartRPG>>;
	
protected:
	std::unique_ptr<fs0::gecode::SmartRPG> _heuristic;
	std::vector<std::unique_ptr<lapkt::events::EventHandler>> _handlers;
	
public:
	SmartEffectDriver() {}
	
	Engine create(const Config& config, const GroundStateModel& problem, SearchStats& stats);
	
	static gecode::SmartRPG* configure_heuristic(const Problem& problem, const Config& config);
	
	static GroundStateModel setup(Problem& problem);
	
	ExitCode search(Problem& problem, const Config& config, const std::string& out_dir, float start_time) override;
};

} } // namespaces

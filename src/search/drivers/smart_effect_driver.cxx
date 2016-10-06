
#include <search/drivers/smart_effect_driver.hxx>

#include <problem.hxx>
#include <problem_info.hxx>
#include <state.hxx>
#include <search/algorithms/aptk/best_first_search.hxx>
#include <search/utils.hxx>
#include <constraints/gecode/handlers/lifted_effect_csp.hxx>
#include <actions/ground_action_iterator.hxx>
#include <actions/grounding.hxx>

#include <heuristics/relaxed_plan/rpg_index.hxx>
#include <utils/support.hxx>
#include <search/drivers/setups.hxx>


using namespace fs0::gecode;

namespace fs0 { namespace drivers {

gecode::SmartRPG*
SmartEffectDriver::configure_heuristic(const Problem& problem, const Config& config) {
	bool novelty = config.useNoveltyConstraint() && !problem.is_predicative();
	bool approximate = config.useApproximateActionResolution();
	
	const auto& tuple_index = problem.get_tuple_index();
	const std::vector<const PartiallyGroundedAction*>& actions = problem.getPartiallyGroundedActions();
	auto managers = LiftedEffectCSP::create(actions, tuple_index, approximate, novelty);
	
	const auto managed = support::compute_managed_symbols(std::vector<const ActionBase*>(actions.begin(), actions.end()), problem.getGoalConditions(), problem.getStateConstraints());
	ExtensionHandler extension_handler(problem.get_tuple_index(), managed);
	return new SmartRPG(problem, problem.getGoalConditions(), problem.getStateConstraints(), std::move(managers), extension_handler);
}

SmartEffectDriver::Engine
SmartEffectDriver::create(const Config& config, const GroundStateModel& model) {
	LPT_INFO("main", "Using the smart-effect driver");
	const Problem& problem = model.getTask();
	bool novelty = config.useNoveltyConstraint() && !problem.is_predicative();
	bool approximate = config.useApproximateActionResolution();
	
	const std::vector<const PartiallyGroundedAction*>& actions = problem.getPartiallyGroundedActions();
	
	_heuristic = std::unique_ptr<SmartRPG>(configure_heuristic(problem, config));
	
	// If necessary, we constrain the state variables domains and even action/effect CSPs that will be used henceforth
	// by performing a reachability analysis.
	if (config.getOption("reachability_analysis")) {
		LPT_INFO("main", "Applying reachability analysis");
		RPGIndex graph = _heuristic->compute_full_graph(problem.getInitialState());
		LiftedEffectCSP::prune_unreachable(_heuristic->get_managers(), graph);
	}
	
	using EvaluatorT = EvaluationObserver<NodeT, SmartRPG>;
	using StatsT = StatsObserver<NodeT>;
	using HAObserverT = HelpfulObserver<NodeT>;
	
	EHCSearch<SmartRPG>* ehc = nullptr;
	if (config.getOption("ehc")) {
		// TODO Apply reachability analysis for the EHC heuristic as well
		auto ehc_managers = LiftedEffectCSP::create(actions,  problem.get_tuple_index(), approximate, novelty);
		const auto managed = support::compute_managed_symbols(std::vector<const ActionBase*>(actions.begin(), actions.end()), problem.getGoalConditions(), problem.getStateConstraints());
		ExtensionHandler extension_handler(problem.get_tuple_index(), managed);		
		SmartRPG ehc_heuristic(problem, problem.getGoalConditions(), problem.getStateConstraints(), std::move(ehc_managers), extension_handler);
		ehc = new EHCSearch<SmartRPG>(model, std::move(ehc_heuristic), config.getOption("helpful_actions"), _stats);
	}
	
	_handlers.push_back(std::unique_ptr<StatsT>(new StatsT(_stats)));
	
	if (config.requiresHelpfulnessAssessment()) {
		_handlers.push_back(std::unique_ptr<HAObserverT>(new HAObserverT()));
	}
	
	_handlers.push_back(std::unique_ptr<EvaluatorT>(new EvaluatorT(*_heuristic, config.getNodeEvaluationType())));
	
	auto gbfs = new lapkt::StlBestFirstSearch<NodeT, SmartRPG, GroundStateModel>(model, *_heuristic);
	
	lapkt::events::subscribe(*gbfs, _handlers);
	
	return Engine(new EHCThenGBFSSearch<SmartRPG>(problem, gbfs, ehc));
}

GroundStateModel
SmartEffectDriver::setup(Problem& problem) {
	return GroundingSetup::ground_search_lifted_heuristic(problem);
}


void 
SmartEffectDriver::search(Problem& problem, const Config& config, const std::string& out_dir, float start_time) {
	GroundStateModel model = setup(problem);
	auto engine = create(config, model);
	Utils::do_search(*engine, model, out_dir, start_time, _stats);
}

} } // namespaces


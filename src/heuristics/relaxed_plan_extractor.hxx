
#pragma once

#include <vector>
#include <algorithm>
#include <queue>

#include <state.hxx>
#include <problem.hxx>
#include <heuristics/rpg_data.hxx>
#include <utils/utils.hxx>
#include <utils/printers.hxx>
#include <action_manager.hxx>

#include <utils/logging.hxx>

namespace fs0 {

class SupportedAction {
public:
	ActionIdx _action;
	Atom::vctrp _support;
	
	SupportedAction(ActionIdx action, Atom::vctrp support) : 
		_action(action), _support(support) {}
	
	inline bool operator==(const SupportedAction& rhs) const { 
		return _action == rhs._action && *_support == *(rhs._support);
	}
	
	inline bool operator!=(const SupportedAction& rhs) const {return !this->operator==(rhs);}
	inline bool operator< (const SupportedAction& rhs) const {
		return _action < rhs._action || (_action == rhs._action && *_support < *(rhs._support));
	}
	inline bool operator> (const SupportedAction& rhs) const {return  rhs.operator<(*this);}
	inline bool operator<=(const SupportedAction& rhs) const {return !this->operator>(rhs);}
	inline bool operator>=(const SupportedAction& rhs) const {return !this->operator<(rhs);}
	
	//! Prints a representation of the state to the given stream.
	friend std::ostream& operator<<(std::ostream &os, const SupportedAction& o) { return o.print(os); }
	std::ostream& print(std::ostream& out) const {
		const ProblemInfo& problemInfo = Problem::getCurrentProblem()->getProblemInfo();
		out << problemInfo.getActionName(_action) << ", where: ";
		for (const auto& atom:*(_support))  out << atom << ", ";
		return out;
	}
};

/**
 * A Relaxed Plan extractor. This class is used to perform plan extraction from
 * an already existing RPG data structure. Two different subclasses exist differing
 * in the way in which the repeated application of the same actions is treated.
 */
class BaseRelaxedPlanExtractor {
protected:
	const State& _seed;
	
	//! The book-keeping RPG data.
	const RPGData& _data;
	
	std::set<Atom> processed;
	std::queue<Atom> pending;

public:
	
	/**
	 * @param seed The original, non-relaxed state.
 	 * @param data The data structure representing the planning graph
	 */
	BaseRelaxedPlanExtractor(const State& seed, const RPGData& data) :
		_seed(seed), _data(data), processed(), pending()
	{}
	
	virtual ~BaseRelaxedPlanExtractor() {}


	/**
	 * Builds the relaxed plan from the layered planning graph contained in the '_data' attribute,
	 * starting from the goal causes given in 'goalAtoms'.
	 * 
	 * @param goalAtoms The atoms that allowed the planning graph to reach a goal state.
	 */
	float computeRelaxedPlanCost(const Atom::vctr& goalAtoms) {
		enqueueAtoms(goalAtoms);
		
		while (!pending.empty()) {
			const Atom& atom = pending.front();
			processAtom(atom);
			pending.pop();
		}
		
		return buildRelaxedPlan();
	}

protected:
	//! Put all the atoms in a given vector of atoms in the queue to be processed.
	inline void enqueueAtoms(const Atom::vctr& atoms) { for(auto& atom:atoms) pending.push(atom); }

	//! Process a single atom by seeking its supports left-to-right in the RPG and enqueuing them to be further processed
	void processAtom(const Atom& atom) {
		if (_seed.contains(atom)) return; // The atom was already on the seed state, thus has empty support.
		if (processed.find(atom) != processed.end()) return; // The atom has already been processed
		
		const RPGData::AtomSupport& support = _data.getAtomSupport(atom);
		
		registerPlanAction(support);
		enqueueAtoms(*(std::get<2>(support))); // Push the causes of the causing action.
		enqueueAtoms(*(std::get<3>(support))); // Push the causes of the particular atom.
		processed.insert(atom); // Tag the atom as processed.
	}
	
	virtual void registerPlanAction(const RPGData::AtomSupport& support) = 0;
	
	virtual float buildRelaxedPlan() = 0;
};



/**
 * In a CompleteRelaxedPlanExtractor, a relaxed plan is composed not only of actions but
 * only of the conditions under which those actions are performed, e.g. an action being such as:
 * "move(right), when the current position is (1, 3)"
 * This yields longer relaxed plans.
 */
class CompleteRelaxedPlanExtractor : public BaseRelaxedPlanExtractor {
protected:
	std::set<SupportedAction> supporters;

public:
	/**
	 * @param seed The original, non-relaxed state.
 	 * @param data The data structure representing the planning graph
	 */
	CompleteRelaxedPlanExtractor(const State& seed, const RPGData& data) :
		BaseRelaxedPlanExtractor(seed, data), supporters()
	{}

protected:

	
	void registerPlanAction(const RPGData::AtomSupport& support) {
		supporters.insert(SupportedAction(std::get<1>(support), std::get<3>(support)));
	}
	
	float buildRelaxedPlan() {
#ifndef DEBUG
		return (float) supporters.size();
#endif

		FDEBUG("heuristic" , "Relaxed plan found with length " << supporters.size() << std::endl << SupportedPlanPrinter(supporters));
		return (float) supporters.size();
	}
};

/**
 * In an ApproximateRelaxedPlanExtractor, a relaxed plan is simply composed of a list of actions,
 * not making any distinction with respect to the values of variables relevant for the action effects
 * under which the action is undertaken.
 */
class ApproximateRelaxedPlanExtractor : public BaseRelaxedPlanExtractor {
protected:
		std::vector<std::set<ActionIdx>> perLayerSupporters;

public:
	/**
	 * @param seed The original, non-relaxed state.
 	 * @param data The data structure representing the planning graph
	 */
	ApproximateRelaxedPlanExtractor(const State& seed, const RPGData& data) :
		BaseRelaxedPlanExtractor(seed, data), perLayerSupporters(data.getNumLayers())
	{}

protected:

	void registerPlanAction(const RPGData::AtomSupport& support) {
		// We ignore the particular atom support and take only into account the action
		perLayerSupporters[std::get<0>(support)].insert(std::get<1>(support));
	}
	
	float buildRelaxedPlan() {
#ifndef DEBUG
		// In production mode, we simply count the number of actions in the plan, but prefer not to build the actual plan.
		unsigned size = 0;
		for (const auto& supporters:perLayerSupporters) {
			size += supporters.size();
		}
		return (float) size;
#endif

		// In debug mode, we build the relaxed plan by flattening the supporters at each layer, so that we can log the actual plan.
		ActionPlan plan;
		for (const auto& supporters:perLayerSupporters) {
			plan.insert(plan.end(), supporters.cbegin(), supporters.cend());
		}

		// Note that computing the relaxed heuristic by using some form of local consistency might yield plans that are not correct for the relaxation
		// assert(ActionManager::checkRelaxedPlanSuccessful(*Problem::getCurrentProblem(), plan, _seed));
		FDEBUG("heuristic" , "Relaxed plan found with length " << plan.size() << std::endl << PlanPrinter(plan));

		return (float) plan.size();
	}
};

} // namespaces
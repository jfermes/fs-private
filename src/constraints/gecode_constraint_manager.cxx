

#include <constraints/gecode_constraint_manager.hxx>
#include <constraints/gecode/helper.hxx>
#include <problem.hxx>
#include <utils/projections.hxx>
#include <utils/utils.hxx>
#include <constraints/gecode/simple_csp.hxx>

using namespace Gecode;
using namespace fs0::gecode;

namespace fs0 {

// Note that we use both types of constraints as goal constraints
GecodeConstraintManager::GecodeConstraintManager(const ScopedConstraint::vcptr& goalConstraints, const ScopedConstraint::vcptr& stateConstraints) :
	allRelevantVariables(),
	allGoalConstraints(Utils::merge(goalConstraints, stateConstraints)),
	goalConstraintsManager(allGoalConstraints), // We store all the constraints in a new vector so that we can pass a const reference 
	                                            // - we're strongly interested in the ConstraintManager having only a reference, not the actual value,
	                                            // since in some cases each grounded action will have a ConstraintManager
	hasStateConstraints(stateConstraints.size() > 0)
{
	auto relevantSet = getAllRelevantVariables(goalConstraints, stateConstraints);
	allRelevantVariables = VariableIdxVector(relevantSet.begin(), relevantSet.end());
	
	baseCSP = createCSPVariables(goalConstraints, stateConstraints);
	Helper::translateConstraints(*baseCSP, translator, stateConstraints); // state constraints
	Helper::translateConstraints(*baseCSP, translator, goalConstraints); // Action preconditions
	
	// MRJ: in order to be able to clone a CSP, we need to ensure that it is "stable" i.e. propagate all constraints until fixed point
	Gecode::SpaceStatus st = baseCSP->status();
	// TODO This should prob. never happened, as it'd mean that the action is (statically) unapplicable.
	assert(st != Gecode::SpaceStatus::SS_FAILED); 	
}

ScopedConstraint::Output GecodeConstraintManager::pruneUsingStateConstraints(RelaxedState& state) const {
	if (!hasStateConstraints) return ScopedConstraint::Output::Unpruned;
	
	// TODO TODO TODO
	
	return ScopedConstraint::Output::Unpruned;
}














bool GecodeConstraintManager::isGoal(const State& seed, const RelaxedState& layer, Atom::vctr& support) const {
	assert(support.empty());
	
	SimpleCSP* csp = dynamic_cast<SimpleCSP::ptr>(baseCSP->clone());

	// Setup domain constraints etc.
	Helper::addRelevantVariableConstraints(*csp, translator, allRelevantVariables, layer);
	
	bool res;
	if (true) {  // Solve the CSP completely
		res = solveCSP(csp, support, seed);
	} else { // Check only local consistency
// 		if (!csp->checkConsistency()) return; // We're done
		// TODO
		res = false;
	}
	
	delete csp;
	return res;
}


//! Don't care about supports, etc.
bool GecodeConstraintManager::isGoal(const RelaxedState& layer) const {
	Atom::vctr dummy;
	State dummy_state(0 ,dummy);
	return isGoal(dummy_state, layer, dummy);
}


SimpleCSP::ptr GecodeConstraintManager::createCSPVariables(const ScopedConstraint::vcptr& goalConstraints, const ScopedConstraint::vcptr& stateConstraints) {
	// Determine input and output variables for this action: we first amalgamate variables into a set
	// to avoid repetitions, then generate corresponding CSP variables, then create the CSP model with them
	// and finally add the model constraints.

	SimpleCSP::ptr csp = new SimpleCSP;

	IntVarArgs variables;
	// Index the relevant variables first
	for (VariableIdx var:allRelevantVariables) {
		unsigned id = Helper::processVariable( *csp, var, variables );
		translator.registerCSPVariable(var, GecodeCSPTranslator::VariableType::Input, id);
	}

	IntVarArray tmp( *csp, variables );
	csp->_X.update( *csp, false, tmp );

	std::cout << "Created SimpleCSP with variables: " << csp->_X << std::endl;
	return csp;
}

VariableIdxSet GecodeConstraintManager::getAllRelevantVariables(const ScopedConstraint::vcptr& goalConstraints, const ScopedConstraint::vcptr& stateConstraints) {
	VariableIdxSet variables;
	// Add the variables mentioned by state constraints
	for (ScopedConstraint::cptr constraint : stateConstraints) variables.insert( constraint->getScope().begin(), constraint->getScope().end() );
	// Add the variables mentioned in the preconditions
	for (ScopedConstraint::cptr constraint : goalConstraints) variables.insert( constraint->getScope().begin(), constraint->getScope().end() );
	return variables;
}

bool GecodeConstraintManager::solveCSP(gecode::SimpleCSP* csp, Atom::vctr& support, const State& seed) const {
	// TODO posting a branching might make sense to prioritize some branching strategy?
    // branch(*this, l, INT_VAR_SIZE_MIN(), INT_VAL_MIN());
	DFS<SimpleCSP> engine(csp);
	
	// ATM we are happy to extract the goal support from the first solution
	// TODO An alternative strategy to try out would be to select the solution with most atoms in the seed state, but that implies iterating through all solutions, which might not be worth it?
	SimpleCSP* solution = engine.next();
	if (solution) {
		for (VariableIdx variable:allRelevantVariables) {
			support.push_back(Atom(variable, translator.resolveValue(*solution, variable, GecodeCSPTranslator::VariableType::Input)));
		}
	}
	return (bool) solution;
}


} // namespaces

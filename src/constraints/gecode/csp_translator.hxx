
#pragma once

#include <unordered_map>
#include <unordered_set>

#include <fs0_types.hxx>
#include <problem_info.hxx>
#include <constraints/gecode/simple_csp.hxx>
#include <constraints/gecode/utils/translation.hxx>
#include <languages/fstrips/language.hxx>

#include <gecode/int.hh>

namespace fs = fs0::language::fstrips;


namespace fs0 {
	class RelaxedState;
}

namespace fs0 { namespace gecode {


/**
 * This exception is thrown whenever a variable cannot be resolved
 */
class UnregisteredStateVariableError : public std::runtime_error {
public:
	UnregisteredStateVariableError( const char* what_msg );
	UnregisteredStateVariableError( const std::string& what_msg );

	virtual ~UnregisteredStateVariableError();

};

/**
 * A CSP translator keeps track of the correspondence between Planning variables and CSP variables.
 * To this end, it keeps a mapping of the form <x, t> --> y, where:
 *   - 'x' is the ID of a planning variable
 *   - 't' is the type or role under which this variable can appear on a CSP
 *     (e.g. the same state variable 'val(c1)' could appear in a certain CSP as input _and_ output variable of an effect
 *   - 'y' is the identifier of the CSP variable (i.e. implementation-wise, the position in which the variable appears on the '_intvars' IntVarArray of the CSP)
 */
class GecodeCSPVariableTranslator {
public:


	GecodeCSPVariableTranslator() {};
	virtual ~GecodeCSPVariableTranslator() {}

	//! Forbid copy constructor
	explicit GecodeCSPVariableTranslator(GecodeCSPVariableTranslator& other);


	//! Returns true iff the given term has already an associated CSP variable
	bool isRegistered(const fs::Term::cptr term, CSPVariableType type) const;
	
	//! Returns true iff the constraints that correspond to the term / type have already been posted
	bool isPosted(const fs::Term::cptr term, CSPVariableType type) const;
	
	//! Marks the given pair of term / type as with its corresponding constraint having been posted
	void setPosted(const fs::Term::cptr term, CSPVariableType type);
	

	//! Register the given term (under the give role/type) by creating a corresponding CSP variable.
	//! Returns true iff the (variable, type) tuple was actually registered for the first time (i.e. had not been registered yet)
	bool registerConstant(fs::Constant::cptr constant, SimpleCSP& csp, Gecode::IntVarArgs& variables);
	bool registerStateVariable(fs::StateVariable::cptr variable, CSPVariableType type, SimpleCSP& csp, Gecode::IntVarArgs& variables, bool nullable = false);
	bool registerNestedTerm(fs::NestedTerm::cptr nested, CSPVariableType type, SimpleCSP& csp, Gecode::IntVarArgs& variables);
	bool registerNestedTerm(fs::NestedTerm::cptr nested, CSPVariableType type, TypeIdx domain_type, SimpleCSP& csp, Gecode::IntVarArgs& variables);
	bool registerNestedTerm(fs::NestedTerm::cptr nested, CSPVariableType type, int min, int max, SimpleCSP& csp, Gecode::IntVarArgs& variables);

	//! Returns the Gecode CSP variable that corresponds to the given term under the given role, for the given CSP
	const Gecode::IntVar& resolveVariable(fs::Term::cptr term, CSPVariableType type, const SimpleCSP& csp) const;
	
	//! Returns the value of the Gecode CSP variable that corresponds to the given term under the given role, for the given CSP
	ObjectIdx resolveValue(fs::Term::cptr term, CSPVariableType type, const SimpleCSP& csp) const;

	//! Handy helper to resolve a number of variables at the same time
	Gecode::IntVarArgs resolveVariables(const std::vector<fs::Term::cptr>& terms, CSPVariableType type, const SimpleCSP& csp) const;

	//! Registers auxiliary data necessary for the element constraint that corresponds to the given term
	void registerElementConstraintData(fs::NestedTerm::cptr term, unsigned max_idx, SimpleCSP& csp, Gecode::IntVarArgs& intvars, Gecode::BoolVarArgs& boolvars);
	
	//! Returns auxiliary data necessary for the element constraint that corresponds to the given term
	void resolveElementConstraintData(fs::Term::cptr term, const SimpleCSP& csp, Gecode::IntVar& index, Gecode::BoolVarArgs& reification_vars_0, Gecode::BoolVarArgs& reification_vars_1) const;


	//! The key operation in the RPG progression: to update the domains of the relevant state variables for a certain layer of the RPG.
	void updateStateVariableDomains(SimpleCSP& csp, const RelaxedState& layer) const;

	//! Returns the CSP variable that corresponds to the given input state variable, in the given CSP.
	const Gecode::IntVar&  resolveInputStateVariable(const SimpleCSP& csp, VariableIdx variable) const {
		const auto& it = _input_state_variables.find(variable);
		if (it == _input_state_variables.end()) throw UnregisteredStateVariableError("Trying to resolve non-registered input state variable");
		return csp._intvars[it->second];
	}

	//! Returns the value of the CSP variable that corresponds to the given input state variable, in the given CSP.
	const ObjectIdx resolveInputStateVariableValue(const SimpleCSP& csp, VariableIdx variable) const {
		return resolveInputStateVariable(csp, variable).val();
	}

	//! Returns the CSP variable that corresponds to the given output state variable, in the given CSP.
	const Gecode::IntVar& resolveOutputStateVariable(const SimpleCSP& csp, VariableIdx variable) const {
		const auto& it = _output_state_variables.find(variable);
		if (it == _output_state_variables.end()) throw UnregisteredStateVariableError("Trying to resolve non-registered output state variable");
		return csp._intvars[it->second];
	}

	//! Returns the value of the CSP variable that corresponds to the given output state variable, in the given CSP.
	const ObjectIdx resolveOutputStateVariableValue(const SimpleCSP& csp, VariableIdx variable) const {
		return resolveOutputStateVariable(csp, variable).val();
	}

	const std::unordered_map<VariableIdx, unsigned>& getAllInputVariables() const { return _input_state_variables; }

	//! Prints a representation of the object to the given stream.
	std::ostream& print(std::ostream& os, const SimpleCSP& csp) const;

protected:
	//! A map mapping terms that have already been processed (under a certain role, e.g. input/output) to the ID of their corresponding CSP variable
	std::unordered_map<TranslationKey, unsigned> _registered;
	
	//! A set marking all terms whose corresponding constraints have already been posted.
	std::unordered_set<TranslationKey> _posted;

	//! Some data structures to keep track of all registered state variables, so that we can update their domains and parse their values efficiently.
	//! In particular, these maps the index of state variables that have been registered under different input/output roles to
	//! the ID of the corresponding CSP variable
	std::unordered_map<VariableIdx, unsigned> _input_state_variables;
	std::unordered_map<VariableIdx, unsigned> _output_state_variables;

	//! A table keeping track of auxiliary data for the element constraint that is necessary for the given term / type.
	//! In particular, the first unsigned of the tuple is the index (within the CSP _intvars array) of the element constraint index variable;
	//! the second is the index (within the CSP _boolvars array) of the first boolean variable of the set of variables that we use for reification purposes,
	//! and the third is the amount of such variables (which will be even, since we keep two equially-sized arrays of boolean variables)
	std::unordered_map<TranslationKey, std::tuple<unsigned, unsigned, unsigned>> element_constraint_data;
};



} } // namespaces

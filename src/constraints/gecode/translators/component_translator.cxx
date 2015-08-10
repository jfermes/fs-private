
#include <constraints/gecode/translators/component_translator.hxx>
#include <constraints/gecode/simple_csp.hxx>
#include <constraints/gecode/csp_translator.hxx>
#include <constraints/gecode/helper.hxx>
#include <constraints/gecode/handlers/csp_handler.hxx>
#include <problem.hxx>
#include <languages/fstrips/builtin.hxx>

#include <gecode/int.hh>

namespace fs0 { namespace gecode {

	typedef fs::RelationalFormula::Symbol AFSymbol;
	const std::map<AFSymbol, Gecode::IntRelType> RelationalFormulaTranslator::symbol_to_gecode = {
		{AFSymbol::EQ, Gecode::IRT_EQ}, {AFSymbol::NEQ, Gecode::IRT_NQ}, {AFSymbol::LT, Gecode::IRT_LE},
		{AFSymbol::LEQ, Gecode::IRT_LQ}, {AFSymbol::GT, Gecode::IRT_GR}, {AFSymbol::GEQ, Gecode::IRT_GQ}
	};
	
	Gecode::IntRelType RelationalFormulaTranslator::gecode_symbol(fs::RelationalFormula::cptr formula) { return symbol_to_gecode.at(formula->symbol()); }
	
	
	const std::map<Gecode::IntRelType, Gecode::IntRelType> RelationalFormulaTranslator::operator_inversions = {
		{Gecode::IRT_EQ, Gecode::IRT_EQ}, {Gecode::IRT_NQ, Gecode::IRT_NQ}, {Gecode::IRT_LE, Gecode::IRT_GQ},
		{Gecode::IRT_LQ, Gecode::IRT_GR}, {Gecode::IRT_GR, Gecode::IRT_LQ}, {Gecode::IRT_GQ, Gecode::IRT_LE}
	};
	
	Gecode::IntRelType RelationalFormulaTranslator::invert_operator(Gecode::IntRelType op) {
		return operator_inversions.at(op);
	}
	
	
void ConstantTermTranslator::registerVariables(const fs::Term::cptr term, CSPVariableType type, SimpleCSP& csp, GecodeCSPVariableTranslator& translator, Gecode::IntVarArgs& variables) const {
	auto constant = dynamic_cast<fs::Constant::cptr>(term);
	assert(constant);
	translator.registerConstant(constant, csp, variables);
}

void StateVariableTermTranslator::registerVariables(const fs::Term::cptr term, CSPVariableType type, SimpleCSP& csp, GecodeCSPVariableTranslator& translator, Gecode::IntVarArgs& variables) const {
	auto variable = dynamic_cast<fs::StateVariable::cptr>(term);
	assert(variable);
	translator.registerStateVariable(variable, type, csp, variables);
}

void NestedTermTranslator::registerVariables(const fs::Term::cptr term, CSPVariableType type, SimpleCSP& csp, GecodeCSPVariableTranslator& translator, Gecode::IntVarArgs& variables) const {
	auto nested = dynamic_cast<fs::NestedTerm::cptr>(term);
	assert(nested);
	
	// If the subterm occurs somewhere else in the action / formula, it might have already been parsed and registered,
	// in which case we do NOT want to register it again
	if (translator.isRegistered(nested, type)) return;
	
	// We first parse and register the subterms recursively. The type of subterm variables is always input
	GecodeCSPHandler::registerTermVariables(nested->getSubterms(), CSPVariableType::Input, csp, translator, variables);
	
	// And now register the CSP variable corresponding to the current term
	do_root_registration(nested, type, csp, translator, variables);
}

void NestedTermTranslator::do_root_registration(const fs::NestedTerm::cptr nested, CSPVariableType type, SimpleCSP& csp, GecodeCSPVariableTranslator& translator, Gecode::IntVarArgs& variables) const {
	translator.registerNestedTerm(nested, type, csp, variables);
}


void ArithmeticTermTranslator::do_root_registration(const fs::NestedTerm::cptr nested, CSPVariableType type, SimpleCSP& csp, GecodeCSPVariableTranslator& translator, Gecode::IntVarArgs& variables) const {
	auto bounds = nested->getBounds();
	translator.registerNestedTerm(nested, type, bounds.first, bounds.second, csp, variables);
}

void ArithmeticTermTranslator::registerConstraints(const fs::Term::cptr term, CSPVariableType type, SimpleCSP& csp, const GecodeCSPVariableTranslator& translator) const {
	auto addition = dynamic_cast<fs::AdditionTerm::cptr>(term);
	assert(addition);
	
	// First we register recursively the constraints of the subterms
	GecodeCSPHandler::registerTermConstraints(addition->getSubterms(), CSPVariableType::Input, csp, translator);
	
	// Now we assert that the root temporary variable equals the sum of the subterms
	const Gecode::IntVar& result = translator.resolveVariable(addition, CSPVariableType::Input, csp);
	Gecode::IntVarArgs operands = translator.resolveVariables(addition->getSubterms(), CSPVariableType::Input, csp);
	post(csp, operands, result);
}


void AdditionTermTranslator::post(SimpleCSP& csp, const Gecode::IntVarArgs& operands, const Gecode::IntVar& result) const {
	Gecode::linear(csp, getLinearCoefficients(), operands, getRelationType(), result);
}

void SubtractionTermTranslator::post(SimpleCSP& csp, const Gecode::IntVarArgs& operands, const Gecode::IntVar& result) const {
	Gecode::linear(csp, getLinearCoefficients(), operands, getRelationType(), result);
}

void MultiplicationTermTranslator::post(SimpleCSP& csp, const Gecode::IntVarArgs& operands, const Gecode::IntVar& result) const {
	Gecode::mult(csp, operands[0], operands[1], result);
}
	
Gecode::IntArgs AdditionTermTranslator::getLinearCoefficients() const {
	std::vector<int> coefficients{1, 1};
	return Gecode::IntArgs(coefficients);
}

Gecode::IntArgs SubtractionTermTranslator::getLinearCoefficients() const {
	std::vector<int> coefficients{1, -1};
	return Gecode::IntArgs(coefficients);
}

Gecode::IntArgs MultiplicationTermTranslator::getLinearCoefficients() const {
	std::vector<int> coefficients{1, -1};
	return Gecode::IntArgs(coefficients);
}


void StaticNestedTermTranslator::registerConstraints(const fs::Term::cptr term, CSPVariableType type, SimpleCSP& csp, const GecodeCSPVariableTranslator& translator) const {
	auto stat = dynamic_cast<fs::StaticHeadedNestedTerm::cptr>(term);
	assert(stat);
	
	// First we register recursively the constraints of the subterms
	GecodeCSPHandler::registerTermConstraints(stat->getSubterms(), CSPVariableType::Input, csp, translator);
	
	// Assume we have a static term s(t_1, ..., t_n), where t_i are the subterms.
	// We have registered a termporary variable Z for the whole term, plus temporaries Z_i accounting for each subterm t_i
	// Now we need to post an extensional constraint on all temporary variables <Z_1, Z_2, ..., Z_n, Z> such that
	// the tuples <z_1, ..., z_n, z> satisfying the constraints are exactly those such that z = s(z_1, ..., z_n)
	
	// First compile the variables in the right order (order matters, must be the same than in the tupleset):
	Gecode::IntVarArgs variables = translator.resolveVariables(stat->getSubterms(), CSPVariableType::Input, csp);
	variables << translator.resolveVariable(stat, CSPVariableType::Input, csp);
	
	// Now compile the tupleset
	Gecode::TupleSet extension = Helper::extensionalize(stat);
	
	// And finally post the constraint
	Gecode::extensional(csp, variables, extension);
}

void FluentNestedTermTranslator::registerConstraints(const fs::Term::cptr term, CSPVariableType type, SimpleCSP& csp, const GecodeCSPVariableTranslator& translator) const {
	const ProblemInfo& info = Problem::getCurrentProblem()->getProblemInfo();
	
	assert(false); // TODO - REGISTER the full element constraint, with reindexing through an extensional constraint
	
	auto fluent = dynamic_cast<fs::FluentHeadedNestedTerm::cptr>(term);
	assert(fluent);
	const auto& signature = info.getFunctionData(fluent->getSymbolId()).getSignature();
	const std::vector<fs::Term::cptr>& subterms = fluent->getSubterms();
	
	// First we register recursively the constraints of the subterms - subterms' constraint will always have input type
	GecodeCSPHandler::registerTermConstraints(subterms, CSPVariableType::Input, csp, translator);
	
	assert(subterms.size() == signature.size());
	if (subterms.size() > 1) throw std::runtime_error("Nested subterms of arity > 1 not yet implemented");
	
	assert(signature.size() == 1); // Cannot be 0, or we'd have instead a StateVariable term
	unsigned idx = 0; // element constraints are 0-indexed
	
	Gecode::IntVarArgs array_variables; // The actual array of variables that will form the element constraint
	
	// The correspondence between the index variable possible values and their 0-indexed position in the element constraint table
	Gecode::TupleSet correspondence;
	
	for (ObjectIdx object:info.getTypeObjects(signature[0])) {
		
		VariableIdx variable = info.resolveStateVariable(fluent->getSymbolId(), {object});
		array_variables << translator.resolveOutputStateVariable(csp, variable); // TODO - Output or Input???
		
		
		correspondence.add(IntArgs(2, object, idx));
		++idx;
	}
	
	correspondence.finalize();
	
	
	
	// Post the extensional constraint relating the value of the index variables
	const Gecode::IntVar& index_variable = translator.resolveVariable(subterms[0], CSPVariableType::Input, csp);
	Gecode::IntVar indexed_index; // TODO - XXX - WHERE DO WE GET THIS FROM??
	Gecode::extensional(csp, IntVarArgs() << index_variable << indexed_index, correspondence);
	
	// Now post the actual element constraint 
	const Gecode::IntVar& element_result = translator.resolveVariable(fluent, CSPVariableType::Input, csp); // TODO - Output or Input???
	Gecode::element(csp, array_variables, indexed_index, element_result);
}




void AtomicFormulaTranslator::registerVariables(const fs::AtomicFormula::cptr formula, SimpleCSP& csp, GecodeCSPVariableTranslator& translator, Gecode::IntVarArgs& variables) const {
	// We simply need to recursively register the variables of each subterm
	for (const Term::cptr subterm:formula->getSubterms()) {
		GecodeCSPHandler::registerTermVariables(subterm, CSPVariableType::Input, csp, translator, variables); // Formula variables are always input variables
	}
}

void AtomicFormulaTranslator::registerConstraints(const fs::AtomicFormula::cptr formula, SimpleCSP& csp, const GecodeCSPVariableTranslator& translator) const {
	// We simply need to recursively register the constraints of each subterm
	for (const Term::cptr subterm:formula->getSubterms()) {
		GecodeCSPHandler::registerTermConstraints(subterm, CSPVariableType::Input, csp, translator);
	}
}

void RelationalFormulaTranslator::registerConstraints(const fs::AtomicFormula::cptr formula, SimpleCSP& csp, const GecodeCSPVariableTranslator& translator) const {
	auto condition = dynamic_cast<fs::RelationalFormula::cptr>(formula);
	assert(condition);
	
	// Register possible nested constraints recursively by calling the parent registrar
	AtomicFormulaTranslator::registerConstraints(formula, csp, translator);
	
	// And register the relation constraint itself
	const Gecode::IntVar& lhs_gec_var = translator.resolveVariable(condition->lhs(), CSPVariableType::Input, csp);
	const Gecode::IntVar& rhs_gec_var = translator.resolveVariable(condition->rhs(), CSPVariableType::Input, csp);
	Gecode::rel(csp, lhs_gec_var, gecode_symbol(condition), rhs_gec_var);
}

void AlldiffGecodeTranslator::registerConstraints(const fs::AtomicFormula::cptr formula, SimpleCSP& csp, const GecodeCSPVariableTranslator& translator) const {
	auto alldiff = dynamic_cast<fs::AlldiffFormula::cptr>(formula);
	assert(alldiff);
	
	// Register possible nested constraints recursively by calling the parent registrar
	AtomicFormulaTranslator::registerConstraints(formula, csp, translator);
	
	Gecode::IntVarArgs variables = translator.resolveVariables(alldiff->getSubterms(), CSPVariableType::Input, csp);
	Gecode::distinct(csp, variables, Gecode::ICL_DOM);
}

void SumGecodeTranslator::registerConstraints(const fs::AtomicFormula::cptr formula, SimpleCSP& csp, const GecodeCSPVariableTranslator& translator) const {
	auto sum = dynamic_cast<fs::SumFormula::cptr>(formula);
	assert(sum);
	
	// Register possible nested constraints recursively by calling the parent registrar
	AtomicFormulaTranslator::registerConstraints(formula, csp, translator);
	
	
	Gecode::IntVarArgs variables = translator.resolveVariables(sum->getSubterms(), CSPVariableType::Input, csp);
	
	// The sum constraint is a particular subcase of gecode's linear constraint with all variables' coefficients set to 1
	// except for the coefficient of the result variable, which is set to -1
	std::vector<int> v_coefficients(variables.size(), 1);
	v_coefficients[variables.size() - 1] = -1; // Last coefficient is a -1, since the last variable of the scope is the element of the sum
	Gecode::IntArgs coefficients(v_coefficients);
	
	Gecode::linear(csp, coefficients, variables, Gecode::IRT_EQ, 0, Gecode::ICL_DOM);
}


} } // namespaces

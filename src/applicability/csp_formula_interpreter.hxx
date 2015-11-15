
#pragma once

#include <memory>
#include <applicability/formula_interpreter.hxx>

namespace fs0 { namespace gecode { class FormulaCSPHandler; }}

namespace fs0 {

//! An satisfiability manager that models formula satisfaction as a CSP in order to determine whether a given formula is satisfiable or not.
class CSPFormulaInterpreter : public FormulaInterpreter {
public:
	CSPFormulaInterpreter(const fs::Formula* formula);

	//! Returns true if the formula represented by the current object is satisfied in the given state
	bool satisfied(const State& state) const;

protected:
	//! The formula handler that will check for CSP applicability
	std::unique_ptr<gecode::FormulaCSPHandler> _csp_handler;
};

} // namespaces


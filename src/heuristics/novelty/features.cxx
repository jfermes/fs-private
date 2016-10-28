
#include <heuristics/novelty/features.hxx>
#include <state.hxx>
#include <languages/fstrips/language.hxx>

namespace fs0 {


aptk::ValueIndex
StateVariableFeature::evaluate( const State& s ) const {
	return s.getValue(_variable);
}

std::ostream& StateVariableFeature::print(std::ostream& os) const {
	const ProblemInfo& info = ProblemInfo::getInstance();
	return os << info.getVariableName(_variable);
}

aptk::ValueIndex
ConditionSetFeature::evaluate( const State& s ) const {
	aptk::ValueIndex satisfied = 0;
	for ( const fs::AtomicFormula* c : _conditions ) {
		if (c->interpret(s)) satisfied++;
	}
	return satisfied;
}

std::ostream& ConditionSetFeature::print(std::ostream& os) const {
	return os << "UNIMPLEMENTED";
}

ArbitraryTermFeature::ArbitraryTermFeature(const fs::Term* term) 
	: _term(term)
{}

ArbitraryTermFeature::~ArbitraryTermFeature() {
	delete _term;
}

ArbitraryTermFeature::ArbitraryTermFeature(const ArbitraryTermFeature& other)
	: _term(other._term->clone())
{}

aptk::ValueIndex
ArbitraryTermFeature::evaluate(const State& s) const {
	return _term->interpret(s);
}

std::ostream& ArbitraryTermFeature::print(std::ostream& os) const {
	return os << *_term;
}

}

#pragma once
#include <fs0_types.hxx>
#include <languages/fstrips/language.hxx>


namespace fs0 { class State; class ProblemInfo; }

namespace fs0 { namespace language { namespace fstrips {

//! A term that has not yet been processed, meaning that it might possibly contain references to yet-unresolved values of action parameters,
//! non-consolidated state variables, etc.
class TermSchema {
public:
	typedef const TermSchema* cptr;
	
	TermSchema() {}
	virtual ~TermSchema() {}

	virtual TermSchema* clone() const = 0;
	
	//! Processes a possibly nested unprocessed term, consolidating the existing state variables
	//! and binding action parameters to concrete language constants.
	virtual Term::cptr process(const Signature& signature, const ObjectIdxVector& binding, const ProblemInfo& info) const = 0;
	
	//! Prints a representation of the object to the given stream.
	friend std::ostream& operator<<(std::ostream &os, const TermSchema& o) { return o.print(os); }
	std::ostream& print(std::ostream& os) const;
	virtual std::ostream& print(std::ostream& os, const ProblemInfo& info) const;
};


class NestedTermSchema : public TermSchema {
public:
	NestedTermSchema(unsigned symbol_id, const std::vector<TermSchema::cptr>& subterms)
		: _symbol_id(symbol_id), _subterms(subterms)
	{}
	
	virtual ~NestedTermSchema() {
		for (TermSchema::cptr term:_subterms) delete term;
	}
	
	NestedTermSchema(const NestedTermSchema& term)
		: _symbol_id(term._symbol_id) {
		for (TermSchema::cptr subterm:term._subterms) {
			_subterms.push_back(subterm->clone());
		}
	}
	
	NestedTermSchema* clone() const { return new NestedTermSchema(*this); }
	
	//! Processes a possibly nested unprocessed term, consolidating the existing state variables
	//! and binding action parameters to concrete language constants.
	Term::cptr process(const Signature& signature, const ObjectIdxVector& binding, const ProblemInfo& info) const;
	
	//! Prints a representation of the object to the given stream.
	std::ostream& print(std::ostream& os, const ProblemInfo& info) const;
	
protected:
	//! The ID of the function or predicate symbol, e.g. in the state variable loc(A), the id of 'loc'
	unsigned _symbol_id;
	
	//! The tuple of fixed, constant symbols of the state variable, e.g. {A, B} in the state variable 'on(A,B)'
	std::vector<TermSchema::cptr> _subterms;
};

class ArithmeticTermSchema : public TermSchema {
public:
	ArithmeticTermSchema(const std::string& symbol, const std::vector<TermSchema::cptr>& subterms)
		: _symbol(symbol), _subterms(subterms)
	{
		assert(subterms.size() == 2);
	}
	
	virtual ~ArithmeticTermSchema() {
		for (TermSchema::cptr term:_subterms) delete term;
	}
	
	ArithmeticTermSchema(const ArithmeticTermSchema& term)
		: _symbol(term._symbol) {
		for (TermSchema::cptr subterm:term._subterms) {
			_subterms.push_back(subterm->clone());
		}
	}
	
	ArithmeticTermSchema* clone() const { return new ArithmeticTermSchema(*this); }
	
	//! Processes a possibly nested unprocessed term, consolidating the existing state variables
	//! and binding action parameters to concrete language constants.
	Term::cptr process(const Signature& signature, const ObjectIdxVector& binding, const ProblemInfo& info) const;
	
	//! Prints a representation of the object to the given stream.
	std::ostream& print(std::ostream& os, const ProblemInfo& info) const;
	
protected:
	//! The name of the function or predicate symbol, e.g. in the term a + b, the string '+'
	std::string _symbol;
	
	//! The tuple of fixed, constant symbols of the state variable, e.g. {A, B} in the state variable 'on(A,B)'
	std::vector<TermSchema::cptr> _subterms;
};


//! A constant which is derived from the parameter of an action schema
class ActionSchemaParameter : public TermSchema {
public:
	ActionSchemaParameter(unsigned position, const std::string& name) : _position(position), _name(name) {}
	
	ActionSchemaParameter* clone() const { return new ActionSchemaParameter(*this); }
	
	//! Processes a possibly nested unprocessed term, consolidating the existing state variables
	//! and binding action parameters to concrete language constants.
	Term::cptr process(const Signature& signature, const ObjectIdxVector& binding, const ProblemInfo& info) const;
	
	//! Prints a representation of the object to the given stream.
	std::ostream& print(std::ostream& os, const ProblemInfo& info) const;
	
protected:
	//! The position of the parameter within the ordered set of action parameters
	unsigned _position;
	
	//! The name of the parameter
	std::string _name;
};

//! A simple constant
class ConstantSchema : public TermSchema {
public:
	ConstantSchema(ObjectIdx value)  : _value(value) {}
	
	ConstantSchema* clone() const { return new ConstantSchema(*this); }
	
	//! Processes a possibly nested unprocessed term, consolidating the existing state variables
	//! and binding action parameters to concrete language constants.
	Term::cptr process(const Signature& signature, const ObjectIdxVector& binding, const ProblemInfo& info) const;

	//! Prints a representation of the object to the given stream.
	std::ostream& print(std::ostream& os, const ProblemInfo& info) const;
	
protected:
	//! The actual value of the constant
	ObjectIdx _value;
};

class IntConstantSchema : public ConstantSchema {
public:
	IntConstantSchema(ObjectIdx value)  : ConstantSchema(value) {}
	
	IntConstantSchema* clone() const { return new IntConstantSchema(*this); }
	
	//! Processes a possibly nested unprocessed term, consolidating the existing state variables
	//! and binding action parameters to concrete language constants.
	Term::cptr process(const Signature& signature, const ObjectIdxVector& binding, const ProblemInfo& info) const;

	//! Prints a representation of the object to the given stream.
	std::ostream& print(std::ostream& os, const ProblemInfo& info) const;
};


class AtomicFormulaSchema {
public:
	typedef const AtomicFormulaSchema* cptr;

	AtomicFormulaSchema(const std::string& symbol ,const std::vector<TermSchema::cptr>& subterms) : _symbol(symbol), _subterms(subterms) {}	
	
	virtual ~AtomicFormulaSchema() { 
		for (const auto ptr:_subterms) delete ptr;
	}
	
	virtual AtomicFormula::cptr process(const Signature& signature, const ObjectIdxVector& binding, const ProblemInfo& info) const;
	
	//! Prints a representation of the object to the given stream.
	friend std::ostream& operator<<(std::ostream &os, const AtomicFormulaSchema& o) { return o.print(os); }
	std::ostream& print(std::ostream& os) const;
	virtual std::ostream& print(std::ostream& os, const fs0::ProblemInfo& info) const;
	
protected:
	//! The symbol identifying the external method
	const std::string _symbol;
	
	std::vector<TermSchema::cptr> _subterms;
};


class ActionEffectSchema {
public:
	typedef const ActionEffectSchema* cptr;
	
	ActionEffectSchema(TermSchema::cptr lhs_, TermSchema::cptr rhs_) : lhs(lhs_), rhs(rhs_) {}
	
	virtual ~ActionEffectSchema() {
		delete lhs; delete rhs;
	}
	
	ActionEffect::cptr process(const Signature& signature, const ObjectIdxVector& binding, const ProblemInfo& info) const;
	
	//! Prints a representation of the object to the given stream.
	friend std::ostream& operator<<(std::ostream &os, const ActionEffectSchema& o) { return o.print(os); }
	std::ostream& print(std::ostream& os) const;
	virtual std::ostream& print(std::ostream& os, const fs0::ProblemInfo& info) const;
	
	TermSchema::cptr lhs;
	TermSchema::cptr rhs;
protected:
};

} } } // namespaces

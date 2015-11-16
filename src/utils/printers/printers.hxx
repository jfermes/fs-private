
#pragma once

#include <vector>
#include <ostream>
#include <problem_info.hxx>
#include <actions/ground_action.hxx>
#include <actions/action_id.hxx>
#include <boost/units/detail/utility.hpp>

namespace fs0 {

class Problem;
class SupportedAction;

//! Print a plan
class PlanPrinter {
protected:
	const ActionPlan& _plan;
public:
	PlanPrinter(const ActionPlan& plan) : _plan(plan) {}
	
	//! Prints a representation of the state to the given stream.
	friend std::ostream& operator<<(std::ostream &os, const PlanPrinter& o) { return o.print(os); }
	std::ostream& print(std::ostream& os) const;
	
	static void printSupportedPlan(const std::set<SupportedAction>& plan, std::ostream& out);
	
	//! static helpers
	static void printPlan(const std::vector<GroundAction::IdType>& plan, const Problem& problem, std::ostream& out);
	static void printPlanJSON(const std::vector<GroundAction::IdType>& plan, const Problem& problem, std::ostream& out);	
};




namespace print {

class plan {
	protected:
		const plan_t& _plan;

	public:
		plan(const plan_t& plan_) : _plan(plan_) {}
		
		friend std::ostream& operator<<(std::ostream &os, const plan& o) { return o.print(os); }
		std::ostream& print(std::ostream& os) const;
};

class supported_plan {
protected:
	const std::set<SupportedAction>& _plan;
public:
	supported_plan(const std::set<SupportedAction>& plan) : _plan(plan) {}
	
	//! Prints a representation of the state to the given stream.
	friend std::ostream& operator<<(std::ostream &os, const supported_plan& o) { return o.print(os); }
	std::ostream& print(std::ostream& os) const {
		printSupportedPlan(_plan, os);
		return os;
	}
	
	static void printSupportedPlan(const std::set<SupportedAction>& plan, std::ostream& out);
};

//! Print the proper, demangled name of a std::type_info / std::type_index object
template <typename T>
std::string type_info_name(const T& type) { return boost::units::detail::demangle(type.name()); }

}

} // namespaces

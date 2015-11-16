
#pragma once

#include <constraints/gecode/handlers/base_action_handler.hxx>

namespace fs0 { namespace gecode {


//! A CSP modeling and solving the effect of an action on a certain RPG layer
class GroundActionCSPHandler : public BaseActionCSPHandler {
public:
	typedef GroundActionCSPHandler* ptr;
	
	//! Factory method
	static std::vector<std::shared_ptr<BaseActionCSPHandler>> create(const std::vector<const GroundAction*>& actions);

	//! Constructors / Destructor
	GroundActionCSPHandler(const GroundAction& action, bool approximate, bool use_novelty_constraint);
	GroundActionCSPHandler(const GroundAction& action, const std::vector<fs::ActionEffect::cptr>& effects, bool approximate, bool use_novelty_constraint);
	virtual ~GroundActionCSPHandler() {}

protected:

	const ActionID* get_action_id(SimpleCSP* solution) const;
	
	//! Log some handler-related into
	virtual void log() const;
};

} } // namespaces

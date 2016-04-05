
#include <constraints/gecode/handlers/ground_action_handler.hxx>
#include <utils/logging.hxx>
#include <actions/actions.hxx>
#include <actions/action_id.hxx>

namespace fs0 { namespace gecode {
	
std::vector<std::shared_ptr<BaseActionCSPHandler>> GroundActionCSPHandler::create(const std::vector<GroundAction::cptr>& actions, const TupleIndex& tuple_index, bool approximate, bool novelty) {
	std::vector<std::shared_ptr<BaseActionCSPHandler>> managers;
	
	for (unsigned idx = 0; idx < actions.size(); ++idx) {
		// auto x = new GroundActionCSPHandler(*actions[idx], approximate, novelty); std::cout << *x << std::endl;
		auto manager = std::make_shared<GroundActionCSPHandler>(*actions[idx], tuple_index, approximate);
		manager->init(novelty);
		FDEBUG("main", "Generated CSP for action " << *actions[idx] << std::endl <<  *manager << std::endl);
		managers.push_back(manager);
	}
	return managers;
}

// If no set of effects is provided, we'll take all of them into account
GroundActionCSPHandler::GroundActionCSPHandler(const GroundAction& action, const TupleIndex& tuple_index, bool approximate)
	:  BaseActionCSPHandler(action, action.getEffects(), tuple_index, approximate)
{}

const ActionID* GroundActionCSPHandler::get_action_id(const SimpleCSP* solution) const {
	return new PlainActionID(&_action);
}

void GroundActionCSPHandler::log() const {
	FFDEBUG("heuristic", "Processing action #" << _action.getId() << ": " << _action.fullname());
}

} } // namespaces

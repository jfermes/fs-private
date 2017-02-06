/*
 * This is largely based verbatim on Fast Downward's GPL'd Planner signal-handling routine:
 * http://www.fast-downward.org/ObtainingAndRunningFastDownward
 */

#pragma once
#include <stddef.h>

/*
 * Author:  David Robert Nadeau
 * Site:    http://NadeauSoftware.com/
 * License: Creative Commons Attribution 3.0 Unported License
 *          http://creativecommons.org/licenses/by/3.0/deed.en_US
 */
namespace fs0 { namespace utils {

size_t getCurrentRSS( );
size_t getPeakRSS( );
	
	
} } // namespaces


namespace fs0 {

enum ExitCode {
    PLAN_FOUND = 0,
    CRITICAL_ERROR = 1,
    INPUT_ERROR = 2,
    UNSUPPORTED = 3,
    UNSOLVABLE = 4,
    UNSOLVED_INCOMPLETE = 5, // Search ended without finding a solution.
    OUT_OF_MEMORY = 6
};

int get_peak_memory_in_kb();
const char *get_exit_code_message_reentrant(int exitcode);
bool is_exit_code_error_reentrant(int exitcode);
void register_event_handlers();
int get_process_id();

size_t get_current_memory_in_kb();

} // namespaces


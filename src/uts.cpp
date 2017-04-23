#include "uts.hpp"
#include "utils.hpp"
#include "utils_ns.hpp"

#include <string>
#include <unistd.h>
#include <sched.h>

using std::string;

int uts_setup_ns() {
	int ret;
	static string const HOST("container");
	CALL(ret, sethostname(HOST.c_str(), HOST.size()),
			"Failed to set hostname", (void) 0);
	// Jobsdone -- Peasant
	return ret;
}

int uts_open(container_id const& id) {
	return ns_open(id, "uts");
}

int uts_enter_ns(int fd) {
	return ns_enter(fd);
}

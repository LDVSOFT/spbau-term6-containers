#include "pid.hpp"
#include "utils.hpp"
#include "utils_ns.hpp"

#include <sched.h>

int pid_open(container_id const& id) {
	return ns_open(id, "pid");
}

int pid_enter_ns(int fd) {
	return ns_enter(fd);
}

#include "ipc.hpp"
#include "utils.hpp"
#include "utils_ns.hpp"

#include <sched.h>

int ipc_open(container_id const& id) {
	return ns_open(id, "ipc");
}

int ipc_enter_ns(int fd) {
	return ns_enter(fd);
}

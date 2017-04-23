#include "container_enter.hpp"
#include "utils.hpp"
#include "log.hpp"
#include "net.hpp"
#include "ipc.hpp"
#include "mount.hpp"
#include "user.hpp"
#include "pid.hpp"
#include "uts.hpp"
#include "cpu.hpp"
#include "syscalls.hpp"

#include <unistd.h>

using std::vector;
using std::string;

int container_enter(container_id const& id, vector<string> const& cmd) {
	int ret, net, ipc, mount, user, pid, uts;

	print_log("Opening namespaces...");
	CALL_(net, net_open(id),
			"Failed to open net namespace", return -1);
	Defer(close(net));

	CALL_(ipc, ipc_open(id),
			"Failed to open IPC namespace", return -1);
	Defer(close(ipc));

	CALL_(mount, mount_open(id),
			"Failed to open mount namespace", return -1);
	Defer(close(mount));

	CALL_(user, user_open(id),
			"Failed to open user namespace", return -1);
	Defer(close(user));

	CALL_(pid, pid_open(id),
			"Failed to open pid namespace", return -1);
	Defer(close(pid));

	CALL_(uts, uts_open(id),
			"Failed to open UTS namespace", return -1);
	Defer(close(uts));

	// Enter CPU
	print_log("Entering cgroups...");
	CALL_(ret, cpu_enter_cgroup(id),
			"Failed to enter CPU cgroup", return -1);

	print_log("Entering namespaces...");
	CALL_(ret, ipc_enter_ns(ipc),
			"Failed to enter IPC namespace", return -1);

	CALL_(ret, mount_enter_ns(mount),
			"Failed to enter mount namespace", return -1);

	CALL_(ret, pid_enter_ns(pid),
			"Failed to enter pid namespace", return -1);

	CALL_(ret, uts_enter_ns(uts),
			"Failed to enter UTS namespace", return -1);

	CALL_(ret, net_enter_ns(net),
			"Failed to enter net namespace", return -1);

	CALL_(ret, user_enter_ns(user),
			"Failed to enter user namespace", return -1);

	print_log("Exec...");
	CALL(ret, exec(cmd),
			"Failed to exec", return -1);
	// Unreachable
	return -1;
}

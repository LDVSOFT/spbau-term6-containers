#include "user.hpp"
#include "utils.hpp"
#include "utils_ns.hpp"
#include "log.hpp"

#include <string>
#include <sys/fsuid.h>
#include <unistd.h>
#include <fcntl.h>
#include <grp.h>
#include <sched.h>

using std::string;
using std::to_string;
using namespace std::literals;

static int write_mapping(string const& path, int id) {
	int ret, fd;
	string map("0 "s + to_string(id) + " 1\n"s);
	print_log("path: %s", path.c_str());
	CALL(fd, open(path.c_str(), O_WRONLY),
		"Failed to open ?id_map", return -1);
	Defer(CALL(ret, close(fd),
				"Failed to close ?id_map", (void) 0));

	CALLv(ssize_t(map.size()), ret, write(fd, map.c_str(), map.size()),
		"Failed to write ?id_map", return -1);
	return 0;
}

int user_setup_ns_monitor(container_opts const& opts) {
	int ret;
	string base_path("/proc/"s + to_string(opts.id.pid) + "/"s);

	CALL_(ret, write_mapping(base_path + "uid_map"s, opts.uid),
			"Failed to write uid_map", return -1);

	CALL_(ret, write_mapping(base_path + "gid_map"s, opts.gid),
			"Failed to write gid_map", return -1);

	return 0;
}

int user_setup_ns_child() {
	int ret;
	print_log("Current uid/gid: %d/%d", getuid(), getgid());
	CALL(ret, setreuid(0, 0),
			"Failed to set {r,e}uid", return -1);
	CALL(ret, setregid(0, 0),
			"Failed to set {r,e}gid", return -1);
	CALL(ret, setgroups(0, NULL),
			"Failed to reset groups", return -1);
	setfsuid(0);
	CALL(ret, setfsuid(0),
			"Failed to check fsuid", return -1);
	setfsgid(0);
	CALL(ret, setfsgid(0),
			"Failed to check fsgid", return -1);

	print_log("Current uid/gid: %d/%d", getuid(), getgid());
	return 0;
}

int user_open(container_id const& id) {
	return ns_open(id, "user");
}

int user_enter_ns(int fd) {
	int ret;
	CALL_(ret, ns_enter(fd),
			"Failed to enter user namespace", return -1);

	CALL_(ret, user_setup_ns_child(),
			"Failed to setup user namespace", return -1);

	return 0;
}

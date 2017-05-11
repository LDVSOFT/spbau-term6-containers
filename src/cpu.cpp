#include "cpu.hpp"
#include "utils.hpp"

#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

static int const SCHEDULING_BASE(1e4);

using std::string;
using std::to_string;
using namespace std::literals;

static string get_cpu_cgroup_name(container_id const& id) {
	return "/aucont/cgroup/"s + id.name() + "/"s;
}

int cpu_setup_cgroup(container_opts const& opts) {
	string path(get_cpu_cgroup_name(opts.id));
	int ret(0), fd;
	CALL(ret, mkdir(path.c_str(), S_IRWXO | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH),
			"Failed to create cgroup", return ret = -1);
	Defer(if (ret != 0) cpu_destroy(opts));

	/* period */ {
		string filepath(path + "cpu.cfs_period_us");
		CALL(fd, open(filepath.c_str(), O_WRONLY),
				"Failed to open cfs_period_us", return ret = -1);
		Defer(close(fd));
		string msg(to_string(SCHEDULING_BASE) + "\n"s);
		CALLv(ssize_t(msg.size()), ret, write(fd, msg.c_str(), msg.size()),
				"Failed to write to cfs_period_us", return ret = -1);
	}

	/* quota */ {
		string filepath(path + "cpu.cfs_quota_us");
		CALL(fd, open(filepath.c_str(), O_WRONLY),
				"Failed to open cfs_quota_us", return ret = -1);
		Defer(close(fd));
		string msg(to_string(sysconf(_SC_NPROCESSORS_ONLN) * SCHEDULING_BASE * opts.cpu_limit / 100) + "\n"s);
		CALLv(ssize_t(msg.size()), ret, write(fd, msg.c_str(), msg.size()),
				"Failed to write to cfs_quota_us", return ret = -1);
	}
	/* tasks */ {
		string filepath(path + "tasks");
		CALL(fd, open(filepath.c_str(), O_WRONLY),
				"Failed to open tasks", return ret = -1);
		Defer(close(fd));
		string msg(to_string(opts.id.pid) + "\n"s);
		CALLv(ssize_t(msg.size()), ret, write(fd, msg.c_str(), msg.size()),
				"Failed to write to tasks", return ret = -1);
	}
	return ret = 0;
}

int cpu_destroy(container_opts const& opts) {
	string path(get_cpu_cgroup_name(opts.id));
	int ret;
	CALL(ret, rmdir(path.c_str()),
			"Failed to remove cgroup", return -1);
	return 0;
}

int cpu_enter_cgroup(container_id const& id) {
	string path(get_cpu_cgroup_name(id));
	string filepath(path + "tasks");
	int ret, fd;

	CALL(fd, open(filepath.c_str(), O_WRONLY),
			"Failed to open tasks", return ret = -1);
	Defer(close(fd));
	string msg(to_string(getpid()) + "\n"s);
	CALLv(ssize_t(msg.size()), ret, write(fd, msg.c_str(), msg.size()),
			"Failed to write to tasks", return ret = -1);

	return ret = 0;
}

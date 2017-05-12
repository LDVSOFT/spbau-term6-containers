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
	int ret(0);
	CALL(ret, mkdir(path.c_str(), S_IRWXO | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH),
			"Failed to create cgroup", return ret = -1);
	Defer(if (ret != 0) cpu_destroy(opts));

	CALL_(ret, write_line_to_file(path + "cpu.cfs_period_us"s, to_string(SCHEDULING_BASE)),
			"Failed to write CPU period", return -1);

	CALL_(ret, write_line_to_file(path + "cpu.cfs_quota_us"s, to_string(sysconf(_SC_NPROCESSORS_ONLN) * SCHEDULING_BASE * opts.cpu_limit / 100)),
			"Failed to write CPU quota", return -1);

	CALL_(ret, write_line_to_file(path + "tasks", to_string(opts.id.pid)),
			"Failed to switch CPU cgroup", return -1);

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
	int ret(0);

	CALL_(ret, write_line_to_file(get_cpu_cgroup_name(id) + "tasks", to_string(getpid())),
			"Failed to swtich CPU cgroup", ret = -1);

	return ret;
}

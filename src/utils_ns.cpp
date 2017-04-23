#include "utils_ns.hpp"
#include "utils.hpp"

#include <string>
#include <sys/stat.h>
#include <fcntl.h>
#include <sched.h>

using std::string;
using std::to_string;
using namespace std::literals;

int ns_open(container_id const& id, char const* s) {
	int ret;
	string path("/proc/"s + to_string(id.pid) + "/ns/"s + s);
	CALL(ret, open(path.c_str(), O_RDONLY),
			"Failed to open ns", return -1);
	return ret;
}

int ns_enter(int fd) {
	int ret;
	CALL(ret, setns(fd, 0),
			"Failed to enter ns", return -1);
	return 0;
}

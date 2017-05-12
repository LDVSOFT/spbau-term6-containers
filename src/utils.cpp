#include "utils.hpp"

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

using std::string;

int write_line_to_file(string const& path, string const& contents) {
	int fd;
	ssize_t res;
	CALL(fd, open(path.c_str(), O_WRONLY),
			"Failed top open file", return -1);
	Defer(close(fd));

	string s(contents + "\n");
	CALLv((ssize_t)s.size(), res, write(fd, s.c_str(), s.size()),
			"Failed to write string", return -1);
	return 0;
}

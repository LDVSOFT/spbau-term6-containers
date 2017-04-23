#include "rootfs.hpp"
#include "syscalls.hpp"
#include "utils.hpp"

#include <string>
#include <sys/stat.h>

using namespace std::literals;
using std::string;

static string escape_single_quotes(string const& s) {
	string result;
	result.reserve(s.size());
	for (char c: s) {
		if (c == '\\' || c == '\'')
			result += '\\';
		result += c;
	}
	return result;
}

string rootfs_get_path(container_opts const& opts) {
	return "/aucont/fs/"s + opts.id.name();
}

static int rootfs_destroy_at(string const& path) {
	int ret;
	SYSTEM(ret, "rm -rf "s + path,
			"Failed to destroy rootfs", (void) 0);
	return ret;
}

int rootfs_create(container_opts const& opts) {
	int ret(0);
	string rootfs_path(rootfs_get_path(opts));
	string escaped(escape_single_quotes(opts.rootfs_path));

	CALL(ret, mkdir(rootfs_path.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH),
			"Failed to create rootfs directory", return -1);
	Defer(if (ret != 0) rootfs_destroy_at(rootfs_path));

	string cmd("id && (cd '"s + escaped + "' && tar cf - .) | (cd "s + rootfs_path + " && tar xf -)"s);
	print_log("command to execute: %s", cmd.c_str());
	SYSTEM(ret, cmd,
			"Failed to copy rootfs", (void) 0);
	return ret;
}

int rootfs_destroy(container_opts const& opts) {
	return rootfs_destroy_at(rootfs_get_path(opts));
}


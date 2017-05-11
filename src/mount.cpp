#include "mount.hpp"
#include "utils.hpp"
#include "utils_ns.hpp"
#include "rootfs.hpp"
#include "syscalls.hpp"

#include <string>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sched.h>

using namespace std::literals;
using std::string;

int mount_setup_ns(container_opts const& opts) {
	static const string OLDROOT("/.old");

	int ret(0);
	string rootfs_path(rootfs_get_path(opts));
	CALL(ret, mount(rootfs_path.c_str(), rootfs_path.c_str(), NULL, MS_BIND, NULL),
			"Failed to bind rootfs", return -1);

	string oldroot(rootfs_path + OLDROOT), fs_path;
	CALL(ret, mkdir(oldroot.c_str(), S_IRWXU),
			"Failed to create directory for old root", return -1);

	CALL(ret, pivot_root(rootfs_path.c_str(), oldroot.c_str()),
			"Failed to pivot root in new mount namespace", return -1);
	rootfs_path = "/"s;
	oldroot = OLDROOT;

	CALL(ret, chdir(rootfs_path.c_str()),
			"Failed to cd to root", return -1);

	CALL(ret, chroot(rootfs_path.c_str()),
			"Failed to reset chroot", return -1);

	fs_path = rootfs_path + "/proc"s;
	CALL(ret, mount("proc", fs_path.c_str(), "proc", 0, NULL),
			"Failed to mount /proc", return -1);

	fs_path = rootfs_path + "/sys"s;
	CALL(ret, mount("sysfs", fs_path.c_str(), "sysfs", 0, NULL),
			"Failed to mount /sys", return -1);

	CALL(ret, umount2(oldroot.c_str(), MNT_DETACH),
			"Failed to umount old root", return -1);

	CALL(ret, rmdir(oldroot.c_str()),
			"Failed to remove directory for old root", return -1);

	return 0;
}

int mount_open(container_id const& id) {
	return ns_open(id, "mnt");
}

int mount_enter_ns(int fd) {
	int ret;
	CALL_(ret, ns_enter(fd),
			"Failed to enter mount namespace", return -1);

	CALL(ret, chdir("/"),
			"Failed to cd to root", return -1);

	CALL(ret, chroot("/"),
			"Failed to reset chroot", return -1);

	return 0;
}

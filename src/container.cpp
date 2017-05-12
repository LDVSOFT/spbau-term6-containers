#include "container.hpp"
#include "rootfs.hpp"
#include "mount.hpp"
#include "user.hpp"
#include "uts.hpp"
#include "net.hpp"
#include "cpu.hpp"
#include "tty.hpp"
#include "utils.hpp"
#include "syscalls.hpp"
#include "log.hpp"

#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sched.h>
#include <signal.h>
#include <fcntl.h>

using namespace std::literals;
using std::to_string;
using std::string;

string container_id::name() const {
	return "aucont-"s + to_string(pid);
}

int const container_opts::CPU_NO_LIMIT(-1);
string const container_opts::NET_NO_IP;

/* There are up to three processes involving:
 * 1. aucont_start itself;
 * 2. monitor process: waits for container to stop and cleans resources;
 * 3. child process: init of a container.
 * First two may be one together, monitor is only created in daemon mode.
 */

int const ERR_SUCCESS(0);
int const ERR_ME(-1);
int const ERR_OTHER(-2);
int const ERR_EOF(-200);

static int write_int(int fd, int value) {
	return write(fd, &value, sizeof(value)) != sizeof(value);
}

static int read_int(int fd) {
	int res;
	if (read(fd, &res, sizeof(res)) == 0)
		return ERR_EOF;
	return res;
}

static void at_child(container_opts const& opts, int fd) {
	log_tag("child");
	int ret;
	Defer(if (ret == ERR_ME) write_int(fd, ERR_ME); close(fd));

	// 1-5. ...
	ret = read_int(fd);
	if (ret != 0) {
		print_log("Monitor aborted");
		return;
	}

	// 6. User
	print_log("Setup user namespace (child)...");
	CALL_(ret, user_setup_ns_child(),
			"Failed to setup user namespace (child)", ret = ERR_ME; return);

	// 7. Mount
	print_log("Setup mount namespace...");
	CALL_(ret, mount_setup_ns(opts),
			"Failed to setup mount namespace", ret = ERR_ME; return);

	// 8. UTS
	print_log("Setup UTS namespace...");
	CALL_(ret, uts_setup_ns(),
			"Failed to setup UTS namespace", ret = ERR_ME; return);

	// 9. Daemon
	CALL_(ret, tty_setup_session(opts),
			"Failed to setup session", ret = ERR_ME; return);

	write_int(fd, ERR_SUCCESS);
	// 10. Exec
	print_log("Exec...");
	CALL_(ret, exec(opts.init_cmd),
			"Failed to exec", ret = ERR_ME; return);
}

static int at_monitor(container_opts const& opts, int fd, int parent_fd) {
	int ret(0), ret_;
	void* err(NULL);

	// 1. Setup control group
	print_log("Setup cpu cgroup...");
	CALL_(ret, cpu_setup_cgroup(opts),
			"Failed to setup cpu cgroup", ret = ERR_ME; err = &&exit_none; goto exit);

	// 2. Setup user namespace
	print_log("Setup user namespace (monitor)...");
	CALL_(ret, user_setup_ns_monitor(opts),
			"Failed to setup user namespace", ret = ERR_ME; err = &&exit_cgroup; goto exit);

	// 3. Setup net namespace
	if (opts.net_ip != opts.NET_NO_IP) {
		print_log("Setup net namespace...");
		CALL_(ret, net_setup_ns(opts),
				"Failed to setup net namespace", ret = ERR_ME; err = &&exit_cgroup; goto exit);
	}

	// 4. Setup rootfs
	print_log("Setup rootfs...");
	CALL_(ret, rootfs_create(opts),
			"Failed to create rootfs", ret = ERR_ME; err = &&exit_net; goto exit);

	// 5. Create job
	print_log("Creating & switching job...");
	CALL_(ret, tty_setup_job(opts),
			"Failed to setup job", ret = ERR_ME; err = &&exit_rootfs; goto exit);

	// 6-9. ...
	write_int(fd, ERR_SUCCESS);
	ret = read_int(fd);
	if (ret != ERR_SUCCESS) {
		print_log("Child failed before exec");
		ret = ERR_OTHER;
	} else {
		// 10. ...
		ret = read_int(fd);
		if (ret == ERR_EOF) {
			print_log("Child successfully exec-ed");
			ret = ERR_SUCCESS;
		} else {
			print_log("Child aborted at exec");
			ret = ERR_OTHER;
		}
	}
	if (parent_fd != -1) {
		write_int(parent_fd, ret);
		if (ret == ERR_SUCCESS) {
			write_int(parent_fd, opts.id.pid);
		}
		close(parent_fd);
	} else if (ret == ERR_SUCCESS) {
		printf("%d\n", opts.id.pid);
	}

	int status;
	if (ret == ERR_SUCCESS) {
		CALL(ret_, tty_setup_session(opts),
				"Failed to setup session", (void) 0);
		waitpid(opts.id.pid, &status, 0);
	}

	CALL(ret_, tty_destroy_job(opts),
			"Failed to destroy job", if (ret == ERR_SUCCESS) ret = ERR_OTHER);
	if (ret != ERR_ME) {
		print_log("Child exited, status = %d", WEXITSTATUS(status));
	}

exit:
	if (ret == ERR_ME) {
		write_int(fd, ERR_ME);
		close(fd);
		print_log("Waiting for child to terminate");
		waitpid(opts.id.pid, &status, 0);
	}
	if (err != NULL)
		goto *err;
exit_rootfs:
	print_log("Destroying rootfs...");
	CALL_(ret_, rootfs_destroy(opts),
			"Failed to destroy rootfs", if (ret == ERR_SUCCESS) ret = ERR_OTHER);
exit_net:
	if (opts.net_ip != opts.NET_NO_IP) {
		print_log("Destroying net namespace...");
		CALL_(ret_, net_destroy(opts),
				"Failed to destroy net namespace", if (ret == ERR_SUCCESS) ret = ERR_OTHER);
	}
exit_cgroup:
	print_log("Destroying cpu cgroup...");
	CALL_(ret_, cpu_destroy(opts),
			"Failed to destroy cpu cgroup", if (ret == ERR_SUCCESS) ret = ERR_OTHER);
exit_none:
	return ret;
}

static int monitor(container_opts& opts, int parent_fd = -1) {
	log_tag("monitor");

	int ret, fd[2];
	/* elevate to full-root; else system(3) is UB... */ {
		uid_t uid, euid, suid;
		gid_t gid, egid, sgid;

		CALL(ret, getresuid(&uid, &euid, &suid),
				"Cannot gain user credentials", return -1);
		CALL(ret, getresgid(&gid, &egid, &sgid),
				"Cannot gain group credentials", return -1);

		if (euid != 0) {
			print_log("ERROR: Have not got root credentials!");
			return -1;
		}
		opts.uid = uid;
		opts.gid = gid;
		CALL(ret, setreuid(0, 0),
				"Failed to elevate user credentials to full-root", return -1);
		CALL(ret, setregid(0, 0),
				"Failed to elevate group credentials to full-root", return -1);
	}

	CALL(ret, tty_stat(opts),
			"Failed to check for interactivity", return -1);
	umask(0);

	CALL(ret, socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, fd),
			"Failed to open monitor-child socket", return -1);

	pid_t pid;
	int flags(SIGCHLD | CLONE_NEWUSER | CLONE_NEWNS | CLONE_NEWPID
			| CLONE_NEWUTS | CLONE_NEWIPC | CLONE_NEWNET);
	CALL(pid, clone(flags),
			"Failed to fork child", return -1);

	if (pid > 0) {
		close(fd[1]);
		print_log("Child forked: %d", pid);
		opts.id.pid = pid;
		print_log("Container name: %s", opts.id.name().c_str());
		write_int(fd[0], pid);
		return at_monitor(opts, fd[0], parent_fd);
	} else {
		close(fd[0]);
		opts.id.pid = read_int(fd[1]);
		at_child(opts, fd[1]);
		// Unreachable!
		exit(1);
	}
}

int container(container_opts& opts) {
	int ret;

	if (!opts.daemonize) {
		// Parent is the monitor
		return monitor(opts);
	}

	int pipe_fd[2];
	CALL(ret, pipe2(pipe_fd, O_CLOEXEC),
			"Failed to open parent-monitor pipe", return -1);

	pid_t pid;
	CALL(pid, fork(),
			"Failed to fork monitor", return -1);

	if (pid > 0) {
		close(pipe_fd[1]);

		print_log("Monitor forked: %d", pid);
		ret = read_int(pipe_fd[0]);
		print_log("Start status: %d", ret);
		if (ret == ERR_SUCCESS) {
			opts.id.pid = read_int(pipe_fd[0]);
			printf("%d\n", opts.id.pid);
		}
		close(pipe_fd[0]);
		return ret;
	} else {
		close(pipe_fd[0]);
		ret = monitor(opts, pipe_fd[1]);
		write_int(pipe_fd[1], ret);
		close(pipe_fd[1]);
		exit(0);
	}
}

#include "tty.hpp"
#include "utils.hpp"
#include "log.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

int tty_stat(container_opts& opts) {
	int ret;
	CALL(ret, isatty(STDIN_FILENO),
			"Failed to test input for a tty", return -1);
	opts.interactive = ret == 1;
	return 0;
}

static bool was_log_stderr;

int tty_setup_session(container_opts const& opts) {
	int ret, fd;
	if (!opts.daemonize) {
		return 0;
	}

	CALL(fd, open("/dev/null", O_RDWR),
			"Failed to open /dev/null", return -1);

	CALL(ret, setsid(),
			"Failed to setup session", return -1);

	dup2(fd, STDIN_FILENO);
	dup2(fd, STDOUT_FILENO);
	dup2(fd, STDERR_FILENO);
	close(fd);
	log_remove(stderr);

	return 0;
}

int tty_setup_job(container_opts const& opts) {
	int ret;
	if (!opts.interactive || opts.daemonize) {
		return 0;
	}

	CALL(ret, setpgid(opts.id.pid, opts.id.pid),
			"Failed to create child job", return -1);

	CALL(ret, tcsetpgrp(STDIN_FILENO, opts.id.pid),
			"Failed to set child job to foreground", return -1);
	was_log_stderr = log_remove(stderr);

	return 0;
}

int tty_destroy_job(container_opts const& opts) {
	if (!opts.interactive || opts.daemonize) {
		return 0;
	}

	int ret;
	struct sigaction old_handler, new_handler;
	new_handler.sa_flags = 0;
	new_handler.sa_handler = SIG_IGN;
	CALL(ret, sigemptyset(&new_handler.sa_mask),
			"Failed to clean sa_mask", return -1);

	CALL(ret, sigaction(SIGTTOU, &new_handler, &old_handler),
			"Failed to mute SIGTTOU", return -1);

	CALL(ret, tcsetpgrp(STDIN_FILENO, getpid()),
			"Failed to become foreground", return -1);
	if (was_log_stderr)
		log_insert(stderr);

	CALL(ret, sigaction(SIGTTOU, &old_handler, NULL),
			"Failed to restore SIGTTOU handler", return -1);

	return 0;
}

#include "container_stop.hpp"
#include "utils.hpp"
#include "log.hpp"

#include <cstring>
#include <string>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/connector.h>
#include <linux/cn_proc.h>
#include <unistd.h>
#include <signal.h>

using std::string;
using std::to_string;
using std::stoi;
using namespace std::literals;

static int netlink_switch(int fd, bool enable) {
	struct __attribute__((aligned(NLMSG_ALIGNTO))) {
		struct nlmsghdr header;
		struct __attribute__((packed)) {
			struct cn_msg message;
			enum proc_cn_mcast_op multicast;
		};
	} msg;

	int ret;
	memset(&msg, 0, sizeof(msg));
	msg.header.nlmsg_len = sizeof(msg);
	msg.header.nlmsg_pid = getpid();
	msg.header.nlmsg_type = NLMSG_DONE;
	msg.message.id.idx = CN_IDX_PROC;
	msg.message.id.val = CN_VAL_PROC;
	msg.message.len = sizeof(msg.multicast);
	msg.multicast = enable ? PROC_CN_MCAST_LISTEN : PROC_CN_MCAST_IGNORE;

	CALL(ret, send(fd, &msg, sizeof(msg), 0),
			"Failed to send control message", return -1);

	return 0;
}

static pid_t netlink_get_exited(int fd) {
	struct __attribute__((aligned(NLMSG_ALIGNTO))) {
		struct nlmsghdr header;
		struct __attribute__((packed)) {
			struct cn_msg message;
			struct proc_event event;
		};
	} msg;

	int ret;
	CALLc(> 0, ret, recv(fd, &msg, sizeof(msg), 0),
			"Failed to read netlink message", return -1);

	if (msg.event.what != proc_event::PROC_EVENT_EXIT)
		return 0;
	pid_t res(msg.event.event_data.exit.process_pid);
	print_log("Netlink reported exit of %d", res);
	return res;
}

int container_kill_and_wait(container_id const& id, int signal) {
	int ret;
	pid_t monitor;

	/* get ppid */ {
		FILE* fd;
		string cmd("grep -F 'PPid:' /proc/"s + to_string(id.pid) + "/status | grep -oP '[0-9]+$'"s);
		CALLc(!= NULL, fd, popen(cmd.c_str(), "r"),
				"Failed to popen", return -1);
		string res;
		while (!feof(fd)) {
			int const BUFFER(128);
			static char buffer[BUFFER];
			if (fgets(buffer, BUFFER, fd) != NULL)
				res += buffer;
		}
		CALL(ret, pclose(fd),
				"Failed to pclose", return -1);
		ret = WEXITSTATUS(ret);
		if (ret == 1) {
			print_log("Grepping PPid failed; container is already dead.");
			return 0;
		}
		monitor = stoi(res);
		print_log("Located monitor %d", monitor);
	}

	int netlink;
	/* open netlink */ {
		CALL(netlink, socket(AF_NETLINK, SOCK_DGRAM, NETLINK_CONNECTOR),
				"Failed to open netlink socket", return -1);
		Defer(if (ret != 0) close(netlink));

		struct sockaddr_nl sa;
		sa.nl_family = AF_NETLINK;
		sa.nl_groups = CN_IDX_PROC;
		sa.nl_pid = getpid();

		CALL(ret, bind(netlink, (struct sockaddr*)(&sa), sizeof(sa)),
				"Failed to bind netlink socket", return -1);
	}

	Defer(close(netlink));
	CALL_(ret, netlink_switch(netlink, true),
			"Failed to enable netlink", return -1);

	print_log("Killing container with %s", sys_siglist[signal]);
	CALL(ret, kill(id.pid, signal),
			"Failed to kill container", return -1);

	while (true) {
		CALL(ret, netlink_get_exited(netlink),
				"Failed to read exited pid", return -1);
		if (ret == monitor) {
			print_log("Monitor has exited");
			break;
		}
	}

	CALL(ret, netlink_switch(netlink, false),
			"Failed to disable netlink", return -1);

	return 0;
}

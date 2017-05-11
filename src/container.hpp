#pragma once

#include <string>
#include <vector>
#include <sys/types.h>

struct container_id {
	pid_t pid = -1;
	std::string name() const;
};

struct container_opts {
	static int const CPU_NO_LIMIT;
	static std::string const NET_NO_IP;

	container_id id;
	// User who owns the container
	uid_t uid;
	gid_t gid;
	// Daemonize the contaier?
	bool daemonize = false;
	// If provided -- IP of the container
	std::string net_ip = NET_NO_IP;
	// If provided -- CPU limit of the container
	int cpu_limit = CPU_NO_LIMIT;
	// Path to the container rootfs source
	std::string rootfs_path;
	// Init command
	std::vector<std::string> init_cmd;

	// Is the container interactive
	bool interactive;
};

int container(container_opts& opts);

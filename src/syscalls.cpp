#include "syscalls.hpp"
#include "utils.hpp"

#include <cstdlib>
#include <unistd.h>
#include <sys/syscall.h>

using std::string;
using std::vector;

int pivot_root(char const* new_root, char const* put_old) {
	// Unportable as hell, yeah...
	return syscall(SYS_pivot_root, new_root, put_old);
}

pid_t clone(unsigned long flags, void* child_stack, void* ptid, void* ctid, struct pt_regs* regs) {
	return syscall(SYS_clone, flags, child_stack, ptid, ctid, regs);
}

int exec(vector<string> const& command) {
	int ret;
	char const* cmd = command[0].c_str();
	vector<char*> args;
	for (string const& s: command)
		args.push_back(const_cast<char*>(s.c_str())); // Thanks POSIX for missing const👍
	args.push_back(NULL);
	vector<char*> env{NULL};
	CALL(ret, execve(cmd, args.data(), env.data()),
			"Failed to exec", return -1);
	// Unreachable
	exit(1);
}

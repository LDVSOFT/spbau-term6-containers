#pragma once

#include <stdlib.h>
#include <vector>
#include <string>
#include <sched.h>

int pivot_root(char const *new_root, char const *put_old);

pid_t clone(unsigned long flags, void* child_stack = NULL, void* ptid = NULL, void* ctid = NULL, struct pt_regs* regs = NULL);

int exec(std::vector<std::string> const& cmd);

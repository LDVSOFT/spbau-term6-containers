#pragma once

#include "container.hpp"

int pid_open(container_id const& id);
int pid_enter_ns(int fd);

#pragma once

#include "container.hpp"

int ipc_open(container_id const& id);
int ipc_enter_ns(int fd);

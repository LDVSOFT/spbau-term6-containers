#pragma once

#include "container.hpp"

int mount_setup_ns(container_opts const& opts);
int mount_open(container_id const& id);
int mount_enter_ns(int fd);

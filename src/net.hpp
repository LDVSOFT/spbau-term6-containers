#pragma once

#include "container.hpp"

int net_setup_ns(container_opts const& opts);
int net_destroy(container_opts const& opts);
int net_open(container_id const& id);
int net_enter_ns(int fd);

#pragma once

#include "container.hpp"

int user_setup_ns_monitor(container_opts const& opts);
int user_setup_ns_child();
int user_open(container_id const& id);
int user_enter_ns(int fd);

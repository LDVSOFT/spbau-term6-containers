#pragma once

#include "container.hpp"

int cpu_setup_cgroup(container_opts const& opts);
int cpu_destroy(container_opts const& opts);
int cpu_enter_cgroup(container_id const& id);

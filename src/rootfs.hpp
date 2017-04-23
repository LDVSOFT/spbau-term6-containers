#pragma once

#include "container.hpp"

#include <string>

std::string rootfs_get_path(container_opts const& opts);
int rootfs_create(container_opts const& opts);
int rootfs_destroy(container_opts const& opts);

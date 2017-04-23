#pragma once

#include "container.hpp"

int uts_setup_ns();
int uts_open(container_id const& id);
int uts_enter_ns(int fd);

#pragma once

#include "container.hpp"

int ns_open(container_id const& id, const char* s);
int ns_enter(int fd);

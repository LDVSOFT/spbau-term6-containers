#pragma once

#include "container.hpp"

int tty_stat(container_opts& opts);
int tty_setup_session(container_opts const& opts);
int tty_setup_job(container_opts const& opts);
int tty_destroy_job(container_opts const& opts);

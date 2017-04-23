#pragma once

#include <cstdio>

#if 1
	#define print_log(args...) log_(args)
#else
	#define print_log(...) (void) 0
#endif
#define log_(fmtargs...) _log(__FILE__, __LINE__, fmtargs)

void _log(char const* file, int line, char const *fmt, ...);
void log_tag(char const* s);
void log_insert(FILE* target);
bool log_remove(FILE* target);

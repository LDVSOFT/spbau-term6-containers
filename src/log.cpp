#include "log.hpp"

#include <cstdio>
#include <cstdarg>
#include <set>

using std::set;

static char const* tag("???");
static set<FILE*> targets;

void log_insert(FILE* target) {
	targets.insert(target);
}

bool log_remove(FILE* target) {
	return targets.erase(target) > 0;
}

void log_tag(char const* s) {
	tag = s;
}

void _log(char const* file, int line, char const* fmt, ...) {
	va_list orig;
	va_start(orig, fmt);
	for (FILE* target: targets) {
		va_list args;
		va_copy(args, orig);

		fprintf(target, "[%s %s:%d] ", tag, file, line);
		vfprintf(target, fmt, args);
		fprintf(target, "\n");
		fflush(target);

		va_end(args);
	}
	va_end(orig);
}

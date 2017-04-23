#pragma once

#include "log.hpp"

#include <cstdio>
#include <cstring>
#include <cerrno>

#define STRINGIFY(x) #x
#define _CALL(cond, var, call, message, on_error, do_errno) \
	do { \
		var = call; \
		if (!(var cond)) { \
			int e = errno; \
			log_("ERROR (%s=%d): %s", STRINGIFY(var), var, message); \
			if (do_errno && ssize_t(var) < 0) \
				log_("Errno %d: %s", e, strerror(e)); \
			on_error; \
		} \
	} while (0)
#define CALLc(cond, var, call, message, on_error)  _CALL(cond, var, call, message, on_error, 1)
#define CALLc_(cond, var, call, message, on_error) _CALL(cond, var, call, message, on_error, 0)
#define CALLv(value, var, call, message, on_error)  _CALL(== value, var, call, message, on_error, 1)
#define CALLv_(value, var, call, message, on_error) _CALL(== value, var, call, message, on_error, 0)
#define CALL(var, call, message, on_error)  _CALL(>= 0, var, call, message, on_error, 1)
#define CALL_(var, call, message, on_error) _CALL(>= 0, var, call, message, on_error, 0)
#define SYSTEM(var, s, message, on_error) CALLv(0, var, system((s).c_str()), message, var = -1; on_error)

//http://stackoverflow.com/a/38289141/4324545
template <class Lambda> class AtScopeExit {
	Lambda& m_lambda;
public:
	AtScopeExit(Lambda& action) : m_lambda(action) {}
	~AtScopeExit() { m_lambda(); }
};

#define Auto_INTERNAL2(lname, aname, ...) \
	auto lname = [&]() { __VA_ARGS__; }; \
	AtScopeExit<decltype(lname)> aname(lname);

#define Auto_TOKENPASTE(x, y) Auto_ ## x ## y

#define Auto_INTERNAL1(ctr, ...) \
	Auto_INTERNAL2(Auto_TOKENPASTE(func_, ctr), \
	Auto_TOKENPASTE(instance_, ctr), __VA_ARGS__)

#define Defer(...) Auto_INTERNAL1(__COUNTER__, __VA_ARGS__)

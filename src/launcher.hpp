#ifndef LAUNCHER_LAUNCHER_HPP_INCLUDED
#define LAUNCHER_LAUNCHER_HPP_INCLUDED

#include <locenv/api.hpp>
#include <locenv/lua.hpp>

#include <filesystem>
#include <string>

#ifdef _WIN32
#include <processthreadsapi.h>
#else
#include <sys/types.h>
#endif

#define PROCESS_METHOD_TABLE() \
	static void setup(locenv::lua l, int c) \
	{ \
		locenv::lua_new_method_table(l, c, [](auto l, auto c) { \
			locenv::lua_new_method(l, c, "wait", &process::wait); \
		}); \
	}

#ifdef _WIN32
class process final {
public:
	process(PROCESS_INFORMATION pi);
	process(process &&o);
	process(const process &) = delete;
	~process();

public:
	PROCESS_METHOD_TABLE()

public:
	process &operator=(const process &) = delete;

private:
	int wait(locenv::lua l);

private:
	PROCESS_INFORMATION pi;
};
#else
class process final {
public:
	process(pid_t pid);
	process(process &&o);
	process(const process &) = delete;
	~process();

public:
	PROCESS_METHOD_TABLE()

public:
	process &operator=(const process &) = delete;

private:
	int wait(locenv::lua l);

private:
	pid_t pid;
};
#endif

class launcher final {
public:
	std::string binary;
	std::filesystem::path working_directory;

public:
	launcher();
	launcher(const launcher &) = delete;
	~launcher();

public:
	launcher &operator=(const launcher &) = delete;

public:
	process launch();
};

#endif // LAUNCHER_LAUNCHER_HPP_INCLUDED

#include "launcher.hpp"

#include <locenv/loader.hpp>

#include <array>
#include <exception>
#include <optional>
#include <utility>

using locenv::api;
using locenv::lua;
using locenv::lua_reg;
using locenv::lua_upvalue;

static int spawn(lua lua)
{
	auto &context = locenv::context::get(lua, lua_upvalue(1));
	auto binary = api->aux_checklstring(lua, 1, nullptr);
	auto wd = api->aux_optlstring(lua, 2, nullptr, nullptr);

	// Setup launcher.
	launcher l;

	l.binary = binary;

	if (wd) {
		l.working_directory = wd;
	} else {
		l.working_directory = context.working_directory;
	}

	// Launch.
	std::optional<process> p;

	try {
		p.emplace(l.launch());
	} catch (std::exception &e) {
		return api->aux_error(lua, "failed to launch {}: {}", binary, e.what());
	}

	// Wrap the process as Lua object.
	locenv::lua_new_object<process>(lua, lua_upvalue(1), std::move(p).value());
	return 1;
}

static int loader(lua lua)
{
	static const std::array functions{
		lua_reg{ .name = "spawn", .func = spawn },
		lua_reg{ .name = nullptr, .func = nullptr }
	};

	// More information about 'loader': https://www.lua.org/manual/5.4/manual.html#6.3
	// The loader data is locenv::context (the actual object is stored within Lua userdata).
	api->lua_createtable(lua, 0, functions.size() - 1);
	api->lua_pushvalue(lua, 2); // Push the loader data as upvalue for all functions in `functions`.
	api->aux_setfuncs(lua, functions.data(), 1);

	// Return a function table that we just created on above.
	return 1;
}

LOCENV_MODULE_LOADER(loader)

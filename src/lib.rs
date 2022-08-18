use locenv::api::LuaState;
use locenv::{upvalue_index, Context, FunctionEntry};
use locenv_macros::loader;
use std::os::raw::c_int;

const MODULE_FUNCTIONS: [FunctionEntry; 1] = [FunctionEntry {
    name: "myfunction",
    function: Some(myfunction),
}];

extern "C" fn myfunction(lua: *mut LuaState) -> c_int {
    // We can access the context here because we make it as the upvalue for this function in the loader.
    let context = Context::from_lua(lua, upvalue_index(1));

    0
}

#[loader]
extern "C" fn loader(lua: *mut LuaState) -> c_int {
    // More information about 'loader': https://www.lua.org/manual/5.4/manual.html#6.3
    // The loader data is locenv::Context.
    locenv::create_table(lua, 0, MODULE_FUNCTIONS.len() as _);
    locenv::push_value(lua, 2); // Push a loader data as upvalue for all functions in MODULE_FUNCTIONS.
    locenv::set_functions(lua, &MODULE_FUNCTIONS, 1);

    // Return a function table that we just created on above.
    1
}

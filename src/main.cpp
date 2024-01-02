#include <iostream>
#include <string>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include "lua_binding.h"




int main() {
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    initLuaBinding(L);
    

    if(luaL_dofile(L, "src/script.lua") != LUA_OK) {
        const char* error_msg = lua_tostring(L, -1);
        std::cerr << "Failed to run script: " << error_msg << std::endl;
    }

    lua_close(L);

    // wait for input
    // std::cin.get();

    return 0;
}
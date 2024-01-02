#pragma once

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

void initLuaBinding(lua_State* L);
int l_import(lua_State* L);
extern "C" int luaopen_MyClass(lua_State* L);
#pragma once

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#define ENABLE_LUA_DEBUG 1
#if ENABLE_LUA_DEBUG
// 辅助函数，打印lua栈
void PrintLuaStack(lua_State* L, const char* file, int line) {
    int top = lua_gettop(L);
    std::cout << file << ":" << line  << " Lua Stack: ";
    for(int i = 1; i <= top; i++) {
        int type = lua_type(L, i);
        switch(type) {
        case LUA_TSTRING: std::cout << lua_tostring(L, i); break;
        case LUA_TBOOLEAN: std::cout << (lua_toboolean(L, i) ? "true" : "false"); break;
        case LUA_TNUMBER: std::cout << lua_tonumber(L, i); break;
        case LUA_TTABLE: std::cout << "tb:" << lua_topointer(L, i); break;
        case LUA_TUSERDATA: std::cout << "ud:" /* << lua_topointer(L, i) */; break;
        default: std::cout << lua_typename(L, type); break;
        }
        std::cout << " ";
    }
    std::cout  << std::endl;
}
#define LS(L) PrintLuaStack(L, __FILE__, __LINE__);
#else
#define LS(L) ;
#endif
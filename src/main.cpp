#include <iostream>
#include <string>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#define ENABLE_LUA_DEBUG 0
#if ENABLE_LUA_DEBUG
// 辅助函数，打印lua栈
void PrintLuaStack(lua_State* L) {
    int top = lua_gettop(L);
    std::cout << "Lua Stack: ";
    for(int i = 1; i <= top; i++) {
        int type = lua_type(L, i);
        switch(type) {
        case LUA_TSTRING: std::cout << lua_tostring(L, i); break;
        case LUA_TBOOLEAN: std::cout << (lua_toboolean(L, i) ? "true" : "false"); break;
        case LUA_TNUMBER: std::cout << lua_tonumber(L, i); break;
        default: std::cout << lua_typename(L, type); break;
        }
        std::cout << " ";
    }
    std::cout << std::endl;
}
#define LS(L) PrintLuaStack(L);
#else
#define LS(L) ;
#endif

class MyClass {
public:
    MyClass(int value) : value_(value) {}
    int GetValue() const { return value_; }
    void SetValue(int value) { value_ = value; }

private:
    int value_;

public:
    std::string name;
};

/*
    1. 在lua侧，调用MyClass.new(value)时，会调用这个函数
    2. 创建的c++对象都是以userdata的形式存在的
    3. 为了让这个userdata可以调用c++的方法，需要给userdata设置元表
 */
static int l_MyClass_Ctor(lua_State* L) {
    // 1. 从栈上取出构造函数需要的参数
    int value = luaL_checkinteger(L, 1);
    // 2. 创建userdata，压入栈顶
    MyClass** udata = (MyClass**)lua_newuserdata(L, sizeof(MyClass*));
    // 3. 调用构造函数，创建对象，给userdata赋值
    *udata = new MyClass(value);
    // 4. 创建一个新的 Lua 表，并将这个表设置为 userdata 的用户值
    lua_newtable(L);
    lua_setuservalue(L, -2);
    // 5. 设置userdata的元表
    luaL_getmetatable(L, "MyClass");
    lua_setmetatable(L, -2);
    return 1;
}

static int l_MyClass_GetValue(lua_State* L) {
    MyClass* myClass = *(MyClass**)luaL_checkudata(L, 1, "MyClass");
    lua_pushinteger(L, myClass->GetValue());
    return 1;
}

static int l_MyClass_SetValue(lua_State* L) {
    MyClass* myClass = *(MyClass**)luaL_checkudata(L, 1, "MyClass");
    int value = luaL_checkinteger(L, 2);
    myClass->SetValue(value);
    return 0;
}

static int l_MyClass_index(lua_State* L) {
    MyClass* myClass = *(MyClass**)luaL_checkudata(L, 1, "MyClass");
    const char* key = luaL_checkstring(L, 2);

    //  1. 找c++对象的成员变量
    if(std::string(key) == "name") {
        lua_pushstring(L, myClass->name.c_str());
        return 1;
    }

    // 2. 在元表中查找 value = MyClass_mt[key]
    LS(L); // userdata name222
    lua_getmetatable(L, 1);
    LS(L); // userdata name222 MyClass_mt
    lua_getfield(L, -1, key);
    LS(L); // userdata name222 MyClass_mt value
    if(!lua_isnil(L, -1)) {
        return 1;
    }
    lua_settop(L, 2);

    // 3. 如果没有找到，返回userdata的用户值 value = getuservalue(udata)[key]
    lua_getuservalue(L, 1);
    lua_getfield(L, -1, key);
    // lua_pushvalue(L, 2);
    // lua_gettable(L, -2);
    if(!lua_isnil(L, -1)) {
        return 1;
    }

    return 0;
}

static int l_MyClass_newindex(lua_State* L) {
    MyClass* myClass = *(MyClass**)luaL_checkudata(L, 1, "MyClass");
    const char* key = luaL_checkstring(L, 2);

    if(std::string(key) == "name") {
        const char* value = luaL_checkstring(L, 3);
        myClass->name = value;
    } else {
        // lua_getmetatable(L, 1);
        lua_getuservalue(L, 1);
        lua_pushvalue(L, 2);
        lua_pushvalue(L, 3);
        lua_settable(L, -3);
    }

    return 0;
}

static const luaL_Reg MyClass_methods[] = {{"new", l_MyClass_Ctor},
                                           {"GetValue", l_MyClass_GetValue},
                                           {"SetValue", l_MyClass_SetValue},
                                           {NULL, NULL}};

// 这个调用完之后，栈顶是 MyClass_mt
extern "C" int luaopen_MyClass(lua_State* L) {
    // 新建一个 同名的 元表 MyClass_mt = {}
    luaL_newmetatable(L, "MyClass");

    // MyClass_mt.__index = l_MyClass_index
    lua_pushcfunction(L, l_MyClass_index);
    lua_setfield(L, -2, "__index");

    // MyClass_mt.__newindex = l_MyClass_newindex
    lua_pushcfunction(L, l_MyClass_newindex);
    lua_setfield(L, -2, "__newindex");

    // MyClass_mt 添加函数表 MyClass_methods
    luaL_setfuncs(L, MyClass_methods, 0);

    return 1;
}

// import的结果就是返回这个 类型的元表
static int l_import(lua_State* L) {
    const char* module_name = luaL_checkstring(L, 1);

    if(std::string(module_name) == "MyClass") {
        luaopen_MyClass(L);
    } else {
        luaL_error(L, "Unknown module: %s", module_name);
    }

    return 1;
}

int main() {
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    lua_pushcfunction(L, l_import);
    lua_setglobal(L, "import");

    if(luaL_dofile(L, "src/script.lua") != LUA_OK) {
        const char* error_msg = lua_tostring(L, -1);
        std::cerr << "Failed to run script: " << error_msg << std::endl;
    }

    lua_close(L);

    // wait for input
    // std::cin.get();

    return 0;
}
#include <iostream>
#include <string>

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
    std::cout << "Lua Stack: ";
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
    std::cout << "====>" << file << ":" << line << std::endl;
}
#define LS(L) PrintLuaStack(L, __FILE__, __LINE__);
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
    // 4. 设置 uservalue，这里可以延迟到newindex时再设置。因为不一定会用到
    // lua_newtable(L);
    // lua_setuservalue(L, -2);
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
    /*
        TODO:
        1. 这里可以用一个map来存储
        2. 应该用反射来获取成员变量
        3. 需要考虑父类的成员
     */
    if(strcmp(key, "name") == 0) {
        lua_pushstring(L, myClass->name.c_str());
        return 1;
    }

    // 2. 在元表中查找 value = MyClass_mt[key]
    LS(L); // userdata key
    lua_getmetatable(L, 1);
    LS(L); // userdata key mt
    if(lua_getfield(L, -1, key)) {
        LS(L); // userdata key mt value
        return 1;
    }
    lua_settop(L, 2);

    // 3. 在uservalue中查找 value = MyClass_user_value[key]
    if(lua_getuservalue(L, 1)) {
        LS(L); // userdata key obj_user_value
        lua_pushvalue(L, 2);
        LS(L); // userdata key obj_user_value key
        if(lua_rawget(L, -2)) {
            LS(L); // userdata key obj_user_value value
            return 1;
        }
    }

    return 0;
}

static int l_MyClass_newindex(lua_State* L) {
    MyClass* myClass = *(MyClass**)luaL_checkudata(L, 1, "MyClass");
    const char* key = luaL_checkstring(L, 2);

    // 1. 设置c++对象的成员变量
    if(strcmp(key, "name") == 0) {
        const char* value = luaL_checkstring(L, 3);
        myClass->name = value;
        return 0;
    }

    // 2. 设置uservalue的值
    // 在这里延迟创建uservalue。 
    // 和index不同的是，这里不用访问元表，因为元表在创建类型时已经确定了，后面不允许通过实例的行为来修改
    if(!lua_getuservalue(L, 1)) {
        LS(L); // userdata key value obj_user_value(nil)
        lua_newtable(L);
        LS(L); // userdata key value obj_user_value(nil) {}
        lua_setuservalue(L, 1);
        LS(L); // userdata key value obj_user_value(nil)
    }
    lua_pop(L, 1);
    LS(L); // userdata key value

    lua_getuservalue(L, 1);
    LS(L); // userdata key value obj_user_value
    lua_pushvalue(L, 2);
    LS(L); // userdata key value obj_user_value key
    lua_pushvalue(L, 3);
    LS(L); // userdata key value obj_user_value key value
    lua_rawset(L, -3);
    LS(L); // userdata key value obj_user_value

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

    if(strcmp(module_name, "MyClass") == 0) {
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
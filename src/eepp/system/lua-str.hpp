#ifndef EE_SYSTEM_LUA_STR_HPP
#define EE_SYSTEM_LUA_STR_HPP

#include <cstdlib>

typedef void ( *LuaFailFun )( const char* msg );

void lua_str_fail_func( LuaFailFun f );

struct LuaMatch {
	int start;
	int end;
};

int lua_str_match( const char* text, int offset, size_t len, const char* pattern, LuaMatch* mm,
				   int force_anchor = 0 );

#endif // EE_SYSTEM_LUA_STR_HPP

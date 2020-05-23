#ifndef _LUA_STR_H
#define _LUA_STR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*FailFun)(const char *msg);
void str_fail_func(FailFun f);

typedef struct LuaMatch {
  int start;
  int end;
} LuaMatch;

int str_match (const char *text, size_t len, const char *pattern,  LuaMatch *mm);

#ifdef __cplusplus
}
#endif

#endif

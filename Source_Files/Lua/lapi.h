/*
** $Id$
** Auxiliary functions from Lua API
** See Copyright Notice in lua.h
*/
#include "config.h"
#ifdef HAVE_LUA

#ifndef lapi_h
#define lapi_h


#include "lobject.h"


LUAI_FUNC void luaA_pushobject (lua_State *L, const TValue *o);

#endif
#endif

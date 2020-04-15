/*
 *  Copyright 2012 The Luvit Authors. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */

#include <string.h>
#include <stdlib.h>
#include "lhttp_parser.h"
#ifdef USE_LLHTTP
#include "llhttp.h"
typedef llhttp_t http_parser;
#else
#include "http_parser.h"
#endif
#if LUA_VERSION_NUM < 502
/* lua_rawlen: Not entirely correct, but should work anyway */
# ifndef lua_rawlen
#	define lua_rawlen lua_objlen
# endif
/* lua_...uservalue: Something very different, but it should get the job done */
# ifndef lua_getuservalue
#	define lua_getuservalue lua_getfenv
# endif
# ifndef lua_setuservalue
#	define lua_setuservalue lua_setfenv
# endif
# ifndef luaL_newlib
#	define luaL_newlib(L,l) (lua_newtable(L), luaL_register(L,NULL,l))
# endif
# ifndef luaL_setfuncs
#	define luaL_setfuncs(L,l,n) (assert(n==0), luaL_register(L,NULL,l))
# endif
#endif

static const char* method_to_str(unsigned short m) {
 #ifdef USE_LLHTTP
  return llhttp_method_name(m);
#else
  return http_method_str(m);
#endif
}



/*****************************************************************************/
#ifdef USE_LLHTTP
static llhttp_settings_t lhttp_parser_settings;
#else
static struct http_parser_settings lhttp_parser_settings;
#endif

static int lhttp_parser_on_message_begin(http_parser *p) {
  lua_State *L = p->data;

  /* Put the environment of the userdata on the top of the stack */
  lua_getuservalue(L, 1);
  /* Get the onMessageBegin callback and put it on the stack */
  lua_getfield(L, -1, "onMessageBegin");
  /* See if it's a function */
  if (lua_isfunction (L, -1) == 0) {
    /* no function defined */
    lua_pop(L, 2);
    return 0;
  };
  lua_call(L, 0, 1);

  lua_pop(L, 1); /* pop returned value */
  lua_pop(L, 1); /* pop the userdata env */
  return 0;
}

static int lhttp_parser_on_message_complete(http_parser *p) {
  lua_State *L = p->data;

  /* Put the environment of the userdata on the top of the stack */
  lua_getuservalue(L, 1);
  /* Get the onMessageComplete callback and put it on the stack */
  lua_getfield(L, -1, "onMessageComplete");
  /* See if it's a function */
  if (lua_isfunction (L, -1) == 0) {
    /* no function defined */
    lua_pop(L, 2);
    return 0;
  };
  lua_call(L, 0, 1);

  lua_pop(L, 2); /* pop returned value and the userdata env */
  return 0;
}


static int lhttp_parser_on_url(http_parser *p, const char *at, size_t length) {
  lua_State *L = p->data;

  /* Put the environment of the userdata on the top of the stack */
  lua_getuservalue(L, 1);
  /* Get the onUrl callback and put it on the stack */
  lua_getfield(L, -1, "onUrl");
  /* See if it's a function */
  if (lua_isfunction (L, -1) == 0) {
    /* no function defined */
    lua_pop(L, 2);
    return 0;
  };
  /* Push the string argument */
  lua_pushlstring(L, at, length);

  lua_call(L, 1, 1);

  lua_pop(L, 2); /* pop returned value and the userdata env */
  return 0;
}

static int lhttp_parser_on_status(http_parser *p, const char *at, size_t length) {
  lua_State *L = p->data;

  /* Put the environment of the userdata on the top of the stack */
  lua_getuservalue(L, 1);
  /* Get the onUrl callback and put it on the stack */
  lua_getfield(L, -1, "onStatus");
  /* See if it's a function */
  if (lua_isfunction (L, -1) == 0) {
    /* no function defined */
    lua_pop(L, 2);
    return 0;
  };
  /* Push the status code and string argument */
  lua_pushinteger(L, p->status_code);
  lua_pushlstring(L, at, length);

  lua_call(L, 2, 1);

  lua_pop(L, 2); /* pop returned value and the userdata env */
  return 0;
}

static int lhttp_parser_on_header_field(http_parser *p, const char *at, size_t length) {
  lua_State *L = p->data;

  /* Put the environment of the userdata on the top of the stack */
  lua_getuservalue(L, 1);
  /* Get the onHeaderField callback and put it on the stack */
  lua_getfield(L, -1, "onHeaderField");
  /* See if it's a function */
  if (lua_isfunction (L, -1) == 0) {
    /* no function defined */
    lua_pop(L, 2);
    return 0;
  };
  /* Push the string argument */
  lua_pushlstring(L, at, length);

  lua_call(L, 1, 1);

  lua_pop(L, 2); /* pop returned value and the userdata env */
  return 0;
}

static int lhttp_parser_on_header_value(http_parser *p, const char *at, size_t length) {
  lua_State *L = p->data;

  /* Put the environment of the userdata on the top of the stack */
  lua_getuservalue(L, 1);
  /* Get the onHeaderValue callback and put it on the stack */
  lua_getfield(L, -1, "onHeaderValue");
  /* See if it's a function */
  if (lua_isfunction (L, -1) == 0) {
    /* no function defined */
    lua_pop(L, 2);
    return 0;
  };
  /* Push the string argument */
  lua_pushlstring(L, at, length);

  lua_call(L, 1, 1);

  lua_pop(L, 2); /* pop returned value and the userdata env */
  return 0;
}

static int lhttp_parser_on_body(http_parser *p, const char *at, size_t length) {
  lua_State *L = p->data;

  /* Put the environment of the userdata on the top of the stack */
  lua_getuservalue(L, 1);
  /* Get the onBody callback and put it on the stack */
  lua_getfield(L, -1, "onBody");
  /* See if it's a function */
  if (lua_isfunction (L, -1) == 0) {
    /* no function defined */
    lua_pop(L, 2);
    return 0;
  };
  /* Push the string argument */
  lua_pushlstring(L, at, length);

  lua_call(L, 1, 1);

  lua_pop(L, 2); /* pop returned value and the userdata env */
  return 0;
}

static int lhttp_parser_on_headers_complete(http_parser *p) {
  lua_State *L = p->data;

  /* Put the environment of the userdata on the top of the stack */
  lua_getuservalue(L, 1);
  /* Get the onHeadersComplete callback and put it on the stack */
  lua_getfield(L, -1, "onHeadersComplete");
  /* See if it's a function */
  if (lua_isfunction (L, -1) == 0) {
    /* no function defined */
    lua_pop(L, 2);
    return 0;
  };

  /* Push a new table as the argument */
  lua_newtable (L);

  /* METHOD */
  if (p->type == HTTP_REQUEST) {
    lua_pushstring(L, method_to_str(p->method));
    lua_setfield(L, -2, "method");
  }

  /* STATUS */
  if (p->type == HTTP_RESPONSE) {
    lua_pushinteger(L, p->status_code);
    lua_setfield(L, -2, "status_code");
  }

  /* VERSION */
  lua_pushinteger(L, p->http_major);
  lua_setfield(L, -2, "version_major");
  lua_pushinteger(L, p->http_minor);
  lua_setfield(L, -2, "version_minor");

#ifdef USE_LLHTTP
  lua_pushboolean(L, llhttp_should_keep_alive(p));
#else
  lua_pushboolean(L, http_should_keep_alive(p));
#endif
  lua_setfield(L, -2, "should_keep_alive");

  lua_pushboolean(L, p->upgrade);
  lua_setfield(L, -2, "upgrade");


  lua_call(L, 1, 1);

  lua_pop(L, 2); /* pop returned value and the userdata env */
  return 0;
}

static int lhttp_parser_on_chunk_header(http_parser *p) {
  lua_State *L = p->data;

  /* Put the environment of the userdata on the top of the stack */
  lua_getuservalue(L, 1);
  /* Get the onMessageComplete callback and put it on the stack */
  lua_getfield(L, -1, "onChunkHeader");
  /* See if it's a function */
  if (lua_isfunction (L, -1) == 0) {
    /* no function defined */
    lua_pop(L, 2);
    return 0;
  };
  lua_pushinteger(L, p->content_length);
  lua_call(L, 1, 1);

  lua_pop(L, 2); /* pop returned value and the userdata env */
  return 0;
}

static int lhttp_parser_on_chunk_complete(http_parser *p) {
  lua_State *L = p->data;

  /* Put the environment of the userdata on the top of the stack */
  lua_getuservalue(L, 1);
  /* Get the onMessageComplete callback and put it on the stack */
  lua_getfield(L, -1, "onChunkComplete");
  /* See if it's a function */
  if (lua_isfunction (L, -1) == 0) {
    /* no function defined */
    lua_pop(L, 2);
    return 0;
  };
  lua_call(L, 0, 1);

  lua_pop(L, 2); /* pop returned value and the userdata env */
  return 0;
}

/******************************************************************************/

/* Takes as arguments a string for type and a table for event callbacks */
static int lhttp_parser_new (lua_State *L) {
  int itype;
  const char *type = luaL_checkstring(L, 1);
  http_parser* parser;
  luaL_checktype(L, 2, LUA_TTABLE);

  parser = (http_parser*)lua_newuserdata(L, sizeof(http_parser));

  if (0 == strcmp(type, "request")) {
    itype = HTTP_REQUEST;
  } else if (0 == strcmp(type, "response")) {
    itype = HTTP_RESPONSE;
  } else {
    return luaL_argerror(L, 1, "type must be 'request' or 'response'");
  }
#ifdef USE_LLHTTP
  llhttp_init(parser, itype, &lhttp_parser_settings);
#else
  http_parser_init(parser, itype);
#endif

  /* Store the current lua state in the parser's data */
  parser->data = L;

  /* Set the callback table as the userdata's environment */
  lua_pushvalue(L, 2);
  lua_setuservalue(L, -2);

  /* Set the type of the userdata as an lhttp_parser instance */
  luaL_getmetatable(L, "lhttp_parser");
  lua_setmetatable(L, -2);

  /* return the userdata */
  return 1;
}

/*
** translate a relative initial string position
** (negative means back from end): clip result to [1, inf).
** The length of any string in Lua must fit in a lua_Integer,
** so there are no overflows in the casts.
** The inverted comparison avoids a possible overflow
** computing '-pos'.
*/
static size_t posrelatI (lua_Integer pos, size_t len) {
  if (pos > 0)
    return (size_t)pos;
  else if (pos == 0)
    return 1;
  else if (pos < -(lua_Integer)len)  /* inverted comparison */
    return 1;  /* clip to 1 */
  else return len + (size_t)pos + 1;
}


/*
** Gets an optional ending string position from argument 'arg',
** with default value 'def'.
** Negative means back from end: clip result to [0, len]
*/
static size_t getendpos (lua_State *L, int arg, lua_Integer def,
                         size_t len) {
  lua_Integer pos = luaL_optinteger(L, arg, def);
  if (pos > (lua_Integer)len)
    return len;
  else if (pos >= 0)
    return (size_t)pos;
  else if (pos < -(lua_Integer)len)
    return 0;
  else return len + (size_t)pos + 1;
}

/* execute(parser, buffer, offset, length) */
static int lhttp_parser_execute (lua_State *L) {
  http_parser* parser = (http_parser *)luaL_checkudata(L, 1, "lhttp_parser");
  size_t chunk_len;
  const char *chunk;
  size_t offset;
  size_t length;
  size_t nparsed;
#ifdef USE_LLHTTP
  llhttp_errno_t err;
  const char* pos;
#endif

  luaL_checktype(L, 2, LUA_TSTRING);
  chunk = lua_tolstring(L, 2, &chunk_len);

  offset = posrelatI(luaL_optinteger(L, 3, 1), chunk_len) - 1;
  length = getendpos(L, 4, -1, chunk_len) - offset;

  luaL_argcheck(L, offset <= chunk_len, 3, "Offset is out of bounds");
  luaL_argcheck(L, offset + length <= chunk_len, 4,  "Length extends beyond end of chunk");

#ifdef USE_LLHTTP
  if (chunk_len)
  {
    err = llhttp_execute(parser, chunk + offset, length);
    if (err != HPE_OK && err != HPE_PAUSED)
    {
      lua_pushnil(L);
      lua_pushstring(L, llhttp_errno_name(err));
      return 2;
    }
    pos = llhttp_get_error_pos(parser);
    if (pos==NULL)
      nparsed = length;
    else
      nparsed = (pos - chunk - offset);
  }else
    nparsed = 0;
#else
  nparsed = http_parser_execute(parser, &lhttp_parser_settings, chunk + offset, length);
#endif

  lua_pushnumber(L, nparsed);
  return 1;
}

static int lhttp_parser_finish (lua_State *L) {
  http_parser* parser = (http_parser *)luaL_checkudata(L, 1, "lhttp_parser");
  size_t nparsed;
#ifdef USE_LLHTTP
  llhttp_errno_t err;
#endif

#ifdef USE_LLHTTP
  err = llhttp_finish(parser);
  if (err != HPE_OK && err != HPE_PAUSED)
  {
    lua_pushnil(L);
    lua_pushstring(L, llhttp_errno_name(err));
    return 2;
  }
  nparsed = 0;
#else
  nparsed = http_parser_execute(parser, &lhttp_parser_settings, NULL, 0);
#endif
  lua_pushnumber(L, nparsed);
  return 1;
}

static int lhttp_parser_reinitialize (lua_State *L) {
  http_parser* parser = (http_parser *)luaL_checkudata(L, 1, "lhttp_parser");
  int itype;
  const char *type = luaL_checkstring(L, 2);

  if (0 == strcmp(type, "request")) {
    itype = HTTP_REQUEST;
  } else if (0 == strcmp(type, "response")) {
    itype = HTTP_RESPONSE;
  } else {
    return luaL_argerror(L, 1, "type must be 'request' or 'response'");
  }
#ifdef USE_LLHTTP
  llhttp_init(parser, itype, &lhttp_parser_settings);
#else
  http_parser_init(parser, itype);
#endif
    /* Store the current lua state in the parser's data */
  parser->data = L;

  if(!lua_isnoneornil(L, 3))
  {
    luaL_checktype(L, 3, LUA_TTABLE);
    /* Set the callback table as the userdata's environment */
    lua_pushvalue(L, 3);
    lua_setuservalue(L, 1);
  }

  return 0;
}

/******************************************************************************/
static int lhttp_parser_http_version(lua_State* L) {
    http_parser* parser = (http_parser *)luaL_checkudata(L, 1, "lhttp_parser");
    lua_pushinteger(L, parser->http_major);
    lua_pushinteger(L, parser->http_minor);
    return 2;
}

static int lhttp_parser_status_code(lua_State* L) {
    http_parser* parser = (http_parser *)luaL_checkudata(L, 1, "lhttp_parser");
    lua_pushinteger(L, parser->status_code);
    return 1;
}

static int lhttp_parser_method(lua_State* L) {
    http_parser* parser = (http_parser *)luaL_checkudata(L, 1, "lhttp_parser");
    lua_pushstring(L, method_to_str(parser->method));
    return 1;
}

static int lhttp_parser_http_errno(lua_State* L) {
    http_parser* parser = (http_parser *)luaL_checkudata(L, 1, "lhttp_parser");
#ifdef USE_LLHTTP
    llhttp_errno_t http_errno = llhttp_get_errno(parser);
    lua_pushnumber(L, http_errno);
    lua_pushstring(L, llhttp_errno_name(http_errno));
    lua_pushstring(L, llhttp_get_error_reason(parser));
#else
    enum http_errno http_errno = parser->http_errno;
    lua_pushnumber(L, http_errno);
    lua_pushstring(L, http_errno_name(http_errno));
    lua_pushstring(L, http_errno_description(http_errno));
#endif
    return 3;
}

static int lhttp_parser_upgrade(lua_State* L) {
    http_parser* parser = (http_parser *)luaL_checkudata(L, 1, "lhttp_parser");
    lua_pushboolean(L, parser->upgrade);
    return 1;
}

static int lhttp_parser_should_keep_alive(lua_State* L) {
    http_parser* parser = (http_parser *)luaL_checkudata(L, 1, "lhttp_parser");
#ifdef USE_LLHTTP
    lua_pushboolean(L, llhttp_should_keep_alive(parser));
#else
    lua_pushboolean(L, http_should_keep_alive(parser));
#endif
    return 1;
}

static int lhttp_parser_tostring(lua_State* L) {
    http_parser* parser = (http_parser *)luaL_checkudata(L, 1, "lhttp_parser");
    lua_pushfstring(L, "lhttp_parser %p", parser);
    return 1;
}

/******************************************************************************/
static const luaL_Reg lhttp_parser_m[] = {
  {"http_version", lhttp_parser_http_version},
  {"status_code", lhttp_parser_status_code},
  {"method", lhttp_parser_method},
  {"http_errno", lhttp_parser_http_errno},
  {"upgrade", lhttp_parser_upgrade},
  {"should_keep_alive", lhttp_parser_should_keep_alive},
  {"execute", lhttp_parser_execute},
  {"finish", lhttp_parser_finish},
  {"reinitialize", lhttp_parser_reinitialize},
  {"__tostring", lhttp_parser_tostring},
  {NULL, NULL}
};

static const luaL_Reg lhttp_parser_f[] = {
  {"new", lhttp_parser_new},
  {"parseUrl", lhttp_parser_parse_url},
  {NULL, NULL}
};

LUALIB_API int luaopen_lhttp_parser (lua_State *L) {

  /* This needs to be done sometime? */
#ifdef USE_LLHTTP
  llhttp_settings_init(&lhttp_parser_settings);
#endif
  lhttp_parser_settings.on_message_begin    = lhttp_parser_on_message_begin;
  lhttp_parser_settings.on_url              = lhttp_parser_on_url;
  lhttp_parser_settings.on_status           = lhttp_parser_on_status;
  lhttp_parser_settings.on_header_field     = lhttp_parser_on_header_field;
  lhttp_parser_settings.on_header_value     = lhttp_parser_on_header_value;
  lhttp_parser_settings.on_headers_complete = lhttp_parser_on_headers_complete;
  lhttp_parser_settings.on_body             = lhttp_parser_on_body;
  lhttp_parser_settings.on_message_complete = lhttp_parser_on_message_complete;
  lhttp_parser_settings.on_chunk_header     = lhttp_parser_on_chunk_header;
  lhttp_parser_settings.on_chunk_complete   = lhttp_parser_on_chunk_complete;

  /* Create a metatable for the lhttp_parser userdata type */
  luaL_newmetatable(L, "lhttp_parser");
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  luaL_setfuncs(L, lhttp_parser_m, 0);

  /* Put our one function on it */
  luaL_newlib(L, lhttp_parser_f);

  /* Stick version info on the http_parser table */
#ifdef USE_LLHTTP
  lua_pushnumber(L, LLHTTP_VERSION_MAJOR);
  lua_setfield(L, -2, "VERSION_MAJOR");
  lua_pushnumber(L, LLHTTP_VERSION_MINOR);
  lua_setfield(L, -2, "VERSION_MINOR");
  lua_pushnumber(L, LLHTTP_VERSION_PATCH);
  lua_setfield(L, -2, "VERSION_PATCH");
  lua_pushboolean(L, 1);
  lua_setfield(L, -2, "llhttp");
#else
  lua_pushnumber(L, HTTP_PARSER_VERSION_MAJOR);
  lua_setfield(L, -2, "VERSION_MAJOR");
  lua_pushnumber(L, HTTP_PARSER_VERSION_MINOR);
  lua_setfield(L, -2, "VERSION_MINOR");
  lua_pushnumber(L, HTTP_PARSER_VERSION_PATCH);
  lua_setfield(L, -2, "VERSION_PATCH");
#endif

  /* Return the new module */
  return 1;
}


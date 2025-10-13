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

/***
 * HTTP Parser for Lua
 *
 * This module provides Lua bindings for llhttp, a fast HTTP parser library.
 * It supports parsing both HTTP requests and responses with callbacks for
 * different parsing events.
 *
 * @module lhttp_parser
 * @license Apache License 2.0
 * @copyright 2012 The Luvit Authors
 */

#include "lhttp_parser.h"
#include "llhttp.h"
#include <stdlib.h>
#include <string.h>
typedef llhttp_t http_parser;
#if LUA_VERSION_NUM < 502
/* lua_rawlen: Not entirely correct, but should work anyway */
#ifndef lua_rawlen
#define lua_rawlen lua_objlen
#endif

#ifndef luaL_newlib
#define luaL_newlib(L, l) (lua_newtable(L), luaL_register(L, NULL, l))
#endif
#ifndef luaL_setfuncs
#define luaL_setfuncs(L, l, n) (assert(n == 0), luaL_register(L, NULL, l))
#endif
#endif

typedef struct {
  void *L;
  int ref;
} parser_ctx;

/*****************************************************************************/
static llhttp_settings_t lhttp_parser_settings;

static int lhttp_parser_pcall_callback(http_parser *p,
                                       const char *func,
                                       int nargs, int nresult) {
  parser_ctx *ctx = p->data;
  lua_State *L = ctx->L;
  int top;

  /* Safety check: ensure L is valid */
  if (L == NULL) {
    return HPE_INTERNAL;
  }

  /* Put the environment of the userdata on the top of the stack */
  lua_rawgeti(L, LUA_REGISTRYINDEX, ctx->ref);
  top = lua_gettop(L);

  /* Get the callback and put it on the stack */
  lua_pushstring(L, func);
  lua_rawget(L, top);

  /* remove environment of the userdata */
  lua_remove(L, top);

  /* See if it's a function */
  if (lua_isnil(L, -1)) {
    /* no function defined */
    lua_pop(L, 1 + nargs);
    return 0;
  };

  if (nargs > 0) lua_insert(L, lua_gettop(L) - nargs);

  if (lua_pcall(L, nargs, nresult, 0) != 0) {
    fprintf(stderr, "Error while calling %s: %s\n", func, lua_tostring(L, -1));

    lua_pop(L, 1);
    return HPE_USER;
  }

  return nresult;
}

static int lhttp_parser_on_message_begin(http_parser *p) {
  return lhttp_parser_pcall_callback(p, "onMessageBegin", 0, 0);
}

static int lhttp_parser_on_message_complete(http_parser *p) {
  return lhttp_parser_pcall_callback(p, "onMessageComplete", 0, 0);
}

static int lhttp_parser_on_url(http_parser *p, const char *at, size_t length) {
  parser_ctx *ctx = p->data;
  lua_State *L = ctx->L;

  /* Push the string argument */
  lua_pushlstring(L, at, length);
  return lhttp_parser_pcall_callback(p, "onUrl", 1, 0);
}

static int lhttp_parser_on_status(http_parser *p, const char *at,
                                  size_t length) {
  parser_ctx *ctx = p->data;
  lua_State *L = ctx->L;

  /* Push the status code and string argument */
  lua_pushinteger(L, p->status_code);
  lua_pushlstring(L, at, length);

  return lhttp_parser_pcall_callback(p, "onStatus", 2, 0);
}

static int lhttp_parser_on_header_field(http_parser *p, const char *at,
                                        size_t length) {
  parser_ctx *ctx = p->data;
  lua_State *L = ctx->L;

  /* Push the string argument */
  lua_pushlstring(L, at, length);

  return lhttp_parser_pcall_callback(p, "onHeaderField", 1, 0);
}

static int lhttp_parser_on_header_value(http_parser *p, const char *at,
                                        size_t length) {
  parser_ctx *ctx = p->data;
  lua_State *L = ctx->L;

  /* Push the string argument */
  lua_pushlstring(L, at, length);

  return lhttp_parser_pcall_callback(p, "onHeaderValue", 1, 0);
}

static int lhttp_parser_on_body(http_parser *p, const char *at, size_t length) {
  parser_ctx *ctx = p->data;
  lua_State *L = ctx->L;

  /* Push the string argument */
  lua_pushlstring(L, at, length);

  return lhttp_parser_pcall_callback(p, "onBody", 1, 0);
}

static int lhttp_parser_on_headers_complete(http_parser *p) {
  parser_ctx *ctx = p->data;
  lua_State *L = ctx->L;
  int ret;

  /* Push a new table as the argument */
  lua_newtable(L);

  /* METHOD */
  if (p->type == HTTP_REQUEST) {
    lua_pushstring(L, llhttp_method_name(p->method));
    lua_setfield(L, -2, "method");
  }

  /* STATUS */
  if (p->type == HTTP_RESPONSE) {
    lua_pushinteger(L, p->status_code);
    lua_setfield(L, -2, "status_code");
  }

  /* VERSION */
  lua_pushinteger(L, p->http_major);
  lua_setfield(L, -2, "http_major");
  lua_pushinteger(L, p->http_minor);
  lua_setfield(L, -2, "http_minor");

  lua_pushboolean(L, llhttp_should_keep_alive(p));
  lua_setfield(L, -2, "should_keep_alive");

  lua_pushboolean(L, llhttp_message_needs_eof(p));
  lua_setfield(L, -2, "message_needs_eof");

  lua_pushboolean(L, p->upgrade);
  lua_setfield(L, -2, "upgrade");

  lua_pushboolean(L, p->flags & F_CONNECTION_KEEP_ALIVE);
  lua_setfield(L, -2, "CONNECTION_KEEP_ALIVE");

  lua_pushboolean(L, p->flags & F_CONNECTION_CLOSE);
  lua_setfield(L, -2, "CONNECTION_CLOSE");

  lua_pushboolean(L, p->flags & F_CONNECTION_UPGRADE);
  lua_setfield(L, -2, "CONNECTION_UPGRADE");

  lua_pushboolean(L, p->flags & F_CHUNKED);
  lua_setfield(L, -2, "CHUNKED");

  lua_pushboolean(L, p->flags & F_UPGRADE);
  lua_setfield(L, -2, "UPGRADE");

  lua_pushboolean(L, p->flags & F_SKIPBODY);
  lua_setfield(L, -2, "SKIPBODY");

  lua_pushboolean(L, p->flags & F_TRAILING);
  lua_setfield(L, -2, "TRAILING");

  lua_pushboolean(L, p->flags & F_CONTENT_LENGTH);
  lua_setfield(L, -2, "CONTENT_LENGTH");

  lua_pushboolean(L, p->flags & F_TRANSFER_ENCODING);
  lua_setfield(L, -2, "TRANSFER_ENCODING");

  ret = lhttp_parser_pcall_callback(p, "onHeadersComplete", 1, 1);
  if(ret==1) {
    ret = luaL_optinteger(L, -1, 0);
    lua_pop(L, 1);
  } else
    ret = HPE_USER;
  return ret;
}

static int lhttp_parser_on_chunk_header(http_parser *p) {
  parser_ctx *ctx = p->data;
  lua_State *L = ctx->L;

  lua_pushinteger(L, p->content_length);
  return lhttp_parser_pcall_callback(p, "onChunkHeader", 1, 0);
}

static int lhttp_parser_on_chunk_complete(http_parser *p) {
  return lhttp_parser_pcall_callback(p, "onChunkComplete", 0, 0);
}

static int lhttp_parser_on_reset(http_parser *p) {
  return lhttp_parser_pcall_callback(p, "onReset", 0, 0);
}

/******************************************************************************/

/***
 * Create a new HTTP parser
 *
 * Creates a new parser instance for parsing HTTP requests or responses.
 * The parser uses callbacks to notify about parsing events.
 *
 * @function new
 * @tparam string parser_type Type of parser: 'request', 'response', or 'both'
 * @tparam table callbacks Table containing callback functions
 * @treturn userdata New parser object
 * @usage
 * local lhp = require('lhttp_parser')
 * local parser = lhp.new('request', {
 *   onMessageBegin = function() end,
 *   onUrl = function(url) end,
 *   onHeaderField = function(field) end,
 *   onHeaderValue = function(value) end,
 *   onHeadersComplete = function(info) end,
 *   onBody = function(chunk) end,
 *   onMessageComplete = function() end
 * })
 */
static int lhttp_parser_new(lua_State *L) {
  int itype;
  const char *type = luaL_optstring(L, 1, "both");
  http_parser *parser;
  parser_ctx *ctx;

  luaL_checktype(L, 2, LUA_TTABLE);

  parser = (http_parser *)lua_newuserdata(L, sizeof(http_parser) +
                                                 sizeof(parser_ctx));
  ctx = (parser_ctx *)&parser[1];

  if (0 == strcmp(type, "request")) {
    itype = HTTP_REQUEST;
  } else if (0 == strcmp(type, "response")) {
    itype = HTTP_RESPONSE;
  } else if (0 == strcmp(type, "both")) {
    itype = HTTP_BOTH;
  } else {
    return luaL_argerror(L, 1, "type must be 'request', 'response' or 'both'");
  }
  llhttp_init(parser, itype, &lhttp_parser_settings);

  /* Store the current lua state in the parser's data */
  parser->data = ctx;

  /* Set the callback table as the userdata's environment */
  lua_pushvalue(L, 2);
  ctx->ref = luaL_ref(L, LUA_REGISTRYINDEX);
  ctx->L = NULL;

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
static size_t posrelatI(lua_Integer pos, size_t len) {
  if (pos > 0)
    return (size_t)pos;
  else if (pos == 0)
    return 1;
  else if (pos < -(lua_Integer)len) /* inverted comparison */
    return 1;                       /* clip to 1 */
  else
    return len + (size_t)pos + 1;
}

/*
** Gets an optional ending string position from argument 'arg',
** with default value 'def'.
** Negative means back from end: clip result to [0, len]
*/
static size_t getendpos(lua_State *L, int arg, lua_Integer def, size_t len) {
  lua_Integer pos = luaL_optinteger(L, arg, def);
  if (pos > (lua_Integer)len)
    return len;
  else if (pos >= 0)
    return (size_t)pos;
  else if (pos < -(lua_Integer)len)
    return 0;
  else
    return len + (size_t)pos + 1;
}

/***
 * Execute the parser on input data
 *
 * Feeds data to the parser for parsing. The parser will invoke callbacks
 * as it encounters different parts of the HTTP message.
 *
 * @function parser:execute
 * @tparam string data Input data to parse
 * @tparam[opt=1] number i Starting position (1-based index)
 * @tparam[opt=-1] number j Ending position (negative values count from end)
 * @treturn number Number of bytes parsed
 * @treturn string Error code name (e.g., "HPE_OK")
 * @usage
 * local nparsed, err = parser:execute(data)
 * if err ~= "HPE_OK" then
 *   print("Parse error:", err)
 * end
 */
static int lhttp_parser_execute(lua_State *L) {
  http_parser *parser = (http_parser *)luaL_checkudata(L, 1, "lhttp_parser");
  parser_ctx *ctx = parser->data;
  size_t chunk_len;
  const char *chunk;
  size_t offset;
  size_t length;
  size_t nparsed;
  llhttp_errno_t err = HPE_OK;

  luaL_checktype(L, 2, LUA_TSTRING);
  chunk = lua_tolstring(L, 2, &chunk_len);

  offset = posrelatI(luaL_optinteger(L, 3, 1), chunk_len) - 1;
  length = getendpos(L, 4, -1, chunk_len) - offset;

  luaL_argcheck(L, offset <= chunk_len, 3, "Offset is out of bounds");
  luaL_argcheck(L, offset + length <= chunk_len, 4,
                "Length extends beyond end of chunk");

  ctx->L = L;

  lua_settop(L, 0);
  if (length) {
    chunk += offset;
    err = llhttp_execute(parser, chunk, length);
    if (err != HPE_OK && err != HPE_PAUSED && err != HPE_PAUSED_UPGRADE &&
        err != HPE_STRICT) {
      ctx->L = NULL;  /* Reset L after execution */
      lua_pushnil(L);
      lua_pushstring(L, llhttp_errno_name(err));
      return 2;
    }

    if (err != HPE_OK) {
      const char *pos = llhttp_get_error_pos(parser);
      nparsed = pos - chunk;
    } else {
      nparsed = length;
    }
  } else
    nparsed = 0;

  ctx->L = NULL;  /* Reset L after execution */
  lua_pushnumber(L, nparsed);
  lua_pushstring(L, llhttp_errno_name(err));
  return 2;
}

/***
 * Finish parsing
 *
 * Signals the end of input to the parser. This should be called after
 * all data has been fed to the parser.
 *
 * @function parser:finish
 * @treturn[1] number 0 on success
 * @treturn[2] nil On error
 * @treturn[2] string Error code name
 * @usage
 * local result, err = parser:finish()
 * if not result then
 *   print("Finish error:", err)
 * end
 */
static int lhttp_parser_finish(lua_State *L) {
  http_parser *parser = (http_parser *)luaL_checkudata(L, 1, "lhttp_parser");
  parser_ctx *ctx = parser->data;
  size_t nparsed;

  ctx->L = L;
  llhttp_errno_t err = llhttp_finish(parser);
  ctx->L = NULL;

  if (err != HPE_OK && err != HPE_PAUSED) {
    lua_pushnil(L);
    lua_pushstring(L, llhttp_errno_name(err));
    return 2;
  }
  nparsed = 0;
  lua_pushnumber(L, nparsed);
  return 1;
}

/***
 * Reset the parser
 *
 * Resets the parser state so it can be reused for parsing a new message.
 *
 * @function parser:reset
 * @usage parser:reset()
 */
static int lhttp_parser_reset(lua_State *L) {
  http_parser *parser = (http_parser *)luaL_checkudata(L, 1, "lhttp_parser");

  llhttp_reset(parser);
  return 1;
}

/******************************************************************************/
static int lhttp_parser_http_version(lua_State *L) {
  http_parser *parser = (http_parser *)luaL_checkudata(L, 1, "lhttp_parser");
  lua_pushinteger(L, parser->http_major);
  lua_pushinteger(L, parser->http_minor);
  return 2;
}

static int lhttp_parser_status_code(lua_State *L) {
  http_parser *parser = (http_parser *)luaL_checkudata(L, 1, "lhttp_parser");
  lua_pushinteger(L, parser->status_code);
  return 1;
}

static int lhttp_parser_method(lua_State *L) {
  http_parser *parser = (http_parser *)luaL_checkudata(L, 1, "lhttp_parser");
  lua_pushstring(L, llhttp_method_name(parser->method));
  return 1;
}

static int lhttp_parser_http_errno(lua_State *L) {
  http_parser *parser = (http_parser *)luaL_checkudata(L, 1, "lhttp_parser");
  llhttp_errno_t http_errno = llhttp_get_errno(parser);
  lua_pushnumber(L, http_errno);
  lua_pushstring(L, llhttp_errno_name(http_errno));
  lua_pushstring(L, llhttp_get_error_reason(parser));

  return 3;
}

static int lhttp_parser_upgrade(lua_State *L) {
  http_parser *parser = (http_parser *)luaL_checkudata(L, 1, "lhttp_parser");
  lua_pushboolean(L, parser->upgrade);
  return 1;
}

static int lhttp_parser_should_keep_alive(lua_State *L) {
  http_parser *parser = (http_parser *)luaL_checkudata(L, 1, "lhttp_parser");
  lua_pushboolean(L, llhttp_should_keep_alive(parser));
  return 1;
}

static int lhttp_parser_pause(lua_State *L) {
  http_parser *parser = (http_parser *)luaL_checkudata(L, 1, "lhttp_parser");
  llhttp_pause(parser);
  lua_pushvalue(L, 1);
  return 1;
}

static int lhttp_parser_resume(lua_State *L) {
  http_parser *parser = (http_parser *)luaL_checkudata(L, 1, "lhttp_parser");
  llhttp_resume(parser);
  llhttp_set_error_reason(parser, NULL);
  lua_pushvalue(L, 1);
  return 1;
}

static int lhttp_resume_after_upgrade(lua_State *L) {
  http_parser *parser = (http_parser *)luaL_checkudata(L, 1, "lhttp_parser");
  llhttp_resume_after_upgrade(parser);
  llhttp_set_error_reason(parser, NULL);
  lua_pushvalue(L, 1);
  return 1;
}

static int lhttp_parser_tostring(lua_State *L) {
  http_parser *parser = (http_parser *)luaL_checkudata(L, 1, "lhttp_parser");
  lua_pushfstring(L, "lhttp_parser %p", parser);
  return 1;
}

static int lhttp_parser_gc(lua_State *L) {
  http_parser *parser = (http_parser *)luaL_checkudata(L, 1, "lhttp_parser");
  parser_ctx *ctx = parser->data;

  /* Only unref if ref is valid */
  if (ctx && ctx->ref != LUA_NOREF && ctx->ref != LUA_REFNIL) {
    luaL_unref(L, LUA_REGISTRYINDEX, ctx->ref);
    ctx->ref = LUA_NOREF;
  }

  return 0;
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
    {"pause", lhttp_parser_pause},
    {"resume", lhttp_parser_resume},
    {"resume_after_upgrade", lhttp_resume_after_upgrade},
    {"reset", lhttp_parser_reset},

    {NULL, NULL}};

static const luaL_Reg lhttp_parser_f[] = {{"new", lhttp_parser_new},
                                          {NULL, NULL}};

LUALIB_API int luaopen_lhttp_parser(lua_State *L) {

  /* This needs to be done sometime? */
  llhttp_settings_init(&lhttp_parser_settings);
  lhttp_parser_settings.on_message_begin = lhttp_parser_on_message_begin;
  lhttp_parser_settings.on_message_complete = lhttp_parser_on_message_complete;
  lhttp_parser_settings.on_url = lhttp_parser_on_url;
  lhttp_parser_settings.on_status = lhttp_parser_on_status;
  lhttp_parser_settings.on_header_field = lhttp_parser_on_header_field;
  lhttp_parser_settings.on_header_value = lhttp_parser_on_header_value;
  lhttp_parser_settings.on_headers_complete = lhttp_parser_on_headers_complete;
  lhttp_parser_settings.on_body = lhttp_parser_on_body;
  lhttp_parser_settings.on_chunk_header = lhttp_parser_on_chunk_header;
  lhttp_parser_settings.on_chunk_complete = lhttp_parser_on_chunk_complete;

  lhttp_parser_settings.on_reset = lhttp_parser_on_reset;

  /*
  lhttp_parser_settings.on_url_complete = lhttp_parser_on_url_complete;
  lhttp_parser_settings.on_method_complete = lhttp_parser_on_method_complete;
  lhttp_parser_settings.on_status_complete = lhttp_parser_on_status_complete;
  lhttp_parser_settings.on_version_complete = lhttp_parser_on_version_complete;

  lhttp_parser_settings.on_header_field_complete =
  lhttp_parser_on_header_field_complete;
  lhttp_parser_settings.on_header_value_complete =
  lhttp_parser_on_header_value_complete;

  lhttp_parser_settings.on_chunk_header = lhttp_parser_on_chunk_header;
  lhttp_parser_settings.on_chunk_extension_name_complete =
  lhttp_parser_on_chunk_extension_name_complete;
  lhttp_parser_settings.on_chunk_extension_value_complete =
  lhttp_parser_on_chunk_extension_value_complete;

  lhttp_parser_settings.on_chunk_extension_name =
  lhttp_parser_on_chunk_extension_name;
  lhttp_parser_settings.on_chunk_extension_value =
  lhttp_parser_on_chunk_extension_value;

  lhttp_parser_settings.on_method = lhttp_parser_on_method;
  lhttp_parser_settings.on_version = lhttp_parser_on_version;
  */

  /* Create a metatable for the lhttp_parser userdata type */
  luaL_newmetatable(L, "lhttp_parser");
  lua_pushcfunction(L, lhttp_parser_tostring);
  lua_setfield(L, -2, "__tostring");
  lua_pushcfunction(L, lhttp_parser_gc);
  lua_setfield(L, -2, "__gc");
  lua_newtable(L);
  luaL_setfuncs(L, lhttp_parser_m, 0);
  lua_setfield(L, -2, "__index");
  lua_pop(L, 1);

  /* Put our one function on it */
  luaL_newlib(L, lhttp_parser_f);

  #define XX(x) lua_pushliteral(L, #x); lua_pushinteger(L, HPE_##x); lua_rawset(L, -3);
  XX(OK);
  XX(INTERNAL);
  XX(STRICT);
  XX(CR_EXPECTED);
  XX(LF_EXPECTED);
  XX(UNEXPECTED_CONTENT_LENGTH);
  XX(UNEXPECTED_SPACE);
  XX(CLOSED_CONNECTION);
  XX(INVALID_METHOD);
  XX(INVALID_URL);
  XX(INVALID_CONSTANT);
  XX(INVALID_VERSION);
  XX(INVALID_HEADER_TOKEN);
  XX(INVALID_CONTENT_LENGTH);
  XX(INVALID_CHUNK_SIZE);
  XX(INVALID_STATUS);
  XX(INVALID_EOF_STATE);
  XX(INVALID_TRANSFER_ENCODING);
  XX(CB_MESSAGE_BEGIN);
  XX(CB_HEADERS_COMPLETE);
  XX(CB_MESSAGE_COMPLETE);
  XX(CB_CHUNK_HEADER);
  XX(CB_CHUNK_COMPLETE);
  XX(PAUSED);
  XX(PAUSED_UPGRADE);
  XX(PAUSED_H2_UPGRADE);
  XX(USER);
  XX(CB_URL_COMPLETE);
  XX(CB_STATUS_COMPLETE);
  XX(CB_METHOD_COMPLETE);
  XX(CB_VERSION_COMPLETE);
  XX(CB_HEADER_FIELD_COMPLETE);
  XX(CB_HEADER_VALUE_COMPLETE);
  XX(CB_CHUNK_EXTENSION_NAME_COMPLETE);
  XX(CB_CHUNK_EXTENSION_VALUE_COMPLETE);
  XX(CB_RESET);
  #undef XX

  /* Stick version info on the http_parser table */
  lua_pushnumber(L, LLHTTP_VERSION_MAJOR);
  lua_setfield(L, -2, "VERSION_MAJOR");
  lua_pushnumber(L, LLHTTP_VERSION_MINOR);
  lua_setfield(L, -2, "VERSION_MINOR");
  lua_pushnumber(L, LLHTTP_VERSION_PATCH);
  lua_setfield(L, -2, "VERSION_PATCH");
  lua_pushboolean(L, 1);
  lua_setfield(L, -2, "llhttp");

  /* Return the new module */
  return 1;
}

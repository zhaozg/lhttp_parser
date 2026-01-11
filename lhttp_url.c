#ifdef __cplusplus
extern "C" {
#endif

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#ifdef __cplusplus
}
#endif

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "llurl.h"
#include "llquery.h"

#if LUA_VERSION_NUM < 502
/* lua_rawlen: Not entirely correct, but should work anyway */
#ifndef lua_rawlen
#define lua_rawlen lua_objlen
#endif
/* lua_...uservalue: Something very different, but it should get the job done */
#ifndef lua_getuservalue
#define lua_getuservalue lua_getfenv
#endif
#ifndef lua_setuservalue
#define lua_setuservalue lua_setfenv
#endif
#ifndef luaL_newlib
#define luaL_newlib(L, l) (lua_newtable(L), luaL_register(L, NULL, l))
#endif
#ifndef luaL_setfuncs
#define luaL_setfuncs(L, l, n) (assert(n == 0), luaL_register(L, NULL, l))
#endif
#endif

/***
 * Encode a string for use in URLs
 *
 * Encodes special characters in a string to make it safe for use in URLs.
 * Spaces are encoded as '+' and other special characters are percent-encoded.
 *
 * @function encode
 * @tparam string str String to encode
 * @treturn string URL-encoded string
 * @usage
 * local lurl = require('lhttp_url')
 * local encoded = lurl.encode("hello world!")
 * -- Returns: "hello+world%21"
 */
static int encode_url(lua_State* L) {
  size_t l;
  const char* input = luaL_checklstring(L, 1, &l);
  char buffer[2048];

  if (!input) {
    lua_pushstring(L, "");
    return 1;
  }

  // 计算编码后的大小
  size_t needed = llquery_url_encode(input, l, buffer, sizeof(buffer));
  if (needed < sizeof(buffer) - 1) {
    // 直接使用栈上的缓冲区
    lua_pushlstring(L, buffer, needed);
    return 1;
  }

  // 分配缓冲区
  char* buff = (char*)malloc(needed + 1);
  if (!buff) {
    lua_pushstring(L, "");
    return 1;
  }

  // 执行编码
  llquery_url_encode(input, l, buff, needed + 1);

  // 将结果压入栈并返回
  lua_pushlstring(L, buff, needed);
  free(buff);

  return 1;
}


static void query_to_lua_table(lua_State* L, struct llquery* query) {
  uint16_t count = llquery_count(query);
  lua_createtable(L, 0, count); // create result table
  for (uint16_t i = 0; i < count; i++) {
    const struct llquery_kv* kv = llquery_get_kv(query, i);
    if (!kv || kv->key_len == 0) continue;
    // push key
    lua_pushlstring(L, kv->key, kv->key_len);
    lua_rawget(L, -2); // get current value
    if (lua_isnil(L, -1)) {
      // first occurrence, set as string
      lua_pop(L, 1);
      lua_pushlstring(L, kv->key, kv->key_len);
      lua_pushlstring(L, kv->value, kv->value_len);
      lua_rawset(L, -3);
    } else if (lua_istable(L, -1)) {
      // already an array, append
      lua_Integer n = lua_rawlen(L, -1);
      lua_pushlstring(L, kv->value, kv->value_len);
      lua_rawseti(L, -2, n + 1);
      lua_pop(L, 1);
    } else {
      // exists as string, convert to array
      size_t oldlen = 0;
      const char* oldval = lua_tolstring(L, -1, &oldlen);
      lua_pop(L, 1);
      lua_pushlstring(L, kv->key, kv->key_len);
      lua_createtable(L, 3, 0);
      lua_pushlstring(L, oldval, oldlen);
      lua_rawseti(L, -2, 1);
      lua_pushlstring(L, kv->value, kv->value_len);
      lua_rawseti(L, -2, 2);
      lua_rawset(L, -3);
    }
  }
}

/***
 * Decode a URL-encoded string
 *
 * Decodes a URL-encoded string back to its original form.
 *
 * @function decode
 * @tparam string str URL-encoded string to decode
 * @treturn string Decoded string
 * @usage
 * local lurl = require('lhttp_url')
 *
 * -- 返回字符串
 * local str = lurl.decode("hello+world%21")
 * -- Returns: "hello world!"
 */
static int decode_url(lua_State* L) {
  size_t l;
  const char* input = luaL_checklstring(L, 1, &l);
  char buffer[2048];

  if (!input) {
    lua_pushstring(L, "");
    return 1;
  }

  size_t needed = llquery_url_decode(input, l, buffer, sizeof(buffer));
  if (needed < sizeof(buffer)) {
    lua_pushlstring(L, buffer, needed);
    return 1;
  }

  char* decoded_str = (char*)malloc(needed + 1);
  if (!decoded_str) {
    lua_pushstring(L, "");
    return 1;
  }
  needed = llquery_url_decode(input, l, decoded_str, needed + 1);
  lua_pushlstring(L, decoded_str, needed);
  free(decoded_str);
  return 1;
}

/***
 * Parse a query string into a table
 *
 * Always parses a query string into a Lua table, regardless of content.
 * Supports URL decoding and merging duplicate keys into arrays.
 *
 * @function parse_query
 * @tparam string query Query string to parse
 * @tparam[opt] boolean merge_duplicates Whether to merge duplicate keys into arrays
 * @treturn table Table of key-value pairs
 * @usage
 * local lurl = require('lhttp_url')
 * local params = lurl.parse_query("name=John&age=30&hobby=sports&hobby=music")
 * -- Returns: {name="John", age="30", hobby={"sports", "music"}}
 */
static int parse_query(lua_State* L) {
  size_t l;
  const char* input = luaL_checklstring(L, 1, &l);
  char buffer[8192];
  char *buf;

  if (!input || !*input) {
    return 0;
  }

  // 初始化 llquery 解析器
  struct llquery query;
  enum llquery_error err = llquery_init(&query, 0, LQF_AUTO_DECODE | LQF_KEEP_EMPTY | LQF_TRIM_VALUES | LQF_LOWERCASE_KEYS);

  if (err != LQE_OK) {
    return 0;
  }

  // 分配可写缓冲区
  if (l >= sizeof(buffer) + 1)
    buf = (char*)malloc(l + 1);
  else
    buf = buffer;

  if (!buf) {
    llquery_free(&query);
    return 0;
  }
  memcpy(buf, input, l);
  buf[l] = '\0';

  // 解析查询字符串
  err = llquery_parse_ex(buf, l, &query, buf, l + 1);

  if (err == LQE_OK) {
    // 直接返回 table
    query_to_lua_table(L, &query);
  } else {
    lua_pushnil(L);
  }

  if(buf != buffer) free(buf);
  llquery_free(&query);

  return 1;
}


/***
 * Parse a URL into components.
 * URL layout as below chart.
 * <pre>
 * ┌────────────────────────────────────────────────────────────────────────────────────────────────┐
 * │                                              href                                              │
 * ├──────────┬──┬─────────────────────┬────────────────────────┬───────────────────────────┬───────┤
 * │ protocol │  │        auth         │          host          │           path            │ hash  │
 * │          │  │                     ├─────────────────┬──────┼──────────┬────────────────┤       │
 * │          │  │                     │    hostname     │ port │ pathname │     search     │       │
 * │          │  │                     │                 │      │          ├─┬──────────────┤       │
 * │          │  │                     │                 │      │          │ │    query     │       │
 * "  https:   //    user   :   pass   @ sub.example.com : 8080   /p/a/t/h  ?  query=string   #hash "
 * │          │  │          │          │    hostname     │ port │          │                │       │
 * │          │  │          │          ├─────────────────┴──────┤          │                │       │
 * │ protocol │  │ username │ password │          host          │          │                │       │
 * ├──────────┴──┼──────────┴──────────┼────────────────────────┤          │                │       │
 * │   origin    │                     │         origin         │ pathname │     search     │ hash  │
 * ├─────────────┴─────────────────────┴────────────────────────┴──────────┴────────────────┴───────┤
 * │                                              href                                              │
 * └────────────────────────────────────────────────────────────────────────────────────────────────┘
 *  (All spaces in the "" line should be ignored. They are purely for formatting.)
 *  </pre>
 *  We not store all fields in parsed Lua table, just below files if exists.
 *
 *   - protocol
 *   - auth
 *   - hostname
 *   - port
 *   - pathname
 *   - query
 *   - hash
 *
 * @function parse
 * @tparam string url URL to parse
 * @tparam[opt=false] boolean is_connect Whether this is a CONNECT request URL
 * @treturn[1] table URL components table with fields: protocol, auth, host,
 * hostname, port, pathname, query, hash
 * @treturn[2] nil If parsing failed
 * @usage
 * local lurl = require('lhttp_url')
 * local parsed = lurl.parse("******example.com:8080/path?query=value#hash")
 * -- Returns a table with all parsed URL components
 */
static int lhttp_parser_parse_url(lua_State* L) {
  size_t len;
  char const* url = luaL_checklstring(L, 1, &len);
  int is_connect = lua_toboolean(L, 2);

  struct http_parser_url u = {0};
  if (http_parser_parse_url(url, len, is_connect, &u)) {
    return 0;
  }

  lua_createtable(L, 0, UF_MAX);  // preallocate space for fields
  if (u.field_set & (1 << UF_SCHEMA)) {
    lua_pushliteral(L, "protocol");
    lua_pushlstring(L, url + u.field_data[UF_SCHEMA].off,
                    u.field_data[UF_SCHEMA].len);
    lua_rawset(L, -3);
  }
  if (u.field_set & (1 << UF_USERINFO)) {
    lua_pushliteral(L, "auth");
    lua_pushlstring(L, url + u.field_data[UF_USERINFO].off,
                    u.field_data[UF_USERINFO].len);
    lua_rawset(L, -3);
  }
  if (u.field_set & (1 << UF_HOST)) {
    lua_pushliteral(L, "hostname");
    lua_pushlstring(L, url + u.field_data[UF_HOST].off,
                    u.field_data[UF_HOST].len);
    lua_rawset(L, -3);
  }
  if (u.field_set & (1 << UF_PORT)) {
    lua_pushliteral(L, "port");
    lua_pushlstring(L, url + u.field_data[UF_PORT].off,
                    u.field_data[UF_PORT].len);
    lua_rawset(L, -3);
  }
  if (u.field_set & (1 << UF_PATH)) {
    lua_pushliteral(L, "pathname");
    lua_pushlstring(L, url + u.field_data[UF_PATH].off,
                    u.field_data[UF_PATH].len);
    lua_rawset(L, -3);
  }
  if (u.field_set & (1 << UF_QUERY)) {
    lua_pushliteral(L, "query");
    lua_pushlstring(L, url + u.field_data[UF_QUERY].off,
                    u.field_data[UF_QUERY].len);
    lua_rawset(L, -3);
  }
  if (u.field_set & (1 << UF_FRAGMENT)) {
    lua_pushliteral(L, "hash");
    lua_pushlstring(L, url + u.field_data[UF_FRAGMENT].off,
                    u.field_data[UF_FRAGMENT].len);
    lua_rawset(L, -3);
  }
  return 1;
}

LUALIB_API int luaopen_lhttp_url(lua_State* L) {
  static const struct luaL_Reg R[] = {
      {"encode", encode_url},
      {"decode", decode_url},
      {"parse_query", parse_query},
      {"parse", lhttp_parser_parse_url},

      {NULL, NULL},
  };

  luaL_newlib(L, R);
  return 1;
};

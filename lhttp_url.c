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



/* 从 Lua 表中按 key 读取布尔字段，不存在时返回默认值 */
static int opt_bool_field(lua_State* L, int table_idx, const char* key, int default_val) {
  lua_getfield(L, table_idx, key);
  int result = default_val;
  if (!lua_isnil(L, -1)) {
    result = lua_toboolean(L, -1);
  }
  lua_pop(L, 1);
  return result;
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
 * Parses a URL query string into a Lua table. Supports URL decoding,
 * merging duplicate keys into arrays, key case control, and value trimming.
 *
 * @function parse_query
 * @tparam string query Query string to parse
 * @tparam[opt] table options Options table with optional boolean fields:
 *
 *   - **decode** (default `true`): enable URL decoding (`%XX` → char, `+` → space)
 *   - **merge_duplicates** (default `true`): merge duplicate keys into an array
 *     (e.g. `k=1&k=2` → `{k={"1","2"}}`)
 *   - **keep_empty** (default `true`): keep keys with empty values
 *     (e.g. `foo=&bar` → `{foo="", bar=""}`)
 *   - **lowercase_keys** (default `false`): convert all keys to lowercase
 *   - **trim_values** (default `false`): strip leading/trailing whitespace from values
 *   - **strict** (default `false`): return error on too many pairs instead of truncating
 *
 * @treturn table Table of key-value pairs
 * @usage
 * local lurl = require('lhttp_url')
 *
 * -- 基本用法，保留原始键名大小写
 * local p = lurl.parse_query("Name=Tom&Age=30")
 * -- Returns: {Name="Tom", Age="30"}
 *
 * -- 启用键名小写 + 去空白
 * local p = lurl.parse_query("A= 1 &B=2 ", {lowercase_keys=true, trim_values=true})
 * -- Returns: {a="1", b="2"}
 */
static int parse_query(lua_State* L) {
  size_t l;
  const char* input = luaL_checklstring(L, 1, &l);

  /* 合理默认：启用解码、合并重复键、保留空值；保留键名大小写、不去空白 */
  uint16_t flags = LQF_AUTO_DECODE | LQF_KEEP_EMPTY | LQF_MERGE_DUPLICATES;

  if (lua_istable(L, 2)) {
    /* options table — 按字段逐项读取 */
    if (!opt_bool_field(L, 2, "decode",           1)) flags &= (uint16_t)~LQF_AUTO_DECODE;
    if (!opt_bool_field(L, 2, "merge_duplicates", 1)) flags &= (uint16_t)~LQF_MERGE_DUPLICATES;
    if (!opt_bool_field(L, 2, "keep_empty",       1)) flags &= (uint16_t)~LQF_KEEP_EMPTY;
    if ( opt_bool_field(L, 2, "lowercase_keys",   0)) flags |= LQF_LOWERCASE_KEYS;
    if ( opt_bool_field(L, 2, "trim_values",      0)) flags |= LQF_TRIM_VALUES;
    if ( opt_bool_field(L, 2, "strict",           0)) flags |= LQF_STRICT;
  } else if (lua_isboolean(L, 2)) {
    /* 向后兼容：布尔参数控制是否合并重复键 */
    if (!lua_toboolean(L, 2)) {
      flags &= (uint16_t)~LQF_MERGE_DUPLICATES;
    }
  } else if (!lua_isnoneornil(L, 2)) {
    /* 数字参数：完整控制解析标志位 */
    flags = (uint16_t)luaL_checkinteger(L, 2);
  }

  char buffer[8192];
  char *buf;
  struct llquery query;
  enum llquery_error err;

  if (!input || !*input) {
    return 0;
  }

  // 初始化 llquery 解析器
  err = llquery_init(&query, 0, flags);

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

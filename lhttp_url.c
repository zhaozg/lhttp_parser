/***
 * URL utilities for Lua
 *
 * This module provides URL encoding, decoding, and parsing functions.
 * Based on https://github.com/moznion/lua-url-encode with memory leak fixes.
 *
 * @module lhttp_url
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#ifdef __cplusplus
}
#endif

#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <stdint.h>
#include <assert.h>
#include "llhttp_url.h"

#define UTF8_LEAD(c) ((uint8_t)(c) < 0x80 || ((uint8_t)(c) > 0xC1 && (uint8_t)(c) < 0xF5))
#define UTF8_TRAIL(c) (((uint8_t)(c) & 0xC0) == 0x80)

uint8_t utf8_len(const char* str);

static const uint8_t xdigit[16] = "0123456789ABCDEF";
static const int url_unreserved[256] =
{
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x00-0x0F */
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x10-0x1F */
  0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0, /* 0x20-0x2F */
  1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0, /* 0x30-0x3F */
  0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 0x40-0x4F */
  1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1, /* 0x50-0x5F */
  0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 0x60-0x6F */
  1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,0, /* 0x70-0x7F */
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x80-0x8F */
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x90-0x9F */
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xA0-0xAF */
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xB0-0xBF */
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xC0-0xCF */
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xD0-0xDF */
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xE0-0xEF */
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xF0-0xFF */
};

static char* _encode_url(const char* input)
{
  const long len = strlen((const char*)input);
  char* encoded = malloc(sizeof(uint8_t) * len * 3 + 1);
  int in_cursor = 0;
  int out_cursor = 0;

  while (input[in_cursor] != '\0')
  {
    const uint8_t charlen = utf8_len(&input[in_cursor]);

    if (charlen == 0)
    {
      continue;
    }

    if (charlen <= 1)
    {
      const uint8_t c = input[in_cursor];
      in_cursor += charlen;
      if (url_unreserved[c])
      {
        encoded[out_cursor++] = c;
      }
      else if (c == ' ')
      {
        encoded[out_cursor++] = '+';
      }
      else
      {
        encoded[out_cursor++] = '%';
        encoded[out_cursor++] = xdigit[c >> 4];
        encoded[out_cursor++] = xdigit[c & 15];
      }
      continue;
    }

    int i;
    for (i = 0; i < charlen; i++, in_cursor++)
    {
      const uint8_t c = input[in_cursor];
      encoded[out_cursor++] = '%';
      encoded[out_cursor++] = xdigit[c >> 4];
      encoded[out_cursor++] = xdigit[c & 15];
    }
  }
  encoded[out_cursor] = '\0';

  return encoded;
}

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
static int encode_url (lua_State* L)
{
  const char* input = luaL_checkstring(L, 1);
  char* encoded;

  if (input[0] == '\0')
    return 1;

  encoded = _encode_url(input);
  lua_pushstring(L, encoded);
  free(encoded);
  return 1;
}

#define __ 256
static const int hexval[256] =
{
  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* 00-0F */
  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* 10-1F */
  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* 20-2F */
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,__,__,__,__,__,__, /* 30-3F */
  __,10,11,12,13,14,15,__,__,__,__,__,__,__,__,__, /* 40-4F */
  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* 50-5F */
  __,10,11,12,13,14,15,__,__,__,__,__,__,__,__,__, /* 60-6F */
  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* 70-7F */
  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* 80-8F */
  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* 90-9F */
  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* A0-AF */
  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* B0-BF */
  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* C0-CF */
  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* D0-DF */
  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* E0-EF */
  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* F0-FF */
};
#undef __

static char* _decode_url(const char* input)
{
  const size_t len = strlen(input);
  char* decoded = malloc(sizeof(uint8_t) * len + 1);

  int in_cursor = 0;
  int out_cursor = 0;
  while (input[in_cursor] != '\0')
  {
    const uint8_t charlen = utf8_len(&input[in_cursor]);

    if (charlen == 0)
    {
      continue;
    }

    if (charlen <= 1)
    {
      const uint8_t c = input[in_cursor++];

      if (c == '+')
      {
        decoded[out_cursor++] = ' ';
        continue;
      }

      if (c != '%')
      {
        decoded[out_cursor++] = c;
        continue;
      }

      const unsigned int v1raw = input[in_cursor++];
      const unsigned int v2raw = input[in_cursor++];
      if (v1raw == 0x30 && v2raw == 0x30)
      {
        // null char termination (%00)
        return decoded;
      }

      const unsigned int v1 = hexval[v1raw];
      const unsigned int v2 = hexval[v2raw];
      if ((v1 | v2) != 0xFF)
      {
        decoded[out_cursor++] = (v1 << 4) | v2;
        continue;
      }

      decoded[out_cursor] = c;

      continue;
    }

    int i;
    for (i = 0; i < charlen; i++, in_cursor++)
    {
      const uint8_t c = input[in_cursor];
      decoded[out_cursor++] = c;
    }
  }
  decoded[out_cursor] = '\0';

  return decoded;
}

/***
 * Decode a URL-encoded string
 *
 * Decodes a URL-encoded string back to its original form.
 * Handles both '+' (as space) and percent-encoded characters.
 *
 * @function decode
 * @tparam string str URL-encoded string to decode
 * @treturn string Decoded string
 * @usage
 * local lurl = require('lhttp_url')
 * local decoded = lurl.decode("hello+world%21")
 * -- Returns: "hello world!"
 */
static int decode_url (lua_State* L)
{
  const char* input = luaL_checkstring(L, 1);
  char* decoded = _decode_url(input);
  lua_pushstring(L, decoded);
  free(decoded);
  return 1;
}

#define __ 0xFF
/*
 * 0x00: 0
 * 0x01-0xC1: 1
 * 0xF5-: 1
 */
static const uint8_t utf8_immediate_len[256] =
{
  0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0x00-0x0F */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0x10-0x1F */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0x20-0x2F */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0x30-0x3F */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0x40-0x4F */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0x50-0x5F */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0x60-0x6F */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0x70-0x7F */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0x80-0x8F */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0x90-0x9F */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0xA0-0xAF */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0xB0-0xBF */
  1, 1,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* 0xC0-0xCF */
  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* 0xD0-0xDF */
  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* 0xE0-0xEF */
  __,__,__,__,__, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0xF0-0xFF */
};
#undef __

/*
 * 0xC2-0xDF: 2
 * 0xE0-0xEF: 3
 * 0xF0-0xF4: 4
 */
static const uint8_t utf8_count_len[256] =
{
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x00-0x0F */
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x10-0x1F */
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x20-0x2F */
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x30-0x3F */
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x40-0x4F */
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x50-0x5F */
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x60-0x6F */
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x70-0x7F */
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x80-0x8F */
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x90-0x9F */
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xA0-0xAF */
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xB0-0xBF */
  0,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2, /* 0xC0-0xCF */
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, /* 0xD0-0xDF */
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3, /* 0xE0-0xEF */
  4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0, /* 0xF0-0xFF */
};

uint8_t utf8_len(const char* str)
{
  const uint8_t lead = *str;

  const uint8_t immediate_len = utf8_immediate_len[lead];
  if (immediate_len != 0xFF)
  {
    return immediate_len;
  }

  const uint8_t count = utf8_count_len[lead];
  uint8_t trail = *(++str);

  if (count == 3)
  {
    if ((lead == 0xE0 && 0xA0 > trail) || (lead == 0xED && trail > 0x9F))
    {
      return 1;
    }
  }
  else if (count == 4)
  {
    if ((lead == 0xF0 && 0x90 > trail) || (lead == 0xF4 && trail > 0x8F))
    {
      return 1;
    }
  }

  uint8_t size = 1;
  for (; size < count; ++size)
  {
    if (!UTF8_TRAIL(trail))
    {
      return size;
    }
    trail = *(++str);
  }
  return size;
}

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
 * @treturn[1] table URL components table with fields: protocol, auth, host, hostname, port, pathname, query, hash
 * @treturn[2] nil If parsing failed
 * @usage
 * local lurl = require('lhttp_url')
 * local parsed = lurl.parse("******example.com:8080/path?query=value#hash")
 * -- Returns a table with all parsed URL components
 */
static int lhttp_parser_parse_url (lua_State *L) {
  size_t len;
  const char *url = luaL_checklstring(L, 1, &len);
  int is_connect = lua_toboolean(L, 2);

  struct http_parser_url u;
  if (http_parser_parse_url(url, len, is_connect, &u)) {
    return 0;
  }

  lua_newtable(L);
  if (u.field_set & (1 << UF_SCHEMA)) {
    lua_pushlstring(L, url + u.field_data[UF_SCHEMA].off, u.field_data[UF_SCHEMA].len);
    lua_setfield(L, -2, "protocol");
  }
  if (u.field_set & (1 << UF_USERINFO)) {
    lua_pushlstring(L, url + u.field_data[UF_USERINFO].off, u.field_data[UF_USERINFO].len);
    lua_setfield(L, -2, "auth");
  }
  if (u.field_set & (1 << UF_HOST)) {
    lua_pushlstring(L, url + u.field_data[UF_HOST].off, u.field_data[UF_HOST].len);
    lua_setfield(L, -2, "hostname");
  }
  if (u.field_set & (1 << UF_PORT)) {
    lua_pushlstring(L, url + u.field_data[UF_PORT].off, u.field_data[UF_PORT].len);
    lua_setfield(L, -2, "port");
  }
  if (u.field_set & (1 << UF_PATH)) {
    lua_pushlstring(L, url + u.field_data[UF_PATH].off, u.field_data[UF_PATH].len);
    lua_setfield(L, -2, "pathname");
  }
  if (u.field_set & (1 << UF_QUERY)) {
    lua_pushlstring(L, url + u.field_data[UF_QUERY].off, u.field_data[UF_QUERY].len);
    lua_setfield(L, -2, "query");
  }
  if (u.field_set & (1 << UF_FRAGMENT)) {
    lua_pushlstring(L, url + u.field_data[UF_FRAGMENT].off, u.field_data[UF_FRAGMENT].len);
    lua_setfield(L, -2, "hash");
  }
  return 1;
}

LUALIB_API int luaopen_lhttp_url (lua_State *L)
{
  static const struct luaL_Reg R[] =
  {
    {"encode", encode_url},
    {"decode", decode_url},
    {"parse", lhttp_parser_parse_url},

    {NULL, NULL},
  };

  luaL_newlib(L, R);
  return 1;
};

#include "lhttp_parser.h"
#include "http_parser.h"

int lhttp_parser_parse_url (lua_State *L) {
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
    lua_setfield(L, -2, "schema");
  }
  if (u.field_set & (1 << UF_HOST)) {
    lua_pushlstring(L, url + u.field_data[UF_HOST].off, u.field_data[UF_HOST].len);
    lua_setfield(L, -2, "host");
  }
  if (u.field_set & (1 << UF_PORT)) {
    lua_pushlstring(L, url + u.field_data[UF_PORT].off, u.field_data[UF_PORT].len);
    lua_setfield(L, -2, "port_string");
    lua_pushnumber(L, u.port);
    lua_setfield(L, -2, "port");
  }
  if (u.field_set & (1 << UF_PATH)) {
    lua_pushlstring(L, url + u.field_data[UF_PATH].off, u.field_data[UF_PATH].len);
    lua_setfield(L, -2, "path");
  }
  if (u.field_set & (1 << UF_QUERY)) {
    lua_pushlstring(L, url + u.field_data[UF_QUERY].off, u.field_data[UF_QUERY].len);
    lua_setfield(L, -2, "query");
  }
  if (u.field_set & (1 << UF_FRAGMENT)) {
    lua_pushlstring(L, url + u.field_data[UF_FRAGMENT].off, u.field_data[UF_FRAGMENT].len);
    lua_setfield(L, -2, "fragment");
  }
  return 1;
}


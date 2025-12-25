#include "llhttp_url.h"
#include <assert.h>
#include <string.h>

#ifndef BIT_AT
#define BIT_AT(a, i)                              \
  (!!((unsigned int)(a)[(unsigned int)(i) >> 3] & \
      (1 << ((unsigned int)(i) & 7))))
#endif

#if LLHTTP_STRICT_MODE
#define T(v) 0
#else
#define T(v) v
#endif

static const uint8_t url_char_table[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0
};

static const uint8_t userinfo_char_table[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

static const uint8_t host_char_table[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};


#ifdef IS_URL_CHAR
#undef IS_URL_CHAR
#endif
#ifdef IS_USERINFO_CHAR
#undef IS_USERINFO_CHAR
#endif
#ifdef IS_HOST_CHAR
#undef IS_HOST_CHAR
#endif

#define IS_URL_CHAR(c) (url_char_table[(unsigned char)(c)])
#define IS_USERINFO_CHAR(c) (userinfo_char_table[(unsigned char)(c)])
#define IS_HOST_CHAR(c) (host_char_table[(unsigned char)(c)])

#undef T

enum state {
  s_dead = 1 /* important that this is > 0 */

  ,
  s_start_req_or_res,
  s_res_or_resp_H,
  s_start_res,
  s_res_H,
  s_res_HT,
  s_res_HTT,
  s_res_HTTP,
  s_res_http_major,
  s_res_http_dot,
  s_res_http_minor,
  s_res_http_end,
  s_res_first_status_code,
  s_res_status_code,
  s_res_status_start,
  s_res_status,
  s_res_line_almost_done

  ,
  s_start_req

  ,
  s_req_method,
  s_req_spaces_before_url,
  s_req_schema,
  s_req_schema_slash,
  s_req_schema_slash_slash,
  s_req_server_start,
  s_req_server,
  s_req_server_with_at,
  s_req_path,
  s_req_query_string_start,
  s_req_query_string,
  s_req_fragment_start,
  s_req_fragment,
  s_req_http_start,
  s_req_http_H,
  s_req_http_HT,
  s_req_http_HTT,
  s_req_http_HTTP,
  s_req_http_I,
  s_req_http_IC,
  s_req_http_major,
  s_req_http_dot,
  s_req_http_minor,
  s_req_http_end,
  s_req_line_almost_done

  ,
  s_header_field_start,
  s_header_field,
  s_header_value_discard_ws,
  s_header_value_discard_ws_almost_done,
  s_header_value_discard_lws,
  s_header_value_start,
  s_header_value,
  s_header_value_lws

  ,
  s_header_almost_done

  ,
  s_chunk_size_start,
  s_chunk_size,
  s_chunk_parameters,
  s_chunk_size_almost_done

  ,
  s_headers_almost_done,
  s_headers_done

  /* Important: 's_headers_done' must be the last 'header' state. All
   * states beyond this must be 'body' states. It is used for overflow
   * checking. See the PARSING_HEADER() macro.
   */

  ,
  s_chunk_data,
  s_chunk_data_almost_done,
  s_chunk_data_done

  ,
  s_body_identity,
  s_body_identity_eof

  ,
  s_message_done
};

enum http_host_state {
  s_http_host_dead = 1,
  s_http_userinfo_start,
  s_http_userinfo,
  s_http_host_start,
  s_http_host_v6_start,
  s_http_host,
  s_http_host_v6,
  s_http_host_v6_end,
  s_http_host_v6_zone_start,
  s_http_host_v6_zone,
  s_http_host_port_start,
  s_http_host_port
};

/* Macros for character classes; depends on strict-mode  */
#define CR '\r'
#define LF '\n'
#define LOWER(c) (unsigned char)(c | 0x20)
#define IS_ALPHA(c) (LOWER(c) >= 'a' && LOWER(c) <= 'z')
#define IS_NUM(c) ((c) >= '0' && (c) <= '9')
#define IS_ALPHANUM(c) (IS_ALPHA(c) || IS_NUM(c))
#define IS_HEX(c) (IS_NUM(c) || (LOWER(c) >= 'a' && LOWER(c) <= 'f'))
#define IS_MARK(c)                                                       \
  ((c) == '-' || (c) == '_' || (c) == '.' || (c) == '!' || (c) == '~' || \
   (c) == '*' || (c) == '\'' || (c) == '(' || (c) == ')')

#define STRICT_TOKEN(c) ((c == ' ') ? 0 : tokens[(unsigned char)c])

#if LLHTTP_STRICT_MODE
#define TOKEN(c) STRICT_TOKEN(c)
#else
static const uint8_t normal_url_char[32] = {
    /*   0 nul    1 soh    2 stx    3 etx    4 eot    5 enq    6 ack    7 bel */
    0 | 0 | 0 | 0 | 0 | 0 | 0 | 0,
    /*   8 bs     9 ht    10 nl    11 vt    12 np    13 cr    14 so    15 si */
    0 | T(2) | 0 | 0 | T(16) | 0 | 0 | 0,
    /*  16 dle   17 dc1   18 dc2   19 dc3   20 dc4   21 nak   22 syn   23 etb */
    0 | 0 | 0 | 0 | 0 | 0 | 0 | 0,
    /*  24 can   25 em    26 sub   27 esc   28 fs    29 gs    30 rs    31 us  */
    0 | 0 | 0 | 0 | 0 | 0 | 0 | 0,
    /*  32 sp    33  !    34  "    35  #    36  $    37  %    38  &    39  '  */
    0 | 2 | 4 | 0 | 16 | 32 | 64 | 128,
    /*  40  (    41  )    42  *    43  +    44  ,    45  -    46  .    47  /  */
    1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
    /*  48  0    49  1    50  2    51  3    52  4    53  5    54  6    55  7  */
    1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
    /*  56  8    57  9    58  :    59  ;    60  <    61  =    62  >    63  ?  */
    1 | 2 | 4 | 8 | 16 | 32 | 64 | 0,
    /*  64  @    65  A    66  B    67  C    68  D    69  E    70  F    71  G  */
    1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
    /*  72  H    73  I    74  J    75  K    76  L    77  M    78  N    79  O  */
    1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
    /*  80  P    81  Q    82  R    83  S    84  T    85  U    86  V    87  W  */
    1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
    /*  88  X    89  Y    90  Z    91  [    92  \    93  ]    94  ^    95  _  */
    1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
    /*  96  `    97  a    98  b    99  c   100  d   101  e   102  f   103  g  */
    1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
    /* 104  h   105  i   106  j   107  k   108  l   109  m   110  n   111  o  */
    1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
    /* 112  p   113  q   114  r   115  s   116  t   117  u   118  v   119  w  */
    1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
    /* 120  x   121  y   122  z   123  {   124  |   125  }   126  ~   127 del */
    1 | 2 | 4 | 8 | 16 | 32 | 64 | 0,
};

#define TOKEN(c) tokens[(unsigned char)c]
#define IS_URL_CHAR(c) \
  (BIT_AT(normal_url_char, (unsigned char)c) || ((c) & 0x80))
#define IS_HOST_CHAR(c) \
  (IS_ALPHANUM(c) || (c) == '.' || (c) == '-' || (c) == '_')
#endif

/* Our URL parser.
 *
 * This is designed to be shared by http_parser_execute() for URL validation,
 * hence it has a state transition + byte-for-byte interface. In addition, it
 * is meant to be embedded in http_parser_parse_url(), which does the dirty
 * work of turning state transitions URL components for its API.
 *
 * This function should only be invoked with non-space characters. It is
 * assumed that the caller cares about (and can detect) the transition between
 * URL and non-URL states by looking for these.
 */
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_STATES (s_req_fragment + 1)
#define NUM_CHARS 256
static enum state state_table[NUM_STATES][NUM_CHARS];

static void init_state_table() {
  memset(state_table, s_dead, sizeof(state_table));

  // s_req_spaces_before_url
  for (int c = 0; c < 256; ++c) {
    if (c == '/' || c == '*')
      state_table[s_req_spaces_before_url][c] = s_req_path;
    else if (IS_ALPHA(c))
      state_table[s_req_spaces_before_url][c] = s_req_schema;
    else
      state_table[s_req_spaces_before_url][c] = s_dead;
  }

  // s_req_schema
  for (int c = 0; c < 256; ++c) {
    if (IS_ALPHA(c))
      state_table[s_req_schema][c] = s_req_schema;
    else if (c == ':')
      state_table[s_req_schema][c] = s_req_schema_slash;
    else
      state_table[s_req_schema][c] = s_dead;
  }

  // s_req_schema_slash
  for (int c = 0; c < 256; ++c) {
    if (c == '/')
      state_table[s_req_schema_slash][c] = s_req_schema_slash_slash;
    else
      state_table[s_req_schema_slash][c] = s_dead;
  }

  // s_req_schema_slash_slash
  for (int c = 0; c < 256; ++c) {
    if (c == '/')
      state_table[s_req_schema_slash_slash][c] = s_req_server_start;
    else
      state_table[s_req_schema_slash_slash][c] = s_dead;
  }

  // s_req_server_with_at
  for (int c = 0; c < 256; ++c) {
    if (c == '@')
      state_table[s_req_server_with_at][c] = s_dead;
    else if (c == '/')
      state_table[s_req_server_with_at][c] = s_req_path;
    else if (c == '?')
      state_table[s_req_server_with_at][c] = s_req_query_string_start;
    else if (c == '@')
      state_table[s_req_server_with_at][c] = s_req_server_with_at;
    else if (IS_USERINFO_CHAR(c) || c == '[' || c == ']')
      state_table[s_req_server_with_at][c] = s_req_server;
    else
      state_table[s_req_server_with_at][c] = s_dead;
  }

  // s_req_server_start, s_req_server
  for (int c = 0; c < 256; ++c) {
    if (c == '/') {
      state_table[s_req_server_start][c] = s_req_path;
      state_table[s_req_server][c] = s_req_path;
    } else if (c == '?') {
      state_table[s_req_server_start][c] = s_req_query_string_start;
      state_table[s_req_server][c] = s_req_query_string_start;
    } else if (c == '@') {
      state_table[s_req_server_start][c] = s_req_server_with_at;
      state_table[s_req_server][c] = s_req_server_with_at;
    } else if (IS_USERINFO_CHAR(c) || c == '[' || c == ']') {
      state_table[s_req_server_start][c] = s_req_server;
      state_table[s_req_server][c] = s_req_server;
    } else {
      state_table[s_req_server_start][c] = s_dead;
      state_table[s_req_server][c] = s_dead;
    }
  }

  // s_req_path
  for (int c = 0; c < 256; ++c) {
    if (IS_URL_CHAR(c))
      state_table[s_req_path][c] = s_req_path;
    else if (c == '?')
      state_table[s_req_path][c] = s_req_query_string_start;
    else if (c == '#')
      state_table[s_req_path][c] = s_req_fragment_start;
    else
      state_table[s_req_path][c] = s_dead;
  }

  // s_req_query_string_start, s_req_query_string
  for (int c = 0; c < 256; ++c) {
    if (IS_URL_CHAR(c)) {
      state_table[s_req_query_string_start][c] = s_req_query_string;
      state_table[s_req_query_string][c] = s_req_query_string;
    } else if (c == '?') {
      state_table[s_req_query_string_start][c] = s_req_query_string;
      state_table[s_req_query_string][c] = s_req_query_string;
    } else if (c == '#') {
      state_table[s_req_query_string_start][c] = s_req_fragment_start;
      state_table[s_req_query_string][c] = s_req_fragment_start;
    } else {
      state_table[s_req_query_string_start][c] = s_dead;
      state_table[s_req_query_string][c] = s_dead;
    }
  }

  // s_req_fragment_start
  for (int c = 0; c < 256; ++c) {
    if (IS_URL_CHAR(c))
      state_table[s_req_fragment_start][c] = s_req_fragment;
    else if (c == '?')
      state_table[s_req_fragment_start][c] = s_req_fragment;
    else if (c == '#')
      state_table[s_req_fragment_start][c] = s_req_fragment_start;
    else
      state_table[s_req_fragment_start][c] = s_dead;
  }

  // s_req_fragment
  for (int c = 0; c < 256; ++c) {
    if (IS_URL_CHAR(c))
      state_table[s_req_fragment][c] = s_req_fragment;
    else if (c == '?' || c == '#')
      state_table[s_req_fragment][c] = s_req_fragment;
    else
      state_table[s_req_fragment][c] = s_dead;
  }
}

static inline enum state fast_parse_url_char(enum state s, unsigned char ch) {
  return state_table[s][ch];
}

static enum http_host_state http_parse_host_char(enum http_host_state s,
                                                 char const ch) {
  switch (s) {
    case s_http_userinfo:
    case s_http_userinfo_start:
      if (ch == '@') return s_http_host_start;
      if (IS_USERINFO_CHAR(ch)) return s_http_userinfo;
      break;
    case s_http_host_start:
      if (ch == '[') return s_http_host_v6_start;
      if (IS_HOST_CHAR(ch)) return s_http_host;
      break;
    case s_http_host:
      if (IS_HOST_CHAR(ch)) return s_http_host;
      // fall through
    case s_http_host_v6_end:
      if (ch == ':') return s_http_host_port_start;
      break;
    case s_http_host_v6:
      if (ch == ']') return s_http_host_v6_end;
      // fall through
    case s_http_host_v6_start:
      if (IS_HEX(ch) || ch == ':' || ch == '.') return s_http_host_v6;
      if (s == s_http_host_v6 && ch == '%') return s_http_host_v6_zone_start;
      break;
    case s_http_host_v6_zone:
      if (ch == ']') return s_http_host_v6_end;
      // fall through
    case s_http_host_v6_zone_start:
      if (IS_ALPHANUM(ch) || ch == '%' || ch == '.' || ch == '-' || ch == '_' ||
          ch == '~')
        return s_http_host_v6_zone;
      break;
    case s_http_host_port:
    case s_http_host_port_start:
      if (IS_NUM(ch)) return s_http_host_port;
      break;
    default:
      break;
  }
  return s_http_host_dead;
}

static int http_parse_host(char const* buf, struct http_parser_url* u,
                           int found_at) {
  enum http_host_state s;
  char const* p;
  size_t buflen = u->field_data[UF_HOST].off + u->field_data[UF_HOST].len;
  assert(u->field_set & (1 << UF_HOST));
  u->field_data[UF_HOST].len = 0;
  s = found_at ? s_http_userinfo_start : s_http_host_start;
  for (p = buf + u->field_data[UF_HOST].off; p < buf + buflen; p++) {
    enum http_host_state new_s = http_parse_host_char(s, *p);
    if (new_s == s_http_host_dead) return 1;
    switch (new_s) {
      case s_http_host:
        if (s != s_http_host) u->field_data[UF_HOST].off = (uint16_t)(p - buf);
        u->field_data[UF_HOST].len++;
        break;
      case s_http_host_v6:
        if (s != s_http_host_v6)
          u->field_data[UF_HOST].off = (uint16_t)(p - buf);
        u->field_data[UF_HOST].len++;
        break;
      case s_http_host_v6_zone_start:
      case s_http_host_v6_zone:
        u->field_data[UF_HOST].len++;
        break;
      case s_http_host_port:
        if (s != s_http_host_port) {
          u->field_data[UF_PORT].off = (uint16_t)(p - buf);
          u->field_data[UF_PORT].len = 0;
          u->field_set |= (1 << UF_PORT);
        }
        u->field_data[UF_PORT].len++;
        break;
      case s_http_userinfo:
        if (s != s_http_userinfo) {
          u->field_data[UF_USERINFO].off = (uint16_t)(p - buf);
          u->field_data[UF_USERINFO].len = 0;
          u->field_set |= (1 << UF_USERINFO);
        }
        u->field_data[UF_USERINFO].len++;
        break;
      default:
        break;
    }
    s = new_s;
  }
  switch (s) {
    case s_http_host_start:
    case s_http_host_v6_start:
    case s_http_host_v6:
    case s_http_host_v6_zone_start:
    case s_http_host_v6_zone:
    case s_http_host_port_start:
    case s_http_userinfo:
    case s_http_userinfo_start:
      return 1;
    default:
      break;
  }
  return 0;
}

int http_parser_parse_url(char const* buf, size_t buflen, int is_connect,
                          struct http_parser_url* u) {
  static int table_inited = 0;
  enum state s = is_connect ? s_req_server_start : s_req_spaces_before_url;
  char const *p = buf, *end = buf + buflen;
  enum http_parser_url_fields uf = UF_MAX, old_uf = UF_MAX;
  int found_at = 0;

  if (!table_inited) {
    init_state_table();
    table_inited = 1;
  }

  if (buflen == 0) return 1;
  u->port = u->field_set = 0;
  old_uf = UF_MAX;
  while (p < end) {
    // 批量跳过普通字符
    if (s == s_req_path || s == s_req_query_string || s == s_req_fragment) {
      char const* special = memchr(p, '?', end - p);
      char const* sharp = memchr(p, '#', end - p);
      char const* next = NULL;
      if (special && sharp)
        next = special < sharp ? special : sharp;
      else if (special)
        next = special;
      else if (sharp)
        next = sharp;
      else
        next = end;
      size_t n = next - p;
      if (n > 0) {
        if (uf == old_uf) {
          u->field_data[uf].len += n;
        } else {
          u->field_data[uf].off = (uint16_t)(p - buf);
          u->field_data[uf].len = n;
          u->field_set |= (1 << uf);
          old_uf = uf;
        }
        p = next;
        continue;
      }
    }
    s = fast_parse_url_char(s, (unsigned char)*p);
    switch (s) {
      case s_dead:
        return 1;
      case s_req_schema_slash:
      case s_req_schema_slash_slash:
      case s_req_server_start:
      case s_req_query_string_start:
      case s_req_fragment_start:
        ++p;
        continue;
      case s_req_schema:
        uf = UF_SCHEMA;
        break;
      case s_req_server_with_at:
        found_at = 1;
      // fall through
      case s_req_server:
        uf = UF_HOST;
        break;
      case s_req_path:
        uf = UF_PATH;
        break;
      case s_req_query_string:
        uf = UF_QUERY;
        break;
      case s_req_fragment:
        uf = UF_FRAGMENT;
        break;
      default:
        assert(!"Unexpected state");
        return 1;
    }
    if (uf == old_uf) {
      u->field_data[uf].len++;
    } else {
      u->field_data[uf].off = (uint16_t)(p - buf);
      u->field_data[uf].len = 1;
      u->field_set |= (1 << uf);
      old_uf = uf;
    }
    ++p;
  }
  if ((u->field_set & (1 << UF_SCHEMA)) &&
      (u->field_set & (1 << UF_HOST)) == 0) {
    return 1;
  }
  if (u->field_set & (1 << UF_HOST)) {
    if (http_parse_host(buf, u, found_at) != 0) {
      return 1;
    }
  }
  if (is_connect && u->field_set != ((1 << UF_HOST) | (1 << UF_PORT))) {
    return 1;
  }
  if (u->field_set & (1 << UF_PORT)) {
    uint16_t off = u->field_data[UF_PORT].off;
    uint16_t len = u->field_data[UF_PORT].len;
    char const* endp = buf + off + len;
    unsigned long v = 0;
    assert((size_t)(off + len) <= buflen && "Port number overflow");
    for (char const* pp = buf + off; pp < endp; ++pp) {
      v = v * 10 + (*pp - '0');
      if (v > 0xffff) return 1;
    }
    u->port = (uint16_t)v;
  }
  return 0;
}

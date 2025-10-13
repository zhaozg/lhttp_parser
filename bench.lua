#!/usr/bin/env lua
local clock = os.clock
local quiet = false
local disable_gc = true
local type = type
local tconcat = table.concat

if arg[1] == '-gc' then
  disable_gc = false
  table.remove(arg, 1)
else
  print "GC is disabled so we can track memory usage better"
  print ""
end

local N = tonumber(arg[1]) or 10000

local function printf(fmt, ...)
  local res
  if not quiet then
    fmt = fmt or ''
    res = print(string.format(fmt, ...))
    io.stdout:flush()
  end
  return res
end

local function full_gc()
  -- make sure all free-able memory is freed
  collectgarbage "collect"
  collectgarbage "collect"
  collectgarbage "collect"
end

local function bench(name, N, func, ...)
  local start1
  printf('run bench: %s', name)
  start1 = clock()
  func(N, ...)
  local diff1 = (clock() - start1)
  printf("total time: %10.6f seconds", diff1)
  return diff1
end

local lhp = require 'lhttp_parser'

local function parse_path_query_fragment(uri)
  local path, query, fragment, off
  -- parse path
  path, off = uri:match('([^?]*)()')
  -- parse query
  if uri:sub(off, off) == '?' then
    query, off = uri:match('([^#]*)()', off + 1)
  end
  -- parse fragment
  if uri:sub(off, off) == '#' then
    fragment = uri:sub(off + 1)
    off = #uri
  end
  return path or '/', query, fragment
end

local expects = {}
local requests = {}

-- NOTE: All requests must be version HTTP/1.1 since we re-use the same HTTP parser for all requests.
requests.ab = {
  "GET /foo/t.html?qstring#frag HTTP/1.1\r\nHost: localhost:8000\r\nUser-Agent: ApacheBench/2.3\r\nContent-Length: 5\r\nAccept: */*\r\n\r\nbody\n",
}

expects.ab = {
  method = "GET",
  url = "/foo/t.html?qstring#frag",
  path = "/foo/t.html",
  query_string = "qstring",
  fragment = "frag",
  headers = {
    Host = "localhost:8000",
    ["User-Agent"] = "ApacheBench/2.3",
    Accept = "*/*",
    ['Content-Length'] = '5'
  },
  body = "body\n",
}

requests.no_buff_body = {
  "GET / HTTP/1.1\r\n",
  "Host: foo:80\r\n",
  "Content-Length: 12\r\n",
  "\r\n",
  "chunk1", "chunk2",
}

expects.no_buff_body = {
  method = "GET",
  body = "chunk1chunk2",
  headers = {
    ['Content-Length'] = '12',
    Host = 'foo:80'
  },
  path = '/',
  url = '/'
}

requests.httperf = {
  "GET / HTTP/1.1\r\nHost: localhost\r\nUser-Agent: httperf/0.9.0\r\n\r\n"
}

expects.httperf = {
  method = "GET",
  url = "/",
  path = "/",
  headers = {
    Host = "localhost",
    ["User-Agent"] = "httperf/0.9.0",
  },
}

requests.firefox = {
  "GET / HTTP/1.1\r\nHost: two.local:8000\r\nUser-Agent: Mozilla/5.0 (X11; U;Linux i686; en-US; rv:1.9.0.15)Gecko/2009102815 Ubuntu/9.04 (jaunty)Firefox/3.0.15\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\nAccept-Language:en-gb,en;q=0.5\r\nAccept-Encoding: gzip,deflate\r\nAccept-Charset:ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\nKeep-Alive: 300\r\nConnection:keep-alive\r\n\r\n"
}

expects.firefox = {
  method = "GET",
  url = "/",
  path = "/",
  headers = {
    ["User-Agent"] = "Mozilla/5.0 (X11; U;Linux i686; en-US; rv:1.9.0.15)Gecko/2009102815 Ubuntu/9.04 (jaunty)Firefox/3.0.15",
    Accept = "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
    ["Accept-Language"] = "en-gb,en;q=0.5",
    ["Accept-Encoding"] = "gzip,deflate",
    ["Accept-Charset"] = "ISO-8859-1,utf-8;q=0.7,*;q=0.7",
    ["Keep-Alive"] = "300",
    Host = 'two.local:8000',
    Connection = "keep-alive",
  }
}

local names_list = {}
local data_list = {}
for name, data in pairs(requests) do
  names_list[#names_list + 1] = name
  data_list[#data_list + 1] = data
end

local function init_parser(reqs)
  local cur
  local parser

  local cb = {}
  function cb.onMessageBegin()
    assert(cur == nil)
    cur = { headers = {} }
  end

  function cb.onUrl(value)
    cur.url = value
    cur.path, cur.query_string, cur.fragment = parse_path_query_fragment(value)
  end

  function cb.onBody(value)
    if not cur.body then
      cur.body = value
    elseif nil ~= value then
      cur.body = cur.body .. value
    end
  end

  function cb.onHeaderField(field)
    cur.headers[#cur.headers + 1] = field
  end

  function cb.onHeaderValue(value)
    local field = cur.headers[#cur.headers]
    cur.headers[field] = value
  end

  function cb.onHeadersComplete()
    cur.method = parser:method()
  end

  function cb.onMessageComplete()
    assert(nil ~= cur)
    if reqs then table.insert(reqs, cur) end
    cur = nil
  end

  parser = lhp.new('request', cb)
  return parser
end

local function null_cb()
end

local null_cbs = {
  onMessageBegin = null_cb,
  onUrl = null_cb,
  onHeader = null_cb,
  onHeadersComplete = null_cb,
  onBody = null_cb,
  onMessageComplete = null_cb,
}
local function init_null_parser()
  return lhp.new('request', null_cbs)
end

local function good_client(parser, data)
  for i = 1, #data do
    local line = data[i]
    local bytes_read = parser:execute(line)
    if bytes_read ~= #line then
      error("only [" .. tostring(bytes_read) .. "] bytes read, expected [" .. tostring(#line) .. "]")
    end
  end
end

local function apply_client(N, client, parser, requests)
  for i = 1, N do
    for x = 1, #requests do
      client(parser, requests[x])
      parser:reset()
    end
  end
end

local function apply_client_memtest(name, client, N)
  local start_mem, end_mem

  local reqs = {}
  local parser = init_parser(reqs)
  full_gc()
  start_mem = (collectgarbage "count" * 1024)
  --print(name, 'start memory size: ', start_mem)
  if disable_gc then collectgarbage "stop" end
  apply_client(N, client, parser, data_list)
  end_mem = (collectgarbage "count" * 1024)
  --print(name, 'end   memory size: ', end_mem)
  print(name, 'N=', N, 'total memory used: ', (end_mem - start_mem))
  print()

  assert = require('luassert')
  -- validate parsed request data.
  local idx = 0
  for name, data in pairs(requests) do
    idx          = idx + 1
    local got    = reqs[idx]
    for i=#got.headers, 1, -1 do
      got.headers[i] = nil
    end
    local expect = expects[name]
    assert.same(got, expect, name)
  end

  reqs = nil
  parser = nil
  collectgarbage "restart"
  full_gc()
end

local function apply_client_speedtest(name, client, N)
  local start_mem, end_mem

  local parser = init_parser()
  full_gc()
  start_mem = (collectgarbage "count" * 1024)
  --print(name, 'start memory size: ', start_mem)
  if disable_gc then collectgarbage "stop" end
  local diff1 = bench(name, N, apply_client, client, parser, data_list)
  end_mem = (collectgarbage "count" * 1024)
  local total = N * #data_list
  printf("units/sec: %10.6f", total / diff1)
  --print(name, 'end   memory size: ', end_mem)
  print(name, 'N=', N, 'total memory used: ', (end_mem - start_mem))
  print()

  parser = nil
  collectgarbage "restart"
  full_gc()
end

local function per_parser_overhead(N)
  local start_mem, end_mem
  local parsers = {}

  -- pre-grow table
  for i = 1, N do
    parsers[i] = true -- add place-holder values.
  end
  full_gc()
  start_mem = (collectgarbage "count" * 1024)
  --print('overhead: start memory size: ', start_mem)
  for i = 1, N do
    parsers[i] = init_null_parser()
  end
  full_gc()
  end_mem = (collectgarbage "count" * 1024)
  --print('overhead: end   memory size: ', end_mem)
  print('overhead: total memory used: ', (end_mem - start_mem) / N, ' bytes per parser')

  parsers = nil
  full_gc()
end

local clients = {
  good = { cb = good_client, mem_N = 1, speed_N = N * 10 },
}

collectgarbage("collect")
collectgarbage("collect")
local before = collectgarbage("collect")
print('memory test')
for name, client in pairs(clients) do
  apply_client_memtest(name, client.cb, client.mem_N)
end

print('speed test')
for name, client in pairs(clients) do
  apply_client_speedtest(name, client.cb, client.speed_N)
end

print('overhead test')
per_parser_overhead(N)
collectgarbage("collect")
collectgarbage("collect")
local after = collectgarbage("collect")
print(string.format('Cost: %dK', (after - before)))

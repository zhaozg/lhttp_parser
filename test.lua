#!/usr/bin/env lua

local utils = require('utils')
local p = utils.prettyPrint
utils.stdout = io.stdout

local lhp = require('lhttp_parser')

p("lhttp_parser", lhp)

-- Make it easier to test
local src_dir, build_dir = ...
if (src_dir) then
  package.path = src_dir .. "?.lua;" .. package.path
  package.cpath = build_dir .. "?.so;" .. package.cpath
end

local counter, failure = 1, 0

function ok(assert_true, desc)
  if not assert_true then failure = failure + 1 end

  local msg = (assert_true and "ok " or "not ok ") .. counter
  if (desc) then msg = msg .. " - " .. desc end
  print(msg)
  counter = counter + 1
end

local function parse_path_query_fragment(uri)
  if lhp.llhttp then return end
  local url = lhp.parseUrl(uri, true)
  return url.path, url.query, url.fragment
end

local pipeline = [[
GET / HTTP/1.1
Host: localhost
User-Agent: httperf/0.9.0
Connection: keep-alive

GET /header.jpg HTTP/1.1
Host: localhost
User-Agent: httperf/0.9.0
Connection: keep-alive

]]
pipeline = pipeline:gsub('\n', '\r\n')

function pipeline_test()
  local cbs = {}
  local complete_count = 0
  local body = ''
  local headers = nil
  function cbs.onBody(chunk) if chunk then body = body .. chunk end end
  function cbs.onMessageComplete() complete_count = complete_count + 1 end
  function cbs.onHeadersComplete(info) headers = info end

  local parser = lhp.new('request', cbs)
  ok(parser:execute(pipeline) == #pipeline)

  ok(headers.should_keep_alive == true)
  ok(headers.method == "GET")
  ok(complete_count == 2)
  ok(#body == 0)
end

function parse_url_test()
  if lhp.llhttp then return end

  local tast_cases = {
    {
      "http://hello.com:8080/some/path?with=1%23&args=value", false, {
        schema = 'http',
        host = 'hello.com',
        port = 8080,
        path = '/some/path',
        query = 'with=1%23&args=value'
      }
    }, {
      "/foo/t.html?qstring#frag", true,
      {path = '/foo/t.html', query = 'qstring', fragment = 'frag'}
    }
  }

  for _, tast_case in ipairs(tast_cases) do
    local url, is_connect, expect = tast_case[1], tast_case[2], tast_case[3]
    local result = lhp.parseUrl(url, is_connect)
    is_deeply(result, expect, 'Url: ' .. url)
  end
end

function status_code_test()
  local response = {"HTTP/1.1 404 Not found", "", ""}
  local code, text
  local parser = lhp.new('response',
                         {onStatus = function(c, s) code, text = c, s end})
  parser:execute(table.concat(response, '\r\n'))
  ok(code == 404, 'Expected status code: 404, got ' .. tostring(code))
  ok(text == 'Not found',
     'Expected status text: `Not found`, got `' .. tostring(text) .. '`')
end

function chunk_header_test()
  local response = {"HTTP/1.1 200 OK", "Transfer-Encoding: chunked", "", ""}
  local content_length
  local parser = lhp.new('response',
                         {onChunkHeader = function(a) content_length = a end})

  parser:execute(table.concat(response, '\r\n'))

  content_length = nil
  parser:execute("23\r\n")
  ok(content_length == 0x23,
     "first chunk Content-Length expected: 0x23, got " ..
       (content_length and string.format("0x%2X", content_length) or 'nil'))
  parser:execute("This is the data in the first chunk\r\n")

  content_length = nil
  parser:execute("1A\r\n")
  ok(content_length == 0x1A,
     "second chunk Content-Length expected: 0x1A, got " ..
       (content_length and string.format("0x%2X", content_length) or 'nil'))
  parser:execute("abcdefghigklmnopqrstuvwxyz\r\n")

  content_length = nil
  parser:execute("FFFFFFFFFF\r\n")
  ok(content_length == 1099511627775,
     "Content-Length over than 32 bits: 1099511627775, got " ..
       (content_length and string.format("%g", content_length) or 'nil'))
end

function reset_test()
  local url

  local parser = lhp.new('request', {onUrl = function(u) url = u end})

  parser:execute('GET /path1 HTTP/1.1')

  parser:reinitialize('request')

  parser:execute('' .. 'POST /path2 HTTP/1.1\r\n' .. '\r\n')

  ok(parser:method() == 'POST', "reset should reinit parser")
  ok(url == '/path2', "reset should clear buffer and do not touch callbacks")
end

function reset_callback_test()
  local headers1, headers2, url = {}, {}

  local parser = lhp.new('request', {
    onHeaderField = function(h) headers1[#headers1 + 1] = h end,
    onHeaderValue = function(v) headers1[headers1[#headers1]] = v end,
    onUrl = function(u) url = u end
  })

  -- reset on_header and remove on_url
  parser:reinitialize('request', {
    onHeaderField = function(h) headers2[#headers2 + 1] = h end,
    onHeaderValue = function(v) headers2[headers2[#headers2]] = v end
  })

  parser:execute('' .. 'GET /path HTTP/1.1\r\n' .. 'A: b\r\n' .. '\r\n')

  ok(url == nil, "reset should remove callback")
  ok(headers1.A == nil and headers2.A == 'b', "reset should change callback")
end

-- NOTE: http-parser fails if the first response is HTTP 1.0:
-- HTTP/1.0 100 Please continue mate.
-- Which I think is a HTTP spec violation, but other HTTP clients, still work.
-- http-parser will fail by seeing only one HTTP response and putting everything elses in
-- the response body for the first 100 response, until socket close.
local please_continue = [[
HTTP/1.1 100 Please continue mate.

HTTP/1.1 200 OK
Date: Wed, 02 Feb 2011 00:50:50 GMT
Content-Length: 10
Connection: close

0123456789]]
pipeline = pipeline:gsub('\r', ''):gsub('\n', '\r\n')

function please_continue_test()
  local cbs = {}
  local complete_count = 0
  local body = ''
  local headers = nil

  function cbs.onBody(chunk) if chunk then body = body .. chunk end end
  function cbs.onMessageComplete() complete_count = complete_count + 1 end
  function cbs.onHeadersComplete(info) headers = info end

  local parser = lhp.new('response', cbs)
  parser:execute(please_continue)

  ok(headers.should_keep_alive == false)
  ok(headers.status_code == 200)
  ok(complete_count == 2)
  ok(#body == 10)
end

local connection_close = [[
HTTP/1.1 200 OK
Date: Wed, 02 Feb 2011 00:50:50 GMT
Connection: close

0123456789]]
connection_close = connection_close:gsub('\n', '\r\n')

function connection_close_test()
  local cbs = {}
  local complete_count = 0
  local body = ''
  local headers = nil

  function cbs.onBody(chunk) if chunk then body = body .. chunk end end
  function cbs.onMessageComplete() complete_count = complete_count + 1 end
  function cbs.onHeadersComplete(info) headers = info end

  local parser = lhp.new('response', cbs)
  parser:execute(connection_close)
  parser:finish()
  ok(headers.should_keep_alive == false, "should_keep_alive == false")
  ok(headers.status_code == 200, "status_code == 200")
  ok(complete_count == 1, "complete_count == 1")
  ok(#body == 10, "#body==10")
end

function nil_body_test()
  local cbs = {}
  local body_count = 0
  local body = {}
  function cbs.onBody(chunk)
    body[#body + 1] = chunk
    body_count = body_count + 1
  end

  local parser = lhp.new('request', cbs)
  parser:execute("GET / HTTP/1.1\r\n")
  parser:execute("Transfer-Encoding: chunked\r\n")
  parser:execute("\r\n")
  parser:execute("23\r\n")
  parser:execute("This is the data in the first chunk\r\n")
  parser:execute("1C\r\n")
  parser:execute("X and this is the second one\r\n")
  ok(body_count == 2, "body_count == 2")

  is_deeply(body, {
    "This is the data in the first chunk", "X and this is the second one"
  })

  -- This should cause on_body(nil) to be sent
  parser:execute("0\r\n\r\n")

  ok(body_count == 2, "body_count == 2")
  ok(#body == 2, "#body == 2")
end

function max_events_test(N)
  N = N or 3000

  -- The goal of this test is to generate the most possible events
  local input_tbl = {"GET / HTTP/1.1\r\n"}
  -- Generate enough events to trigger a "stack overflow"
  local header_cnt = N
  for i = 1, header_cnt do input_tbl[#input_tbl + 1] = "a:\r\n" end
  input_tbl[#input_tbl + 1] = "\r\n"

  local cbs = {}
  local field_cnt = 0
  function cbs.on_header(field, value) field_cnt = field_cnt + 1 end

  local parser = lhp.new('request', cbs)
  local input = table.concat(input_tbl)
  local result = parser:execute(input)

  N = N * 2
  if (#input == result) and (N < 100000) then return max_events_test(N) end

  input = input:sub(result + 1)

  -- We should have generated a stack overflow event that should be
  -- handled gracefully... note that
  ok(#input < result,
     "Expect " .. header_cnt .. " field events, got " .. field_cnt)

  result = parser:execute(input)

  ok(0 == result, "Parser can not continue after stack overflow [" ..
       tostring(result) .. "]")
end

function regression_no_body_cb_test()
  -- The goal of this test is to generate the most possible events
  local input_tbl = {"GET / HTTP/1.1\r\n", "Header: value\r\n", "\r\n"}

  local parser = lhp.new('request', {})

  local input = table.concat(input_tbl)

  local result = parser:execute(input)
  ok(result == #input, 'can work without on_body callback')
end

function basic_tests()
  local expects = {}
  local requests = {}

  -- NOTE: All requests must be version HTTP/1.1 since we re-use the same HTTP parser for all requests.
  requests.ab = {
    "GET /", "foo/t", ".html?", "qst", "ring", "#fr", "ag ", "HTTP/1.1\r\n",
    "Ho", "st: loca", "lhos", "t:8000\r\nUser-Agent: ApacheBench/2.3\r\n",
    "Con", "tent-L", "ength", ": 5\r\n", "Accept: */*\r\n\r", "\nbody\n"
  }

  expects.ab = {
    path = "/foo/t.html",
    query_string = "qstring",
    fragment = "frag",
    url = "/foo/t.html?qstring#frag",
    headers = {
      Host = "localhost:8000",
      ["User-Agent"] = "ApacheBench/2.3",
      Accept = "*/*"
    },
    body = {"body\n"}
  }
  if lhp.llhttp then
    expects.ab.path = nil
    expects.ab.query_string = nil
    expects.ab.fragment = nil
  end

  requests.no_buff_body = {
    "GET / HTTP/1.1\r\n", "Host: foo:80\r\n", "Content-Length: 12\r\n", "\r\n",
    "chunk1", "chunk2"
  }

  expects.no_buff_body = {body = {"chunk1", "chunk2"}}

  requests.httperf = {
    "GET / HTTP/1.1\r\n", "Host: localhost\r\n",
    "User-Agent: httperf/0.9.0\r\n\r\n"
  }

  expects.httperf = {}

  requests.firefox = {
    "GET / HTTP/1.1\r\n", "Host: two.local:8000\r\n",
    "User-Agent: Mozilla/5.0 (X11; U;", "Linux i686; en-US; rv:1.9.0.15)",
    "Gecko/2009102815 Ubuntu/9.04 (jaunty)", "Firefox/3.0.15\r\n",
    "Accept: text/html,application/xhtml+xml,application/xml;",
    "q=0.9,*/*;q=0.8\r\n", "Accept-Language:en-gb,en;q=0.5\r\n",
    "Accept-Encoding: gzip,deflate\r\n",
    "Accept-Charset:ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\n", "Keep-Alive: 300\r\n",
    "Connection:keep-alive\r\n\r\n"
  }

  expects.firefox = {
    headers = {
      ["User-Agent"] = "Mozilla/5.0 (X11; U;Linux i686; en-US; rv:1.9.0.15)Gecko/2009102815 Ubuntu/9.04 (jaunty)Firefox/3.0.15"
    }
  }

  local parser, reqs = init_parser()

  for name, data in pairs(requests) do
    for _, line in ipairs(data) do
      local bytes_read = parser:execute(line)
      ok(bytes_read == #line,
         "only [" .. tostring(bytes_read) .. "] bytes read, expected [" ..
           tostring(#line) .. "] in " .. name)
    end

    local got = reqs[#reqs]
    local expect = expects[name]
    ok(got.info.method == "GET", "Method is GET")
    is_deeply(got, expect, "Check " .. name)
  end
end

function init_parser()
  local reqs = {}
  local cur = nil
  local cb = {}

  function cb.onMessageBegin()
    ok(cur == nil)
    cur = {headers = {}}
  end

  function cb.onUrl(value)
    cur.url = cur.url and (cur.url .. value) or value;
    if not lhp.llhttp then
      cur.path, cur.query_string, cur.fragment =
        parse_path_query_fragment(cur.url)
    end
  end

  function cb.onBody(value)
    if (nil == cur.body) then cur.body = {} end
    table.insert(cur.body, value)
  end

  function cb.onHeaderField(field)
    cur.field = cur.field and (cur.field .. field) or field
  end

  function cb.onHeaderValue(value)
    local field
    if cur.field then
      cur.headers[#cur.headers + 1] = cur.field
      field = cur.field
      cur.field = nil
    else
      field = cur.headers[#cur.headers]
    end
    local t = cur.headers[field]
    if t then
      cur.headers[field] = t .. value
    else
      cur.headers[field] = value
    end
  end

  function cb.onMessageComplete()
    ok(nil ~= cur)
    table.insert(reqs, cur)
    cur = nil
  end

  function cb.onHeadersComplete(info)
    ok(cur.info == nil)
    cur.info = info
  end

  return lhp.new('request', cb), reqs
end

function is_deeply(got, expect, msg, context)
  if (type(expect) ~= "table") then
    print("# Expected [" .. context .. "] to be a table")
    ok(false, msg)
    return false
  end
  for k, v in pairs(expect) do
    local ctx
    if (nil == context) then
      ctx = k
    else
      ctx = context .. "." .. k
    end
    if type(expect[k]) == "table" then
      if (not is_deeply(got[k], expect[k], msg, ctx)) then return false end
    else
      if (got[k] ~= expect[k]) then
        print("# Expected [" .. ctx .. "] to be '" .. tostring(expect[k]) ..
                "', but got '" .. tostring(got[k]) .. "'")
        print('god')
        p(got)
        print('expect')
        p(expect)
        ok(false, msg)
        return false
      end
    end
  end
  if (nil == context) then ok(true, msg); end
  return true
end

basic_tests()
max_events_test()
nil_body_test()
pipeline_test()
please_continue_test()
connection_close_test()
regression_no_body_cb_test()
status_code_test()
chunk_header_test()
parse_url_test()
reset_test()
reset_callback_test()

print("1.." .. counter - 1)
if failure > 0 then os.exit(-1) end

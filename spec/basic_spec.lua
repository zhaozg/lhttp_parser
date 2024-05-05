describe('lhttp_parser basic unit testing', function()

  local lhp = require('lhttp_parser')
  local lurl = require('lhttp_url')

  local function init_parser()
    local reqs = {}
    local cb = {}
    local cur
    local parser

    function cb.onMessageBegin()
      assert(cur == nil)
      cur = { headers = {} }
    end

    function cb.onReset()
      cur = nil
    end

    function cb.onUrl(value)
      cur.url = cur.url and (cur.url .. value) or value;
    end

    function cb.onBody(value)
      if (nil == cur.body) then cur.body = {} end
      table.insert(cur.body, value)
    end

    function cb.onHeaderField(field)
      if not cur.path then
        local url = lurl.parse(cur.url, parser:method() == 'CONNECT')
        cur.path, cur.query, cur.fragment = url.path, url.query, url.fragment
      end
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
      assert(nil ~= cur)
      table.insert(reqs, cur)
      cur = nil
    end

    function cb.onHeadersComplete(info)
      assert(cur.info == nil)
      cur.info = info
    end

    parser = lhp.new('request', cb)
    return parser, reqs
  end

  it("lhttp_parser pipeline", function()
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

    local cbs = {}
    local complete_count = 0
    local body = ''
    local headers
    function cbs.onBody(chunk) if chunk then body = body .. chunk end end

    function cbs.onMessageComplete()
      complete_count = complete_count + 1
    end

    function cbs.onHeadersComplete(info)
      headers = info
    end

    function cbs.onReset()
      body = ''
      headers = nil
    end

    local parser = lhp.new('request', cbs)
    assert(parser:execute(pipeline) == #pipeline)

    assert(headers.http_major == 1)
    assert(headers.http_minor == 1)
    assert(headers.should_keep_alive == true)
    assert(headers.method == "GET")
    assert(complete_count == 2)
    assert(#body == 0)
  end)

  it("lhttp_parser status_code", function()
    local response = { "HTTP/1.1 404 Not found", "", "" }
    local code, text
    local parser = lhp.new('response',
      { onStatus = function(c, s) code, text = c, s end })
    parser:execute(table.concat(response, '\r\n'))
    assert(code == 404, 'Expected status code: 404, got ' .. tostring(code))
    assert(text == 'Not found',
      'Expected status text: `Not found`, got `' .. tostring(text) .. '`')
  end)

  it("lhttp_parser chunk_header", function()
    local response = { "HTTP/1.1 200 assert", "Transfer-Encoding: chunked", "", "" }
    local content_length
    local parser = lhp.new('response',
      {
        onChunkHeader = function(a) content_length = a end,
        onHeadersComplete = function() end
      })

    assert(parser:execute(table.concat(response, '\r\n')))

    content_length = nil
    assert(parser:execute("23\r\n"))
    assert(content_length == 0x23,
      "first chunk Content-Length expected: 0x23, got " ..
      (content_length and string.format("0x%2X", content_length) or 'nil'))
    parser:execute("This is the data in the first chunk\r\n")

    content_length = nil
    parser:execute("1A\r\n")
    assert(content_length == 0x1A,
      "second chunk Content-Length expected: 0x1A, got " ..
      (content_length and string.format("0x%2X", content_length) or 'nil'))
    parser:execute("abcdefghigklmnopqrstuvwxyz\r\n")

    content_length = nil
    parser:execute("FFFFFFFFFF\r\n")
    assert(content_length == 1099511627775,
      "Content-Length over than 32 bits: 1099511627775, got " ..
      (content_length and string.format("%g", content_length) or 'nil'))
  end)

  it("lhttp_parser reset", function()
    local url

    local parser = lhp.new('request', { onUrl = function(u) url = u end })

    parser:execute('GET /path1 HTTP/1.1')
    assert(parser:method() == 'GET')
    assert(url == '/path1')

    parser:reset('request')

    parser:execute('' .. 'POST /path2 HTTP/1.1\r\n' .. '\r\n')

    assert(parser:method() == 'POST', "reset should reinit parser")
    assert(url == '/path2', "reset should clear buffer and do not touch callbacks")
  end)

  it("lhttp_parser reset callback", function()
    local headers1, headers2, url, headers = {}, {}, nil, {}

    local parser = lhp.new('request', {
      onHeaderField = function(h) headers[#headers + 1] = h end,
      onHeaderValue = function(v) headers[headers[#headers]] = v end,
      onUrl = function(u) url = u end
    })

    headers = headers1
    parser:execute('' .. 'GET /path HTTP/1.1\r\n' .. 'A: b\r\n' .. '\r\n')

    assert(url == '/path')

    -- reset
    parser:reset()
    headers = headers2
    parser:execute('' .. 'GET /path HTTP/1.1\r\n' .. 'A: b\r\n' .. '\r\n')

    assert(url == '/path')
    assert(headers1.A == headers2.A)
    assert(headers1.A == 'b')
  end)


  it("lhttp_parser continue", function()
    -- NOTE: http-parser fails if the first response is HTTP 1.0:
    -- HTTP/1.0 100 Please continue mate.
    -- Which I think is a HTTP spec violation, but other HTTP clients, still work.
    -- http-parser will fail by seeing only one HTTP response and putting everything elses in
    -- the response body for the first 100 response, until socket close.
    local please_continue = [[
HTTP/1.1 100 Please continue mate.

HTTP/1.1 200 assert
Date: Wed, 02 Feb 2011 00:50:50 GMT
Content-Length: 10
Connection: close

0123456789]]
    please_continue = please_continue:gsub('\r', ''):gsub('\n', '\r\n')
    local cbs = {}
    local complete_count = 0
    local body = ''
    local headers

    function cbs.onBody(chunk) if chunk then body = body .. chunk end end

    function cbs.onMessageComplete() complete_count = complete_count + 1 end

    function cbs.onHeadersComplete(info)
      headers = info
      if complete_count == 0 then
        return 2
      end
    end

    local parser = lhp.new('response', cbs)
    local code, status = parser:execute(please_continue)
    assert(code==38)
    assert(status=='HPE_PAUSED_UPGRADE')
    assert(headers.should_keep_alive == true)
    assert(headers.status_code == 100)
    assert(complete_count == 1)
    assert(#body == 0)

    code, status = parser:resume_after_upgrade():execute(please_continue, code+1)
    assert(code==109, code)
    assert(status=='HPE_OK')
    assert(headers.should_keep_alive == false)
    assert(headers.status_code == 200)
    assert(complete_count == 2)
    assert(#body == 10)
  end)


  it("lhttp_parser connection close", function()
    local connection_close = [[
HTTP/1.1 200 assert
Date: Wed, 02 Feb 2011 00:50:50 GMT
Connection: close

0123456789]]
    connection_close = connection_close:gsub('\n', '\r\n')
    local cbs = {}
    local complete_count = 0
    local body = ''
    local headers

    function cbs.onBody(chunk) if chunk then body = body .. chunk end end

    function cbs.onMessageComplete() complete_count = complete_count + 1 end

    function cbs.onHeadersComplete(info) headers = info end

    local parser = lhp.new('response', cbs)
    parser:execute(connection_close)
    parser:finish()
    assert(headers.should_keep_alive == false, "should_keep_alive == false")
    assert(headers.status_code == 200, "status_code == 200")
    assert(complete_count == 1, "complete_count == 1")
    assert(#body == 10, "#body==10")
  end)

  it("lhttp_parser nil body", function()
    local cbs = {}
    local body_count = 0
    local body = {}
    function cbs.onBody(chunk)
      body[#body + 1] = chunk
      body_count = body_count + 1
    end
    function cbs.onHeadersComplete() end

    local parser = lhp.new('request', cbs)
    assert(parser:execute("GET / HTTP/1.1\r\n"))
    assert(parser:execute("Transfer-Encoding: chunked\r\n"))
    assert(parser:execute("\r\n"))
    assert(parser:execute("23\r\n"))
    assert(parser:execute("This is the data in the first chunk\r\n"))
    assert(parser:execute("1C\r\n"))
    assert(parser:execute("X and this is the second one\r\n"))
    assert(body_count == 2, "body_count == 2")

    assert.same(body, {
      "This is the data in the first chunk", "X and this is the second one"
    })

    -- This should cause on_body(nil) to be sent
    parser:execute("0\r\n\r\n")

    assert(body_count == 2, "body_count == 2")
    assert(#body == 2, "#body == 2")
  end)

  function max_events_test(N)
    N = N or 3000

    -- The goal of this test is to generate the most possible events
    local input_tbl = { "GET / HTTP/1.1\r\n" }
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
    assert(#input < result,
      "Expect " .. header_cnt .. " field events, got " .. field_cnt)

    result = parser:execute(input)

    assert(0 == result, "Parser can not continue after stack overflow [" ..
      tostring(result) .. "]")
  end

  it("lhttp_parser nil body cb", function()
    -- The goal of this test is to generate the most possible events
    local input_tbl = { "GET / HTTP/1.1\r\n", "Header: value\r\n", "\r\n" }

    local parser = lhp.new('request', {})

    local input = table.concat(input_tbl)

    local result, status = parser:execute(input)
    assert(result == nil)
    assert(status == 'HPE_CB_HEADERS_COMPLETE')
  end)

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
    fragment = "frag",
    url = "/foo/t.html?qstring#frag",
    headers = {
      Host = "localhost:8000",
      ["User-Agent"] = "ApacheBench/2.3",
      Accept = "*/*"
    },
    body = { "body\n" }
  }

  requests.no_buff_body = {
    "GET / HTTP/1.1\r\n", "Host: foo:80\r\n", "Content-Length: 12\r\n", "\r\n",
    "chunk1", "chunk2"
  }

  expects.no_buff_body = { body = { "chunk1", "chunk2" } }

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
    it("lhttp_parser basic:" .. name, function()
      for _, line in ipairs(data) do
        local bytes_read = parser:execute(line)
        assert(bytes_read == #line,
          "only [" .. tostring(bytes_read) .. "] bytes read, expected [" ..
          tostring(#line) .. "] in " .. name)
      end

      local got = reqs[#reqs]
      local expected = expects[name]
      assert(got.info.method == "GET", "Method is GET")
      if type(expected.headers) == 'table' then
        for i=1, #expected.headers do
          local k = expected.headers[i]
          assert(got.headers[k] == expected.headers[k])
        end
      end
      if expected.body then
        assert.same(expected.body, got.body)
      end
    end)
  end

end)

describe('lhttp_parser llhttp', function()

  local lhp = require('lhttp_parser')

  local callbacks = {
    onMessageBegin = function() end,
    onUrl = function(url) end,
    onHeaderField = function(field) end,
    onHeaderValue = function(value) end,
    onHeadersComplete = function(info) end,
    onBody = function(body) end,
    onMessageComplete = function() end
  }

  it("llhttp Basic HTTP request parsing", function()
    local parser = lhp.new('request', callbacks)
    local request = "GET /path HTTP/1.1\r\nHost: example.com\r\n\r\n"
    local bytes, err = parser:execute(request)
    assert(bytes == #request, "Should parse all bytes")
    assert(err == "HPE_OK", "Should have no errors")
    parser:reset()
  end)

  it("llhttp Error handling for invalid requests", function()
    local parser = lhp.new('request', callbacks)
    local invalid = "INVALID REQUEST"
    local bytes, err = parser:execute(invalid)
    assert(bytes == nil)
    assert(err == "HPE_INVALID_METHOD", "Should detect error in invalid request")
    --TODO:
  end)

  it("llhttp Memory leak test (creating 1000 parsers)", function()
    collectgarbage("collect")
    collectgarbage("collect")
    local before = collectgarbage("count")
    local invalid = "INVALID REQUEST"
    for _ = 1, 1000 do
      local parser = lhp.new('request', callbacks)
      local bytes, err = parser:execute(invalid)
      assert(bytes == nil)
      assert(err == "HPE_INVALID_METHOD", "Should detect error in invalid request")
      parser = nil
    end
    collectgarbage("collect")
    collectgarbage("collect")
    local after = collectgarbage("count")
    local leaked = after - before
    -- print(string.format("Memory before: %.2f KB", before))
    -- print(string.format("Memory after: %.2f KB", after))
    -- print(string.format("Leaked: %.2f KB", leaked))
    -- Allow some overhead but it shouldn't be proportional to the number of parsers
    assert(leaked < 10, "Memory leak detected!")
  end)

  it("llhttp Parser reuse with reset()", function()
    local parser = lhp.new('request', callbacks)
    local request = "GET /path HTTP/1.1\r\nHost: example.com\r\n\r\n"
    local bytes, err = parser:execute(request)
    assert(bytes == #request, "Should parse all bytes after reset")
    assert(err == "HPE_OK", "Should have no errors after reset")
    parser:reset()
    bytes, err= parser:execute(request)
    assert(bytes == #request, "Should parse all bytes after reset")
    assert(err == "HPE_OK", "Should have no errors after reset")
  end)

  it("llhttp Chunked transfer encoding", function()
    local body_chunks = {}
    local chunk_callbacks = {
      onHeadersComplete = function(info) end,
      onChunkHeader = function(size)
        -- print("Chunk size:", size)
      end,
      onBody = function(data)
        table.insert(body_chunks, data)
      end,
      onChunkComplete = function()
        -- print("Chunk complete")
      end,
      onMessageComplete = function()
        -- print("Message complete")
      end
    }

    local chunk_parser = lhp.new('request', chunk_callbacks)
    local chunked_req = "POST / HTTP/1.1\r\n" ..
                        "Host: example.com\r\n" ..
                        "Transfer-Encoding: chunked\r\n\r\n" ..
                        "5\r\nHello\r\n" ..
                        "6\r\n World\r\n" ..
                        "0\r\n\r\n"
    local bytes, err = chunk_parser:execute(chunked_req)
    assert(bytes == #chunked_req)
    assert(err == "HPE_OK", "Should parse chunked encoding: " .. tostring(err))
    assert(table.concat(body_chunks) == 'Hello World')
  end)

  it("llhttp Malformed headers handling", function()
    local exception_parser = lhp.new('request', {onHeadersComplete = function() end})
    local malformed_headers = "GET / HTTP/1.1\r\nHost: example.com\r\nInvalid-Header\r\n\r\n"
    local bytes, err= exception_parser:execute(malformed_headers)
    assert(bytes==nil)
    assert(err == 'HPE_INVALID_HEADER_TOKEN')
  end)

  it("llhttp Incomplete request handling", function()
    local exception_parser = lhp.new('request', {onHeadersComplete = function() end})
    local incomplete = "GET / HTTP/1.1\r\nHost: exa"
    local bytes, err = exception_parser:execute(incomplete)
    assert(bytes, #incomplete+1)
    assert(err == 'HPE_OK')
  end)

  it("llhttp Oversized header handling", function()
    local exception_parser = lhp.new('request', {onHeadersComplete = function() end})
    local oversized = "GET / HTTP/1.1\r\nHost: " .. string.rep("x", 10000) .. "\r\n\r\n"
    local bytes, err = exception_parser:execute(oversized)
    assert(bytes == #oversized)
    assert(err == 'HPE_OK')
  end)

  it("llhttp Multiple parsers working simultaneously", function()
    local parsers = {}
    for i = 1, 10 do
      parsers[i] = lhp.new('request', {
        onHeadersComplete = function() end
      })
    end
    for i = 1, 10 do
      local req = "GET /test" .. i .. " HTTP/1.1\r\nHost: test.com\r\n\r\n"
      local b, e = parsers[i]:execute(req)
      assert(e == "HPE_OK", "Parser " .. i .. " should succeed")
    end
  end)

  it("llhttp Error codes verification", function ()
    assert(lhp.OK == 0, "OK should be 0")
    assert(lhp.INTERNAL ~= nil, "INTERNAL should be defined")
    assert(lhp.INVALID_METHOD ~= nil, "INVALID_METHOD should be defined")
    assert(lhp.VERSION_MAJOR ~= nil, "VERSION_MAJOR should be defined")
  end)

  it("llhttp All parser methods are available", function ()
    local test_parser = lhp.new('request', {})
    assert(type(test_parser.execute) == "function", "execute should be a function")
    assert(type(test_parser.finish) == "function", "finish should be a function")
    assert(type(test_parser.reset) == "function", "reset should be a function")
    assert(type(test_parser.http_version) == "function", "http_version should be a function")
    assert(type(test_parser.http_errno) == "function", "http_errno should be a function")
  end)

  it("llhttp Response parser functionality", function ()
    local response_parser = lhp.new('response', {
      onStatus = function(code, text) end,
      onHeadersComplete = function(info) end
    })
    local response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n"
    local bytes, err = response_parser:execute(response)
    assert(bytes == #response)
    assert(err == "HPE_OK", "Should parse response successfully")
  end)

end)

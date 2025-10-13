describe('lhttp_url', function()
  local URL = require('lhttp_url')

  local function is_deeply(got, expect, msg, context)
    if (type(expect) ~= "table") then
      print("# Expected [" .. context .. "] to be a table")
      assert(false, msg)
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
          assert(false, msg)
          return false
        end
      end
    end
    if (nil == context) then assert(true, msg); end
    return true
  end

  it("lhttp_parser URL parsing basic", function()
    local url = URL.parse("https://user:pass@example.com:8080/path?query=value#hash")
    assert(url ~= nil, "Should parse URL successfully")
    assert(url.schema == "https", "Protocol should be https")
    assert(url.port == 8080, "Port should be 8080")
  end)

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
      "/foo/t.html?qstring#frag", false,
      { path = '/foo/t.html', query = 'qstring', fragment = 'frag' }
    }, {
      "192.168.0.1:80", true,
      { port = 80, port_string = "80", host = "192.168.0.1" }
    }
  }

  for _, tast_case in ipairs(tast_cases) do
    it("lhttp_parser url parse:#" .. tast_case[1], function()
      local url, is_connect, expect = tast_case[1], tast_case[2], tast_case[3]
      local result = URL.parse(url, is_connect)
      is_deeply(result, expect, 'Url: ' .. url)
    end)
  end

  it("lhttp_parser URL encoding and decoding", function()
    local original = "hello world & special chars: @#$%"
    local encoded = URL.encode(original)
    local decoded = URL.decode(encoded)
    assert(decoded == original, "Decoded should match original")
  end)

  it("lhttp_parser URL parsing edge cases", function()
    -- Test with no path
    local url1 = URL.parse("http://example.com")
    assert(url1 ~= nil, "Should parse URL without path")

    -- Test with IPv6
    local url2 = URL.parse("http://[::1]:8080/path")
    assert(url2 ~= nil, "Should parse IPv6 URL")
  end)
end)

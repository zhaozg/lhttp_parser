describe('lhttp_url', function()
  local URL = require('lhttp_url')

  it("lhttp_parser URL parsing basic", function()
    local url = URL.parse("https://user:pass@example.com:8080/path?query=value#hash")
    assert(url ~= nil, "Should parse URL successfully")
    assert(url.protocol == "https", "Protocol should be https")
    assert(url.port == '8080', "Port should be 8080 with string type")
  end)

  local tast_cases = {
    {
      "http://dev:123456@hello.com:8080/some/path?with=1%23&args=value#hash", false, {
        protocol = 'http',
        auth = "dev:123456",
        hostname = 'hello.com',
        port = '8080',
        pathname = '/some/path',
        query = 'with=1%23&args=value',
        hash = "hash"
      }
    }, {
      "/foo/t.html?qstring#frag", false,
      { pathname = '/foo/t.html', query = 'qstring', hash = 'frag' }
    }, {
      "192.168.0.1:80", true,
      { port = '80', hostname = "192.168.0.1" }
    }
  }

  for _, tast_case in ipairs(tast_cases) do
    it("lhttp_parser url parse:#" .. tast_case[1], function()
      local url, is_connect, expect = tast_case[1], tast_case[2], tast_case[3]
      local result = URL.parse(url, is_connect)
      assert.are.same(expect, result)
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

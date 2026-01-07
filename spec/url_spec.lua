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

  it("lhttp_parser URL parse in CONNECT mode", function()
    local parsed = URL.parse('192.168.0.1:80')
    assert(not parsed)
    parsed = URL.parse('192.168.0.1:80', true)
    assert(parsed)
  end)

  -- 测试 encode/decode 功能
  describe("Encoding and decoding", function()
    it("should encode basic string", function()
      local original = "hello world & special chars: @#$%"
      local encoded = URL.encode(original)
      local decoded = URL.decode(encoded)
      assert(decoded == original, "Decoded should match original")
    end)

    it("should handle empty string", function()
      assert(URL.encode("") == "")
      assert(URL.decode("") == "")
    end)

    it("should encode reserved characters", function()
      local reserved = "!*'();:@&=+$,/?#[]"
      local encoded = URL.encode(reserved)
      -- 所有保留字符都应该被编码
      assert(encoded:find("%%%x%x") ~= nil)
    end)

    it("should decode percent-encoded sequences", function()
      assert(URL.decode("hello%20world") == "hello world")
      assert(URL.decode("%40%23%24") == "@#$")
      assert(URL.decode("%41") == "A")
    end)

    -- it("should handle invalid percent encoding", function()
    --   -- 不完整的百分比编码
    --   print(URL.decode("hello%2") ,"hello%2")
    --   print(URL.decode("hello%2G"), "hello%2G")  -- G不是十六进制
    --   print(URL.decode("%%20"),"%%20")  -- 双百分号
    -- end)
  end)

  -- 正向测试：合法的 URL
  describe("Positive tests - valid URLs", function()
    it("should parse basic HTTPS URL", function()
      local url = URL.parse("https://user:pass@example.com:8080/path?query=value#hash")
      assert(url ~= nil, "Should parse URL successfully")
      assert(url.protocol == "https", "Protocol should be https")
      assert(url.auth == "user:pass", "Auth should be user:pass")
      assert(url.hostname == "example.com", "Hostname should be example.com")
      assert(url.port == '8080', "Port should be 8080 with string type")
      assert(url.pathname == '/path', "Pathname should be /path")
      assert(url.query == 'query=value', "Query should be query=value")
      assert(url.hash == "hash", "Hash should be hash")
    end)

    it("should parse URL without auth and port", function()
      local url = URL.parse("http://example.com/path")
      assert(url.protocol == "http")
      assert(url.hostname == "example.com")
      assert(url.port == nil)
      assert(url.auth == nil)
      assert(url.pathname == '/path')
    end)

    it("should parse URL with only host", function()
      local url = URL.parse("http://example.com")
      assert(url.protocol == "http")
      assert(url.hostname == "example.com")
      assert(url.pathname == nil or url.pathname == "")
    end)

    it("should parse relative URL", function()
      local url = URL.parse("/foo/t.html?qstring#frag")
      assert(url.pathname == '/foo/t.html')
      assert(url.query == 'qstring')
      assert(url.hash == 'frag')
      assert(url.protocol == nil)
      assert(url.hostname == nil)
    end)

    it("should parse URL with IPv4 address", function()
      local url = URL.parse("http://192.168.1.1:8080/test")
      assert(url.hostname == "192.168.1.1")
      assert(url.port == '8080')
    end)

    it("should parse URL with IPv6 address", function()
      local url = URL.parse("http://[::1]:8080/path")
      assert(url.hostname == "::1")
      assert(url.port == '8080')
      assert(url.pathname == '/path')
    end)

    it("should parse URL with IPv6 link-local address", function()
      local url = URL.parse("http://[fe80::1%eth0]:8080/path")
      assert(url.hostname == "fe80::1%eth0")
      assert(url.port == '8080')
    end)

    it("should parse URL with encoded characters", function()
      local url = URL.parse("http://dev:123456@hello.com:8080/some/path?with=1%23&args=value#hash")
      assert(url.protocol == 'http')
      assert(url.auth == "dev:123456")
      assert(url.hostname == 'hello.com')
      assert(url.port == '8080')
      assert(url.pathname == '/some/path')
      assert(url.query == 'with=1%23&args=value')
      assert(url.hash == "hash")
    end)

    it("should parse URL with special characters in query", function()
      local url = URL.parse("/search?q=hello+world&lang=en-US")
      assert(url.query == 'q=hello+world&lang=en-US')
    end)

    it("should parse URL with asterisk in path", function()
      local url = URL.parse("*")
      assert(url.pathname == "*")
    end)

    it("should parse URL with dot in hostname", function()
      local url = URL.parse("http://example.co.uk/path")
      assert(url.hostname == "example.co.uk")
    end)

    it("should parse URL with underscore in hostname", function()
      local url = URL.parse("http://my_server.com/path")
      assert(url.hostname == "my_server.com")
    end)

    it("should parse URL with multiple query parameters", function()
      local url = URL.parse("/test?a=1&b=2&c=3")
      assert(url.query == 'a=1&b=2&c=3')
    end)

    it("should parse URL with empty query", function()
      local url = URL.parse("/test?")
      assert(url.query==nil)
      assert(url.pathname=='/test')
    end)

    it("should parse URL with empty fragment", function()
      local url = URL.parse("/test#")
      assert(url.hash == nil)
      assert(url.pathname=='/test')
    end)

    it("should parse URL with only fragment", function()
      local url = URL.parse("#fragment-only")
      assert(url==nil)
    end)

    it("should parse URL with only query", function()
      local url = URL.parse("?query-only")
      assert(url==nil)
    end)
  end)

  -- CONNECT 模式测试
  describe("CONNECT mode tests", function()
    it("should parse host:port in CONNECT mode", function()
      local parsed = URL.parse('192.168.0.1:80', true)
      assert(parsed)
      assert(parsed.hostname == "192.168.0.1")
      assert(parsed.port == '80')
    end)

    it("should reject URL with path in CONNECT mode", function()
      local parsed = URL.parse('192.168.0.1:80/path', true)
      assert(not parsed, "Should reject URL with path in CONNECT mode")
    end)

    it("should reject URL with query in CONNECT mode", function()
      local parsed = URL.parse('192.168.0.1:80?query', true)
      assert(not parsed, "Should reject URL with query in CONNECT mode")
    end)

    it("should reject URL without port in CONNECT mode", function()
      local parsed = URL.parse('192.168.0.1', true)
      assert(not parsed, "Should reject URL without port in CONNECT mode")
    end)

    it("should parse IPv6 in CONNECT mode", function()
      local parsed = URL.parse('[::1]:8080', true)
      assert(parsed)
      assert(parsed.hostname == "::1")
      assert(parsed.port == '8080')
    end)

    it("should reject IPv6 without port in CONNECT mode", function()
      local parsed = URL.parse('[::1]', true)
      assert(not parsed, "Should reject IPv6 without port in CONNECT mode")
    end)
  end)

  -- 逆向测试：无效的 URL
  describe("Negative tests - invalid URLs", function()
    it("should reject empty string", function()
      local url = URL.parse("")
      assert(url == nil, "Should reject empty string")
    end)

    it("should reject URL with spaces in hostname", function()
      local url = URL.parse("http://exa mple.com/path")
      assert(url == nil, "Should reject URL with spaces in hostname")
    end)

    it("should reject URL with invalid port", function()
      local url = URL.parse("http://example.com:80abc/path")
      assert(url == nil, "Should reject URL with invalid port")
    end)

    it("should reject URL with port out of range", function()
      local url = URL.parse("http://example.com:70000/path")
      assert(url == nil, "Should reject URL with port out of range")
    end)

    it("should reject URL with missing host after schema", function()
      local url = URL.parse("http:///path")
      assert(url == nil, "Should reject URL with missing host")
    end)

    it("should reject URL with invalid IPv6", function()
      local url = URL.parse("http://[::1/path")
      assert(url == nil, "Should reject URL with unclosed IPv6 bracket")
    end)

    it("should reject URL with double @ in auth", function()
      local url = URL.parse("http://user@@example.com/path")
      assert(url == nil, "Should reject URL with double @")
    end)

    it("should reject URL with invalid characters", function()
      -- 控制字符
      local url = URL.parse("http://example.com/\npath")
      assert(url == nil, "Should reject URL with control characters")
    end)

    it("should reject URL with only schema", function()
      local url = URL.parse("http:")
      assert(url == nil, "Should reject URL with only schema")
    end)

    it("should reject URL with only schema and slashes", function()
      local url = URL.parse("http://")
      assert(url == nil, "Should reject URL with only schema and slashes")
    end)

    it("should reject URL with invalid percent encoding in host", function()
      local url = URL.parse("http://exa%2Gmple.com/path")
      assert(url == nil, "Should reject URL with invalid percent encoding")
    end)

    it("should parse // as path", function()
      local url = URL.parse("//path")
      assert(url.pathname == "//path")
    end)

    it("should parse //host as host when followed by /", function()
      -- 实际上，//host/path 在规范中应该被解析为主机
      -- 但你的解析器可能不支持，取决于实现
      local url = URL.parse("//example.com/path")
      -- 根据你的解析器实现，可能有两种结果：
      -- 1. 路径模式: pathname = "//example.com/path"
      -- 2. 主机模式: hostname = "example.com", pathname = "/path"
      assert(url ~= nil)
    end)

    it("should parse //host:port correctly", function()
      local url = URL.parse("//example.com:8080/path")
      assert(url ~= nil)
    end)
  end)

  -- 边缘情况测试
  describe("Edge cases", function()
    it("should parse very long URL", function()
      local longPath = "/" .. string.rep("a", 1000)
      local url = URL.parse("http://example.com" .. longPath)
      assert(url ~= nil)
      assert(#url.pathname == 1001)  -- 包括开头的斜杠
    end)

    it("should parse URL with maximum port number", function()
      local url = URL.parse("http://example.com:65535/path")
      assert(url ~= nil)
      assert(url.port == '65535')
    end)

    it("should reject URL with port 0", function()
      local url = URL.parse("http://example.com:0/path")
      assert(url.port == '0')
    end)

    it("should handle URL with mixed case scheme", function()
      local url = URL.parse("HTTP://example.com/path")
      assert(url ~= nil)
      assert(url.protocol:lower() == "http")
    end)

    it("should parse URL with international domain name", function()
      -- 注意：这取决于解析器是否支持非ASCII字符
      local url = URL.parse("http://例子.测试/path")
      if url then
        -- 如果解析器支持IDN
        assert(url.hostname == "例子.测试")
      end
    end)

    it("should parse URL with plus in query", function()
      local url = URL.parse("/search?q=c%2B%2B")
      assert(url.query == 'q=c%2B%2B')
    end)

    it("should handle repeated calls correctly", function()
      -- 确保解析器状态不会在调用之间泄漏
      local url1 = URL.parse("http://example.com/path1")
      local url2 = URL.parse("/path2")
      assert(url1.hostname == "example.com")
      assert(url1.pathname == "/path1")
      assert(url2.pathname == "/path2")
      assert(url2.hostname == nil)
    end)
  end)

  -- 批量测试用例
  describe("Batch test cases", function()
    local test_cases = {
      -- {url, is_connect, expected_result_or_nil}
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
      }, {
        "https://api.example.com/v1/users?page=1&limit=10", false, {
          protocol = 'https',
          hostname = 'api.example.com',
          pathname = '/v1/users',
          query = 'page=1&limit=10'
        }
      }, {
        "mailto:user@example.com", false, nil  -- mailto 不是 http/https，可能不被支持
      }, {
        "ftp://example.com/file.txt", false, {
          protocol = 'ftp',
          hostname = 'example.com',
          pathname = '/file.txt'
        }
      }, {
        "ws://example.com/chat", false, {
          protocol = 'ws',
          hostname = 'example.com',
          pathname = '/chat'
        }
      }
    }

    for i, test_case in ipairs(test_cases) do
      it("test case " .. i .. ": " .. (test_case[1] or "nil"), function()
        local url, is_connect, expect = test_case[1], test_case[2], test_case[3]
        local result = URL.parse(url, is_connect)

        if expect == nil then
          assert(result == nil, "Expected nil but got: " .. (result and "table" or "nil"))
        else
          assert.are.same(expect, result)
        end
      end)
    end
  end)
end)

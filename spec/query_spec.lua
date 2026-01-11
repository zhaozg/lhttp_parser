
describe('llquery', function()
  local URL = require('lhttp_url')

  it("lhttp_parser URL encoding and decoding", function()
    local original = "hello world & special chars: @#$%"
    local encoded = URL.encode(original)
    -- print("encoded:", encoded)
    local decoded = URL.decode(encoded, "string")
    -- print("decoded:", decoded)
    assert(decoded == original, "Decoded should match original")
  end)

  -- 测试 encode/decode 功能
  describe("Encoding and decoding", function()
    it("should encode basic string", function()
      local original = "hello world & special chars: @#$%"
    local encoded = URL.encode(original)
    -- print("encoded:", encoded)
    local decoded = URL.decode(encoded, "string")
    -- print("decoded:", decoded)
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
  describe("llquery query string parsing", function()
    local URL = require('lhttp_url')

  local inspect = require('inspect')

  it("should parse query string into table", function()
    local tbl = URL.parse_query("a=1&b=2&c=3")
    -- print("tbl:", inspect(tbl))
    assert(type(tbl) == "table")
    assert(tbl.a == "1" and tbl.b == "2" and tbl.c == "3")
  end)

  it("should merge duplicate keys into array", function()
    local tbl = URL.parse_query("k=1&k=2&k=3", true)
    -- print("tbl:", inspect(tbl))
    assert(type(tbl.k) == "table")
    assert(tbl.k[1] == "1" and tbl.k[2] == "2" and tbl.k[3] == "3")
  end)

  it("should handle empty value and key without value", function()
    local tbl = URL.parse_query("foo=&bar")
    -- print("tbl:", inspect(tbl))
    assert(type(tbl) == "table")
    assert(tbl.foo == "" and tbl.bar == "")
  end)

  it("should trim values and lowercase keys if flags enabled", function()
    local tbl = URL.parse_query("A= 1 &B=2 ")
    -- print("tbl:", inspect(tbl))
    assert(type(tbl) == "table")
    assert(tbl.a == "1" and tbl.b == "2")
  end)

  it("should decode percent-encoded and plus", function()
    local tbl = URL.parse_query("city=New+York&lang=zh%2Fcn")
    -- print("tbl:", inspect(tbl))
    assert(type(tbl) == "table")
    assert(tbl.city == "New York" and tbl.lang == "zh/cn")
  end)

  it("should handle non-ascii and unicode", function()
    local encoded = URL.encode("中文 空格")
    local decoded = URL.decode(encoded)
    assert(decoded == "中文 空格")
  end)

  it("should return string if not looks like query", function()
    local s = URL.decode("not_a_query_string")
    assert(s == "not_a_query_string")
  end)

  it("should handle invalid percent encoding gracefully", function()
    local s = URL.decode("foo=%2G")
    -- print("s:", s)
    assert(s == "foo=%2G")
  end)
end)

end)

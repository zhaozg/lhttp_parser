local name
local requests = {
  GET = true, --OK
  POST = true, --OK
  CONNECT = true, --OK
  UPGRADE = true, --OK
  MISC = true, --OK
  SSL = true, --OK
  INVALID = true, --OK
  CHUNK = true,
  COMPLEX = true, --OK
}
local all_methods = {
  "DELETE",
  "GET",
  "HEAD",
  "POST",
  "PUT",
  "CONNECT", -- CONNECT can't be tested like other methods, it's a tunnel
  "OPTIONS",
  "TRACE",
  "COPY",
  "LOCK",
  "MKCOL",
  "MOVE",
  "PROPFIND",
  "PROPPATCH",
  "SEARCH",
  "UNLOCK",
  "BIND",
  "REBIND",
  "UNBIND",
  "ACL",
  "REPORT",
  "MKACTIVITY",
  "CHECKOUT",
  "MERGE",
  "M-SEARCH",
  "NOTIFY",
  "SUBSCRIBE",
  "UNSUBSCRIBE",
  "PATCH",
  "PURGE",
  "MKCALENDAR",
  "LINK",
  "UNLINK",
  "SOURCE",
}

-- GET
if requests.GET then
  requests.get = "GET /test HTTP/1.1\r\n"
      .. "User-Agent: curl/7.18.0 (i486-pc-linux-gnu) libcurl/7.18.0 OpenSSL/0.9.8g zlib/1.2.3.3 libidn/1.1\r\n"
      .. "Host: 0.0.0.0=5000\r\n"
      .. "Accept: */*\r\n"
      .. "\r\n"

  name = "firefox get"
  requests[name] = "GET /favicon.ico HTTP/1.1\r\n"
      .. "Host: 0.0.0.0=5000\r\n"
      .. "User-Agent: Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9) Gecko/2008061015 Firefox/3.0\r\n"
      .. "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
      .. "Accept-Language: en-us,en;q=0.5\r\n"
      .. "Accept-Encoding: gzip,deflate\r\n"
      .. "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\n"
      .. "Keep-Alive: 300\r\n"
      .. "Connection: keep-alive\r\n"
      .. "\r\n"

  name = "dumbluck"
  requests[name] = "GET /dumbluck HTTP/1.1\r\n" .. "aaaaaaaaaaaaa:++++++++++\r\n" .. "\r\n"

  name = "fragment in url"
  requests[name] = "GET /forums/1/topics/2375?page=1#posts-17408 HTTP/1.1\r\n" .. "\r\n"

  name = "get no headers no body"
  requests[name] = "GET /get_no_headers_no_body/world HTTP/1.1\r\n" .. "\r\n"

  name = "get one header no body"
  requests[name] = "GET /get_one_header_no_body HTTP/1.1\r\n" .. "Accept: */*\r\n" .. "\r\n"

  name = "host:port and basic_auth"
  requests[name] = "GET http://a%12:b!&*$@hypnotoad.org:1234/toto HTTP/1.1\r\n" .. "\r\n"

  name = "line folding in header value"
  requests[name] = "GET / HTTP/1.1\n"
      .. "Line1:   abc\n"
      .. "\tdef\n"
      .. " ghi\n"
      .. "\t\tjkl\n"
      .. "  mno \n"
      .. "\t \tqrs\n"
      .. "Line2: \t line2\t\n"
      .. "Line3:\n"
      .. " line3\n"
      .. "Line4: \n"
      .. " \n"
      .. "Connection:\n"
      .. " close\n"
      .. "\n"
end

-- POST
if requests.POST then
  name = "post identity body world"
  requests[name] = "POST /post_identity_body_world?q=search#hey HTTP/1.1\r\n"
      .. "Accept: */*\r\n"
      .. "Content-Length: 5\r\n"
      .. "\r\n"
      .. "World"

  name = "post - chunked body: all your base are belong to us"
  requests[name] = "POST /post_chunked_all_your_base HTTP/1.1\r\n"
      .. "Transfer-Encoding: chunked\r\n"
      .. "\r\n"
      .. "1e\r\nall your base are belong to us\r\n"
      .. "0\r\n"
      .. "\r\n"

  name = "two chunks ; triple zero ending"
  requests[name] = "POST /two_chunks_mult_zero_end HTTP/1.1\r\n"
      .. "Transfer-Encoding: chunked\r\n"
      .. "\r\n"
      .. "5\r\nhello\r\n"
      .. "6\r\n world\r\n"
      .. "000\r\n"
      .. "\r\n"

  name = 'eat CRLF between requests, no "Connection: close" header'
  requests[name] = "POST / HTTP/1.1\r\n"
      .. "Host: www.example.com\r\n"
      .. "Content-Type: application/x-www-form-urlencoded\r\n"
      .. "Content-Length: 4\r\n"
      .. "\r\n"
      .. "q=42\r\n" -- /* note the trailing CRLF */

  name = 'eat CRLF between requests even if "Connection: close" is set'
  requests[name] = "POST / HTTP/1.1\r\n"
      .. "Host: www.example.com\r\n"
      .. "Content-Type: application/x-www-form-urlencoded\r\n"
      .. "Content-Length: 4\r\n"
      .. "Connection: close\r\n"
      .. "\r\n"
      .. "q=42\r\n" -- /* note the trailing CRLF */

  name = "post - multi coding transfer-encoding chunked body"
  requests[name] = "POST / HTTP/1.1\r\n"
      .. "Transfer-Encoding: deflate, chunked\r\n"
      .. "\r\n"
      .. "1e\r\nall your base are belong to us\r\n"
      .. "0\r\n"
      .. "\r\n"

  name = "invalid post - multi line coding transfer-encoding chunked body"
  requests[name] = "POST / HTTP/1.1\r\n"
      .. "Transfer-Encoding: deflate,\r\n"
      .. " chunked\r\n"
      .. "\r\n"
      .. "1e\r\nall your base are belong to us\r\n"
      .. "0\r\n"
      .. "\r\n"

  name = "invalid content length"
  requests[name] = "POST / HTTP/1.1\r\n"
      .. "Content-Length:\r\n" -- empty
      .. "\r\n"

  name = "invalid content length #2"
  requests[name] = "POST / HTTP/1.1\r\n" .. "Content-Length: 4 2\r\n" .. "\r\n"

  name = "invalid content length #3"
  requests[name] = "POST / HTTP/1.1\r\n" .. "Content-Length: 13 37\r\n" .. "\r\n"

  name = "invalid content length #4"
  requests[name] = "POST / HTTP/1.1\r\n" .. "Content-Length:  42\r\n" .. " Hello world!\r\n"

  name = "invalid content length #1"
  requests[name] = "POST / HTTP/1.1\r\n" .. "Content-Length:  42\r\n" .. " \r\n"
end

-- CHUNK
if requests.CHUNK then
  name = "chunked with trailing headers. blech."
  requests[name] = "POST /chunked_w_trailing_headers HTTP/1.1\r\n"
      .. "Transfer-Encoding: chunked\r\n"
      .. "\r\n"
      .. "5\r\nhello\r\n"
      .. "6\r\n world\r\n"
      .. "0\r\n"
      .. "Vary: *\r\n"
      .. "Content-Type: text/plain\r\n"
      .. "\r\n"

  name = "invalid with nonsense after the length"
  requests[name] = "POST /chunked_w_nonsense_after_length HTTP/1.1\r\n"
      .. "Transfer-Encoding: chunked\r\n"
      .. "\r\n"
      .. "5; ilovew3;whattheluck=aretheseparametersfor\r\nhello\r\n"
      .. "6; blahblah; blah\r\n world\r\n"
      .. "0\r\n"
      .. "\r\n"
end

-- CONNECT
if requests.CONNECT then
  name = "invalid connect request"
  requests[name] = "CONNECT 0-home0.netscape.com:443 HTTP/1.0\r\n"
      .. "User-agent: Mozilla/1.1N\r\n"
      .. "Proxy-authorization: basic aGVsbG86d29ybGQ=\r\n"
      .. "\r\n"
      .. "some data\r\n"
      .. "and yet even more data"

  name = "connect caps request"
  requests[name] = "CONNECT HOME0.NETSCAPE.COM:443 HTTP/1.0\r\n"
      .. "User-agent: Mozilla/1.1N\r\n"
      .. "Proxy-authorization: basic aGVsbG86d29ybGQ=\r\n"
      .. "\r\n"

  name = "invalid connect hostname underscore"
  requests[name] = "CONNECT home_0.netscape.com:443 HTTP/1.0\r\n"
      .. "User-agent: Mozilla/1.1N\r\n"
      .. "Proxy-authorization: basic aGVsbG86d29ybGQ=\r\n"
      .. "\r\n"

  name = "invalid connect with body request"
  requests[name] = "CONNECT foo.bar.com:443 HTTP/1.0\r\n"
      .. "User-agent: Mozilla/1.1N\r\n"
      .. "Proxy-authorization: basic aGVsbG86d29ybGQ=\r\n"
      .. "Content-Length: 10\r\n"
      .. "\r\n"
      .. "blarfcicle"
end

-- MISC
if requests.MISC then
  name = "with quotes"
  requests[name] = 'GET /with_"stupid"_quotes?foo="bar" HTTP/1.1\r\n\r\n'

  name = "apachebench get"
  requests[name] = "GET /test HTTP/1.0\r\n"
      .. "Host: 0.0.0.0:5000\r\n"
      .. "User-Agent: ApacheBench/2.3\r\n"
      .. "Accept: */*\r\n\r\n"

  name = "query url with question mark"
  requests[name] = "GET /test.cgi?foo=bar?baz HTTP/1.1\r\n\r\n"

  name = "newline prefix get"
  requests[name] = "\r\nGET /test HTTP/1.1\r\n\r\n"

  name = "request with no http version"
  requests[name] = "GET /\r\n" .. "\r\n"

  name = "line folding in header value"
  requests[name] = "GET / HTTP/1.1\r\n"
      .. "Line1:   abc\r\n"
      .. "\tdef\r\n"
      .. " ghi\r\n"
      .. "\t\tjkl\r\n"
      .. "  mno \r\n"
      .. "\t \tqrs\r\n"
      .. "Line2: \t line2\t\r\n"
      .. "Line3:\r\n"
      .. " line3\r\n"
      .. "Line4: \r\n"
      .. " \r\n"
      .. "Connection:\r\n"
      .. " close\r\n"
      .. "\r\n"

  name = "host terminated by a query string"
  requests[name] = "GET http://hypnotoad.org?hail=all HTTP/1.1\r\n" .. "\r\n"

  name = "host:port terminated by a query string"
  requests[name] = "GET http://hypnotoad.org:1234?hail=all HTTP/1.1\r\n" .. "\r\n"

  name = "host:port terminated by a space"
  requests[name] = "GET http://hypnotoad.org:1234 HTTP/1.1\r\n" .. "\r\n"

  name = "report request"
  requests[name] = "REPORT /test HTTP/1.1\r\n" .. "\r\n"

  name = "m-search request"
  requests[name] = "M-SEARCH * HTTP/1.1\r\n"
      .. "HOST: 239.255.255.250:1900\r\n"
      .. 'MAN: "ssdp:discover"\r\n'
      .. 'ST: "ssdp:all"\r\n'
      .. "\r\n"

  name = "PATCH request"
  requests[name] = "PATCH /file.txt HTTP/1.1\r\n"
      .. "Host: www.example.com\r\n"
      .. "Content-Type: application/example\r\n"
      .. 'If-Match: "e0023aa4e"\r\n'
      .. "Content-Length: 10\r\n"
      .. "\r\n"
      .. "cccccccccc"

  name = "PURGE request"
  requests[name] = "PURGE /file.txt HTTP/1.1\r\n" .. "Host: www.example.com\r\n" .. "\r\n"

  name = "SEARCH request"
  requests[name] = "SEARCH / HTTP/1.1\r\n" .. "Host: www.example.com\r\n" .. "\r\n"

  name = "link request"
  requests[name] = "LINK /images/my_dog.jpg HTTP/1.1\r\n"
      .. "Host: example.com\r\n"
      .. 'Link: <http://example.com/profiles/joe>; rel="tag"\r\n'
      .. 'Link: <http://example.com/profiles/sally>; rel="tag"\r\n'
      .. "\r\n"

  name = "unlink request"
  requests[name] = "UNLINK /images/my_dog.jpg HTTP/1.1\r\n"
      .. "Host: example.com\r\n"
      .. 'Link: <http://example.com/profiles/sally>; rel="tag"\r\n'
      .. "\r\n"

  name = "source request"
  requests[name] = "SOURCE /music/sweet/music HTTP/1.1\r\n" .. "Host: example.com\r\n" .. "\r\n"

  name = "source request"
  requests[name] = "SOURCE /music/sweet/music ICE/1.0\r\n" .. "Host: example.com\r\n" .. "\r\n"

  name = "invalid corrupted_connection"
  requests[name] = "GET / HTTP/1.1\r\n"
      .. "Host: www.example.com\r\n"
      .. "Connection\r\027\053\213eep-Alive\r\n"
      .. "Accept-Encoding: gzip\r\n"
      .. "\r\n"

  name = "invalid corrupted_header_name"
  requests[name] = "GET / HTTP/1.1\r\n"
      .. "Host: www.example.com\r\n"
      .. "X-Some-Header\r\027\053\213eep-Alive\r\n"
      .. "Accept-Encoding: gzip\r\n"
      .. "\r\n"
end

-- COMPLEX

if requests.COMPLEX then
  name = "content length surrounding whitespace"
  requests[name] = "POST / HTTP/1.1\r\n"
      .. "Content-Length:  2 \r\n" -- Note the surrounding whitespace.
      .. "\r\n  "

  name = "invalid utf-8 path request"
  requests[name] = "GET /δ¶/δt/pope?q=1#narf HTTP/1.1\r\n" .. "Host: github.com\r\n" .. "\r\n"
end

-- UPGRADE
if requests.UPGRADE then
  name = "upgrade request"
  requests[name] = "GET /demo HTTP/1.1\r\n"
      .. "Host: example.com\r\n"
      .. "Connection: Upgrade\r\n"
      .. "Sec-WebSocket-Key2: 12998 5 Y3 1  .P00\r\n"
      .. "Sec-WebSocket-Protocol: sample\r\n"
      .. "Upgrade: WebSocket\r\n"
      .. "Sec-WebSocket-Key1: 4 @1  46546xW%0l 1 5\r\n"
      .. "Origin: http://example.com\r\n"
      .. "\r\n"

  name = "multiple connection header values with folding"
  requests[name] = "GET /demo HTTP/1.1\r\n"
      .. "Host: example.com\r\n"
      .. "Connection: Something,\r\n"
      .. " Upgrade, ,Keep-Alive\r\n"
      .. "Sec-WebSocket-Key2: 12998 5 Y3 1  .P00\r\n"
      .. "Sec-WebSocket-Protocol: sample\r\n"
      .. "Upgrade: WebSocket\r\n"
      .. "Sec-WebSocket-Key1: 4 @1  46546xW%0l 1 5\r\n"
      .. "Origin: http://example.com\r\n"
      .. "\r\n"

  name = "multiple connection header values with folding and lws"
  requests[name] = "GET /demo HTTP/1.1\r\n"
      .. "Connection: keep-alive, upgrade\r\n"
      .. "Upgrade: WebSocket\r\n"
      .. "\r\n"

  name = "upgrade post request"
  requests[name] = "POST /demo HTTP/1.1\r\n"
      .. "Host: example.com\r\n"
      .. "Connection: Upgrade\r\n"
      .. "Upgrade: HTTP/2.0\r\n"
      .. "Content-Length: 15\r\n"
      .. "\r\n"
      .. "sweet post body"
end

if requests.INVALID then
  name = "HPE_INVALID_CONSTANT #1"
  requests[name] = "GET / IHTTP/1.0\r\n\r\n"

  name = "HPE_INVALID_CONSTANT #2"
  requests[name] = "GET / ICE/1.0\r\n\r\n"

  name = "HPE_INVALID_VERSION #1"
  requests[name] = "GET / HTP/1.1\r\n\r\n"

  name = "HPE_INVALID_VERSION #2"
  requests[name] = "GET / HTTP/01.1\r\n\r\n"

  name = "HPE_INVALID_VERSION #3"
  requests[name] = "GET / HTTP/11.1\r\n\r\n"

  name = "HPE_INVALID_VERSION #4"
  requests[name] = "GET / HTTP/1.01\r\n\r\n"

  name = "HPE_INVALID_TOKEN #1"
  requests[name] = "GET / HTTP/1.0\r\nHello: w\1rld\r\n\r\n"

  name = "HPE_INVALID_TOKEN #1"
  requests[name] = "GET / HTTP/1.0\r\nHello: woooo\2rld\r\n\r\n"
end

if requests.SSL then
  name = "SSL"
  requests[name] = "GET / HTTP/1.1\r\n"
      .. "X-SSL-Nonsense:   -----BEGIN CERTIFICATE-----\r\n"
      .. "\tMIIFbTCCBFWgAwIBAgICH4cwDQYJKoZIhvcNAQEFBQAwcDELMAkGA1UEBhMCVUsx\r\n"
      .. "\tETAPBgNVBAoTCGVTY2llbmNlMRIwEAYDVQQLEwlBdXRob3JpdHkxCzAJBgNVBAMT\r\n"
      .. "\tAkNBMS0wKwYJKoZIhvcNAQkBFh5jYS1vcGVyYXRvckBncmlkLXN1cHBvcnQuYWMu\r\n"
      .. "\tdWswHhcNMDYwNzI3MTQxMzI4WhcNMDcwNzI3MTQxMzI4WjBbMQswCQYDVQQGEwJV\r\n"
      .. "\tSzERMA8GA1UEChMIZVNjaWVuY2UxEzARBgNVBAsTCk1hbmNoZXN0ZXIxCzAJBgNV\r\n"
      .. "\tBAcTmrsogriqMWLAk1DMRcwFQYDVQQDEw5taWNoYWVsIHBhcmQYJKoZIhvcNAQEB\r\n"
      .. "\tBQADggEPADCCAQoCggEBANPEQBgl1IaKdSS1TbhF3hEXSl72G9J+WC/1R64fAcEF\r\n"
      .. "\tW51rEyFYiIeZGx/BVzwXbeBoNUK41OK65sxGuflMo5gLflbwJtHBRIEKAfVVp3YR\r\n"
      .. "\tgW7cMA/s/XKgL1GEC7rQw8lIZT8RApukCGqOVHSi/F1SiFlPDxuDfmdiNzL31+sL\r\n"
      .. "\t0iwHDdNkGjy5pyBSB8Y79dsSJtCW/iaLB0/n8Sj7HgvvZJ7x0fr+RQjYOUUfrePP\r\n"
      .. "\tu2MSpFyf+9BbC/aXgaZuiCvSR+8Snv3xApQY+fULK/xY8h8Ua51iXoQ5jrgu2SqR\r\n"
      .. "\twgA7BUi3G8LFzMBl8FRCDYGUDy7M6QaHXx1ZWIPWNKsCAwEAAaOCAiQwggIgMAwG\r\n"
      .. "\tA1UdEwEB/wQCMAAwEQYJYIZIAYb4QgHTTPAQDAgWgMA4GA1UdDwEB/wQEAwID6DAs\r\n"
      .. "\tBglghkgBhvhCAQ0EHxYdVUsgZS1TY2llbmNlIFVzZXIgQ2VydGlmaWNhdGUwHQYD\r\n"
      .. "\tVR0OBBYEFDTt/sf9PeMaZDHkUIldrDYMNTBZMIGaBgNVHSMEgZIwgY+AFAI4qxGj\r\n"
      .. "\tloCLDdMVKwiljjDastqooXSkcjBwMQswCQYDVQQGEwJVSzERMA8GA1UEChMIZVNj\r\n"
      .. "\taWVuY2UxEjAQBgNVBAsTCUF1dGhvcml0eTELMAkGA1UEAxMCQ0ExLTArBgkqhkiG\r\n"
      .. "\t9w0BCQEWHmNhLW9wZXJhdG9yQGdyaWQtc3VwcG9ydC5hYy51a4IBADApBgNVHRIE\r\n"
      .. "\tIjAggR5jYS1vcGVyYXRvckBncmlkLXN1cHBvcnQuYWMudWswGQYDVR0gBBIwEDAO\r\n"
      .. "\tBgwrBgEEAdkvAQEBAQYwPQYJYIZIAYb4QgEEBDAWLmh0dHA6Ly9jYS5ncmlkLXN1\r\n"
      .. "\tcHBvcnQuYWMudmT4sopwqlBWsvcHViL2NybC9jYWNybC5jcmwwPQYJYIZIAYb4QgEDBDAWLmh0\r\n"
      .. "\tdHA6Ly9jYS5ncmlkLXN1cHBvcnQuYWMudWsvcHViL2NybC9jYWNybC5jcmwwPwYD\r\n"
      .. "\tVR0fBDgwNjA0oDKgMIYuaHR0cDovL2NhLmdyaWQt5hYy51ay9wdWIv\r\n"
      .. "\tY3JsL2NhY3JsLmNybDANBgkqhkiG9w0BAQUFAAOCAQEAS/U4iiooBENGW/Hwmmd3\r\n"
      .. "\tXCy6Zrt08YjKCzGNjorT98g8uGsqYjSxv/hmi0qlnlHs+k/3Iobc3LjS5AMYr5L8\r\n"
      .. "\tUO7OSkgFFlLHQyC9JzPfmLCAugvzEbyv4Olnsr8hbxF1MbKZoQxUZtMVu29wjfXk\r\n"
      .. "\thTeApBv7eaKCWpSp7MCbvgzm74izKhu3vlDk9w6qVrxePfGgpKPqfHiOoGhFnbTK\r\n"
      .. "\twTC6o2xq5y0qZ03JonF7OJspEd3I5zKY3E+ov7/ZhW6DqT8UFvsAdjvQbXyhV8Eu\r\n"
      .. "\tYhixw1aKEPzNjNowuIseVogKOLXxWI5vAi5HgXdS0/ES5gDGsABo4fqovUKlgop3\r\n"
      .. "\tRA==\r\n"
      .. "\t-----END CERTIFICATE-----\r\n"
      .. "\r\n"
end

local function init_parser(connect)
  local lhp = require('lhttp_parser')
  local URL = require('lhttp_url')
  local reqs = {}
  local cur = nil
  local cb = {}

  local function parse_path_query_fragment(uri)
    local url = URL.parse(uri, connect)
    if url then
      return url.path, url.query, url.fragment
    end
  end

  function cb.onMessageBegin()
    assert(cur == nil)
    cur = { headers = {} }
  end

  function cb.onUrl(value)
    cur.url = cur.url and (cur.url .. value) or value
    cur.path, cur.query_string, cur.fragment = parse_path_query_fragment(cur.url)
  end

  function cb.onBody(value)
    if nil == cur.body then
      cur.body = {}
    end
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
    assert(nil ~= cur)
    table.insert(reqs, cur)
    cur = nil
  end

  function cb.onHeadersComplete(info)
    assert(cur.info == nil)
    cur.info = info
  end

  return lhp.new("request", cb), reqs
end

return require("lib/tap")(function(test)
  for k, v in pairs(requests) do
    if type(v) ~= "boolean" then
      test(k, function(p, p, expect, uv)
        local connect = k:match("connect ") ~= nil
        local parser, reqs = init_parser(connect)
        local bytes_read = parser:execute(v)
        if not k:match("^HPE_INVALID") and not k:match("^invalid") and not k:match("underscore") then
          assert(
            bytes_read == #v,
            "only [" .. tostring(bytes_read) .. "] bytes read, expected [" .. tostring(#v) .. "] in " .. k
          )
          local got = reqs[#reqs]
          local found = false
          for i = 1, #all_methods do
            found = found or all_methods[i] == got.info.method
          end
          assert(found, got.info.method)
        end
      end)
    end
  end


  local bad_methods = {
    "ASDF",
    "C******",
    "COLA",
    "GEM",
    "GETA",
    "M****",
    "MKCOLA",
    "PROPPATCHA",
    "PUN",
    "PX",
    "SA",
    "hello world",
  }

  for i = 1, #all_methods do
    local k = all_methods[i]
    test(k, function(p, p, expect, uv)
      local req = string.format("%s / HTTP/1.1\r\n\r\n", k)
      local connect = k:match("CONNECT") ~= nil
      local parser = init_parser(connect)
      local bytes_read = parser:execute(req)
      assert(bytes_read == #req)
    end)
  end

  for i = 1, #bad_methods do
    local k = bad_methods[i]
    test(k, function(p, p, expect, uv)
      local req = string.format("%s / HTTP/1.1\r\n\r\n", k)
      local connect = k:match("CONNECT") ~= nil
      local parser = init_parser(connect)
      local bytes_read = parser:execute(req)
      assert(bytes_read ~= #req, bytes_read)
    end)
  end
end)

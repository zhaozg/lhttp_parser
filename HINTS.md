# HTTP Security Best Practices and Exception Handling Guide

This document provides guidance on how to handle malicious, malformed, and exceptional HTTP requests when using `lhttp_parser` in production HTTP web services.

## Table of Contents
1. [Overview](#overview)
2. [Common Attack Patterns](#common-attack-patterns)
3. [Parser Error Handling](#parser-error-handling)
4. [Resource Limits](#resource-limits)
5. [Input Validation](#input-validation)
6. [Security Checklist](#security-checklist)

## Overview

HTTP servers are constantly exposed to various types of malicious or malformed requests. Proper handling of these requests is crucial for:
- **Security**: Prevent exploitation and attacks
- **Stability**: Avoid crashes and resource exhaustion
- **Compliance**: Meet security standards and best practices

## Common Attack Patterns

### 1. Request Smuggling
**Attack**: Manipulating Content-Length and Transfer-Encoding headers to smuggle requests.

**Protection**:
```lua
local parser = lhp.new('request', {
  onHeadersComplete = function(info)
    -- Reject requests with both Content-Length and Transfer-Encoding
    if has_content_length and has_transfer_encoding then
      return -1  -- Reject the request
    end
  end
})
```

### 2. Large Header Attacks
**Attack**: Sending extremely large headers to exhaust memory or trigger buffer overflows.

**Protection**:
```lua
local MAX_HEADER_SIZE = 8192  -- 8KB limit
local current_header_size = 0

local callbacks = {
  onHeaderField = function(field)
    current_header_size = current_header_size + #field
    if current_header_size > MAX_HEADER_SIZE then
      error("Header too large")
    end
  end,
  onHeaderValue = function(value)
    current_header_size = current_header_size + #value
    if current_header_size > MAX_HEADER_SIZE then
      error("Header too large")
    end
  end
}
```

### 3. Slowloris Attack
**Attack**: Sending partial requests very slowly to keep connections open and exhaust server resources.

**Protection**:
- Implement connection timeouts at the network layer
- Set maximum request processing time
- Limit the number of concurrent connections per IP

```lua
-- Example: Timeout handling (requires additional infrastructure)
local start_time = os.time()
local MAX_REQUEST_TIME = 30  -- 30 seconds

local function check_timeout()
  if os.time() - start_time > MAX_REQUEST_TIME then
    error("Request timeout")
  end
end
```

### 4. HTTP Method Overflow
**Attack**: Using unusual or invalid HTTP methods.

**Protection**:
```lua
local ALLOWED_METHODS = {
  GET = true,
  POST = true,
  PUT = true,
  DELETE = true,
  HEAD = true,
  OPTIONS = true
}

callbacks.onHeadersComplete = function(info)
  local method = info.method
  if not ALLOWED_METHODS[method] then
    return -1  -- Reject non-standard methods
  end
end
```

### 5. Path Traversal
**Attack**: Using `../` sequences to access files outside the intended directory.

**Protection**:
```lua
callbacks.onUrl = function(url)
  if url:match("%.%.") or url:match("//") then
    error("Suspicious URL pattern detected")
  end
end
```

## Parser Error Handling

### Error Codes
`lhttp_parser` returns specific error codes when parsing fails. Always check these codes:

```lua
local parser = lhp.new('request', callbacks)
local bytes, err = parser:execute(data)

if err ~= "HPE_OK" then
  if err == "HPE_INVALID_METHOD" then
    -- Log and reject: Invalid HTTP method
    log_security_event("Invalid method attempted")
    close_connection()
  elseif err == "HPE_INVALID_URL" then
    -- Log and reject: Malformed URL
    log_security_event("Malformed URL")
    close_connection()
  elseif err == "HPE_INVALID_HEADER_TOKEN" then
    -- Log and reject: Invalid header
    log_security_event("Invalid header token")
    close_connection()
  else
    -- Handle other errors
    log_error("Parse error: " .. err)
    close_connection()
  end
  return
end
```

### Graceful Degradation
When parser errors occur:

1. **Log the incident**: Record IP, timestamp, and error details
2. **Close the connection**: Don't try to recover from parse errors
3. **Rate limit**: Track repeated errors from the same IP
4. **Alert**: Generate alerts for repeated attacks

```lua
local error_counts = {}  -- IP -> error count

local function handle_parse_error(client_ip, err)
  -- Log the error
  syslog("Parse error from " .. client_ip .. ": " .. err)
  
  -- Track errors per IP
  error_counts[client_ip] = (error_counts[client_ip] or 0) + 1
  
  -- Ban IPs with too many errors
  if error_counts[client_ip] > 10 then
    ban_ip(client_ip)
    alert("Possible attack from " .. client_ip)
  end
end
```

## Resource Limits

### Memory Limits

```lua
local MAX_BODY_SIZE = 10 * 1024 * 1024  -- 10MB
local body_size = 0
local body_parts = {}

callbacks.onBody = function(chunk)
  if chunk then
    body_size = body_size + #chunk
    if body_size > MAX_BODY_SIZE then
      error("Request body too large")
    end
    table.insert(body_parts, chunk)
  end
end

callbacks.onHeadersComplete = function(info)
  -- Check Content-Length before accepting the body
  if info.content_length and info.content_length > MAX_BODY_SIZE then
    return -1  -- Reject immediately
  end
end
```

### Connection Limits

```lua
local MAX_REQUESTS_PER_CONNECTION = 1000
local request_count = 0

callbacks.onMessageComplete = function()
  request_count = request_count + 1
  
  if request_count >= MAX_REQUESTS_PER_CONNECTION then
    -- Force connection close after max requests
    close_connection()
  end
end
```

### Header Count Limits

```lua
local MAX_HEADERS = 100
local header_count = 0

callbacks.onHeaderField = function(field)
  header_count = header_count + 1
  if header_count > MAX_HEADERS then
    error("Too many headers")
  end
end
```

## Input Validation

### URL Validation

```lua
local function validate_url(url)
  -- Check URL length
  if #url > 2048 then
    return false, "URL too long"
  end
  
  -- Check for null bytes
  if url:find("\0") then
    return false, "Invalid character in URL"
  end
  
  -- Check for valid characters
  if not url:match("^[%w%-%._~:/?#%[%]@!$&'()*+,;=%%]+$") then
    return false, "Invalid URL characters"
  end
  
  return true
end

callbacks.onUrl = function(url)
  local ok, err = validate_url(url)
  if not ok then
    error(err)
  end
end
```

### Header Validation

```lua
local function validate_header(field, value)
  -- Check for null bytes
  if field:find("\0") or value:find("\0") then
    return false, "Null byte in header"
  end
  
  -- Check for CR/LF injection
  if field:find("[\r\n]") or value:find("[\r\n]") then
    return false, "CRLF injection attempt"
  end
  
  -- Validate header name (alphanumeric and hyphens only)
  if not field:match("^[%w-]+$") then
    return false, "Invalid header name"
  end
  
  return true
end

callbacks.onHeaderField = function(field)
  local ok, err = validate_header(field, "")
  if not ok then
    error(err)
  end
end
```

## Security Checklist

Use this checklist when deploying an HTTP service with `lhttp_parser`:

### Basic Security
- [ ] Implement request size limits (headers, body, URL)
- [ ] Validate all HTTP methods against an allowlist
- [ ] Check for path traversal attempts in URLs
- [ ] Sanitize all input before logging to prevent log injection
- [ ] Return generic error messages to clients (don't expose internals)

### Parser Configuration
- [ ] Always check return values from `parser:execute()`
- [ ] Handle all possible error codes appropriately
- [ ] Don't attempt to continue parsing after errors
- [ ] Reset or recreate parser between requests
- [ ] Implement timeouts for incomplete requests

### Resource Protection
- [ ] Set maximum header size (recommend 8KB)
- [ ] Set maximum body size based on your use case
- [ ] Limit number of headers per request (recommend 100)
- [ ] Limit requests per connection (recommend 1000)
- [ ] Implement connection timeouts (recommend 30s for request, 5s for headers)

### Monitoring & Logging
- [ ] Log all parser errors with client IP and timestamp
- [ ] Monitor error rates per IP address
- [ ] Alert on suspicious patterns (repeated errors, unusual methods)
- [ ] Implement rate limiting based on error counts
- [ ] Keep access logs for forensics and analysis

### Defense in Depth
- [ ] Use a reverse proxy (nginx, HAProxy) in front of your application
- [ ] Implement IP-based rate limiting
- [ ] Use fail2ban or similar for automatic blocking
- [ ] Keep lhttp_parser and dependencies updated
- [ ] Regular security audits and penetration testing

## Testing Your Implementation

Always test your HTTP service with various attack patterns:

```bash
# Test with invalid method
echo -e "INVALID /path HTTP/1.1\r\nHost: test\r\n\r\n" | nc localhost 8080

# Test with oversized header
python -c "print('GET / HTTP/1.1\r\nHost: test\r\n' + 'X-Large: ' + 'A'*100000 + '\r\n\r\n')" | nc localhost 8080

# Test with path traversal
curl "http://localhost:8080/../../../etc/passwd"

# Test with CR/LF injection
curl "http://localhost:8080/" -H "X-Header: test\r\nX-Injected: malicious"

# Test with slow request (Slowloris simulation)
echo -e "GET / HTTP/1.1\r\nHost: test\r\n" | nc localhost 8080 && sleep 60
```

## Additional Resources

- [OWASP HTTP Security Headers](https://owasp.org/www-project-secure-headers/)
- [HTTP Request Smuggling](https://portswigger.net/web-security/request-smuggling)
- [CWE-400: Uncontrolled Resource Consumption](https://cwe.mitre.org/data/definitions/400.html)
- [RFC 9110: HTTP Semantics](https://www.rfc-editor.org/rfc/rfc9110.html)

## Contributing

If you discover new attack patterns or have suggestions for improving these security practices, please open an issue or pull request in the repository.

## License

This document is provided as-is for educational and reference purposes. Always consult with security professionals for production deployments.

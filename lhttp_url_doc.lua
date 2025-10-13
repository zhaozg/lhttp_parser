--- URL utilities for Lua
-- This module provides URL encoding, decoding, and parsing functions.
--
-- @module lhttp_url
-- @author zhaozg
-- @license Apache License 2.0
-- @copyright 2012 The Luvit Authors

local lhttp_url = {}

--- Encode a string for use in URLs
-- Encodes special characters in a string to make it safe for use in URLs.
-- Spaces are encoded as '+' and other special characters are percent-encoded.
--
-- @function encode
-- @tparam string str String to encode
-- @treturn string URL-encoded string
-- @usage
-- local lurl = require('lhttp_url')
-- local encoded = lurl.encode("hello world!")
-- -- Returns: "hello+world%21"
function lhttp_url.encode(str)
end

--- Decode a URL-encoded string
-- Decodes a URL-encoded string back to its original form.
-- Handles both '+' (as space) and percent-encoded characters.
--
-- @function decode
-- @tparam string str URL-encoded string to decode
-- @treturn string Decoded string
-- @usage
-- local lurl = require('lhttp_url')
-- local decoded = lurl.decode("hello+world%21")
-- -- Returns: "hello world!"
function lhttp_url.decode(str)
end

--- Parse a URL into components
-- Parses a URL string and returns a table with its components.
--
-- @function parse
-- @tparam string url URL to parse
-- @tparam[opt=false] boolean is_connect Whether this is a CONNECT request URL
-- @treturn[1] table URL components table
-- @treturn[2] nil If parsing failed
-- @usage
-- local lurl = require('lhttp_url')
-- local url = lurl.parse("https://user:pass@example.com:8080/path?query=value#hash")
-- -- Returns a table with:
-- -- {
-- --   protocol = "https",
-- --   auth = "user:pass",
-- --   host = "example.com",
-- --   port = 8080,
-- --   port_string = "8080",
-- --   path = "/path",
-- --   query = "query=value",
-- --   hash = "hash"
-- -- }
--
-- @return table URL components:
--
-- * `protocol` - URL scheme (e.g., "http", "https")
-- * `auth` - Authentication info (username:password)
-- * `host` - Hostname or IP address
-- * `port` - Port number (as number)
-- * `port_string` - Port number (as string)
-- * `path` - Path component
-- * `query` - Query string (without the '?')
-- * `hash` - Fragment identifier (without the '#')
function lhttp_url.parse(url, is_connect)
end

return lhttp_url

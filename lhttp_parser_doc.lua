--- HTTP Parser for Lua
-- This module provides Lua bindings for llhttp, a fast HTTP parser library.
-- It supports parsing both HTTP requests and responses with callbacks for
-- different parsing events.
--
-- @module lhttp_parser
-- @author zhaozg
-- @license Apache License 2.0
-- @copyright 2012 The Luvit Authors

local lhttp_parser = {}

--- Error codes
-- @section errors

--- No error occurred
lhttp_parser.HPE_OK = 0

--- Internal error
lhttp_parser.HPE_INTERNAL = 1

--- Strict mode error
lhttp_parser.HPE_STRICT = 2

--- CR expected
lhttp_parser.HPE_CR_EXPECTED = 3

--- LF expected
lhttp_parser.HPE_LF_EXPECTED = 4

--- Unexpected content length
lhttp_parser.HPE_UNEXPECTED_CONTENT_LENGTH = 5

--- Unexpected space
lhttp_parser.HPE_UNEXPECTED_SPACE = 6

--- Closed connection
lhttp_parser.HPE_CLOSED_CONNECTION = 7

--- Invalid method
lhttp_parser.HPE_INVALID_METHOD = 8

--- Invalid URL
lhttp_parser.HPE_INVALID_URL = 9

--- Invalid constant
lhttp_parser.HPE_INVALID_CONSTANT = 10

--- Invalid version
lhttp_parser.HPE_INVALID_VERSION = 11

--- Invalid header token
lhttp_parser.HPE_INVALID_HEADER_TOKEN = 12

--- Invalid content length
lhttp_parser.HPE_INVALID_CONTENT_LENGTH = 13

--- Invalid chunk size
lhttp_parser.HPE_INVALID_CHUNK_SIZE = 14

--- Invalid status
lhttp_parser.HPE_INVALID_STATUS = 15

--- Invalid EOF state
lhttp_parser.HPE_INVALID_EOF_STATE = 16

--- Invalid transfer encoding
lhttp_parser.HPE_INVALID_TRANSFER_ENCODING = 17

--- Parser was paused
lhttp_parser.HPE_PAUSED = 18

--- Parser was paused for upgrade
lhttp_parser.HPE_PAUSED_UPGRADE = 19

--- User error
lhttp_parser.HPE_USER = 20

--- Version information
-- @section version

--- Major version number
lhttp_parser.VERSION_MAJOR = 9

--- Minor version number
lhttp_parser.VERSION_MINOR = 3

--- Patch version number
lhttp_parser.VERSION_PATCH = 0

--- Indicates this is llhttp (not http-parser)
lhttp_parser.llhttp = true

--- Create a new HTTP parser
-- Creates a new parser instance for parsing HTTP requests or responses.
-- The parser uses callbacks to notify about parsing events.
--
-- @function new
-- @tparam string parser_type Type of parser: 'request', 'response', or 'both'
-- @tparam table callbacks Table containing callback functions
-- @return parser New parser object
-- @usage
-- local lhp = require('lhttp_parser')
-- local parser = lhp.new('request', {
--   onMessageBegin = function() end,
--   onUrl = function(url) end,
--   onHeaderField = function(field) end,
--   onHeaderValue = function(value) end,
--   onHeadersComplete = function(info) end,
--   onBody = function(chunk) end,
--   onMessageComplete = function() end
-- })
function lhttp_parser.new(parser_type, callbacks)
end

--- Parser object methods
-- @section parser

--- Execute the parser on input data
-- Feeds data to the parser for parsing. The parser will invoke callbacks
-- as it encounters different parts of the HTTP message.
--
-- @function parser:execute
-- @tparam string data Input data to parse
-- @tparam[opt=1] number i Starting position (1-based index)
-- @tparam[opt=-1] number j Ending position (negative values count from end)
-- @return number Number of bytes parsed
-- @return string Error code name (e.g., "HPE_OK")
-- @usage
-- local nparsed, err = parser:execute(data)
-- if err ~= "HPE_OK" then
--   print("Parse error:", err)
-- end
function lhttp_parser.execute(data, i, j)
end

--- Finish parsing
-- Signals the end of input to the parser. This should be called after
-- all data has been fed to the parser.
--
-- @function parser:finish
-- @return[1] number 0 on success
-- @return[2] nil On error
-- @return[2] string Error code name
-- @usage
-- local result, err = parser:finish()
-- if not result then
--   print("Finish error:", err)
-- end
function lhttp_parser.finish()
end

--- Reset the parser
-- Resets the parser state so it can be reused for parsing a new message.
--
-- @function parser:reset
-- @usage
-- parser:reset()
function lhttp_parser.reset()
end

--- Get HTTP version
-- Returns the HTTP version from the parsed message.
--
-- @function parser:http_version
-- @return number Major version
-- @return number Minor version
-- @usage
-- local major, minor = parser:http_version()
-- print("HTTP version:", major .. "." .. minor)
function lhttp_parser.http_version()
end

--- Get status code
-- Returns the HTTP status code. Only valid for response parsers.
--
-- @function parser:status_code
-- @return number HTTP status code
-- @usage
-- local code = parser:status_code()
-- print("Status code:", code)
function lhttp_parser.status_code()
end

--- Get HTTP method
-- Returns the HTTP method name. Only valid for request parsers.
--
-- @function parser:method
-- @return string Method name (e.g., "GET", "POST", "PUT")
-- @usage
-- local method = parser:method()
-- print("Method:", method)
function lhttp_parser.method()
end

--- Get error information
-- Returns detailed error information from the parser.
--
-- @function parser:http_errno
-- @return number Error number
-- @return string Error name
-- @return string Error description
-- @usage
-- local errno, name, desc = parser:http_errno()
-- if errno ~= 0 then
--   print("Error:", name, desc)
-- end
function lhttp_parser.http_errno()
end

--- Check if upgrade is requested
-- Returns whether the parsed message requests a protocol upgrade.
--
-- @function parser:upgrade
-- @return boolean true if upgrade is requested
-- @usage
-- if parser:upgrade() then
--   print("Protocol upgrade requested")
-- end
function lhttp_parser.upgrade()
end

--- Check if connection should be kept alive
-- Returns whether the connection should be kept alive for reuse.
--
-- @function parser:should_keep_alive
-- @return boolean true if connection should be kept alive
-- @usage
-- if parser:should_keep_alive() then
--   -- Reuse connection
-- end
function lhttp_parser.should_keep_alive()
end

--- Pause the parser
-- Pauses parsing. The parser can be resumed with parser:resume().
--
-- @function parser:pause
-- @return parser The parser object (for chaining)
-- @usage
-- parser:pause()
function lhttp_parser.pause()
end

--- Resume the parser
-- Resumes a paused parser.
--
-- @function parser:resume
-- @return parser The parser object (for chaining)
-- @usage
-- parser:resume()
function lhttp_parser.resume()
end

--- Resume after upgrade
-- Resumes the parser after a protocol upgrade.
--
-- @function parser:resume_after_upgrade
-- @return parser The parser object (for chaining)
-- @usage
-- parser:resume_after_upgrade()
function lhttp_parser.resume_after_upgrade()
end

--- Callbacks
-- @section callbacks

--- Called when message parsing begins
-- @callback onMessageBegin
-- @usage
-- callbacks.onMessageBegin = function()
--   print("Starting to parse a new message")
-- end

--- Called when URL is parsed (request only)
-- @callback onUrl
-- @tparam string url URL or URL fragment
-- @usage
-- callbacks.onUrl = function(url)
--   print("URL:", url)
-- end

--- Called when status is parsed (response only)
-- @callback onStatus
-- @tparam number code Status code
-- @tparam string text Status text
-- @usage
-- callbacks.onStatus = function(code, text)
--   print("Status:", code, text)
-- end

--- Called when header field is parsed
-- @callback onHeaderField
-- @tparam string field Header field name (may be called multiple times for same header)
-- @usage
-- callbacks.onHeaderField = function(field)
--   current_field = field
-- end

--- Called when header value is parsed
-- @callback onHeaderValue
-- @tparam string value Header value (may be called multiple times for same header)
-- @usage
-- callbacks.onHeaderValue = function(value)
--   headers[current_field] = value
-- end

--- Called when all headers are parsed
-- @callback onHeadersComplete
-- @tparam table info Information table containing method, status_code, version, flags, etc.
-- @treturn[opt] number Return 1 to skip body parsing, 0 or nil for normal parsing
-- @usage
-- callbacks.onHeadersComplete = function(info)
--   print("Method:", info.method)
--   print("HTTP version:", info.http_major .. "." .. info.http_minor)
--   print("Should keep alive:", info.should_keep_alive)
-- end

--- Called when body chunk is parsed
-- @callback onBody
-- @tparam string chunk Body data chunk
-- @usage
-- callbacks.onBody = function(chunk)
--   body = body .. chunk
-- end

--- Called when message parsing is complete
-- @callback onMessageComplete
-- @usage
-- callbacks.onMessageComplete = function()
--   print("Message parsing complete")
-- end

--- Called when chunk header is parsed (chunked encoding)
-- @callback onChunkHeader
-- @tparam number length Chunk length
-- @usage
-- callbacks.onChunkHeader = function(length)
--   print("Chunk size:", length)
-- end

--- Called when chunk parsing is complete (chunked encoding)
-- @callback onChunkComplete
-- @usage
-- callbacks.onChunkComplete = function()
--   print("Chunk complete")
-- end

--- Called when parser is reset
-- @callback onReset
-- @usage
-- callbacks.onReset = function()
--   print("Parser reset")
-- end

return lhttp_parser

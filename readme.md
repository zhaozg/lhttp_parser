# Nodejs `http-parser` binding for lua

This is `http-parser` binding for lua.

## Build

```shell
git clone 
make
```

## Usage

In lua, please use `local lhp = require('lhttpparser')`.

In c application, lhttp_parser can be compiled and linked statically. Just call
the function `luaopen_lhttp_parser(L)` to create a table with the http parser
functions and leave it on the top of the stack.

To test the library run, please run `./test.lua`, 

## API

`lhttp-parser` has tow method:

* `parseUrl` to parse a url, and return a parsed table, which have `schema`, 
`host`, `port`, `port_string`, `path`, `query` and `fragment`.
* `new` to create a request or response parser.

### Callbacks

To create a http parser you need make a table contains any fields of [http_parser_settings](https://github.com/nodejs/http-parser/blob/master/http_parser.h#L324-L338)

| C                   | Lua               |
| :------------------ | :---------------- |
| on_message_begin    | onMessageBegin    |
| on_url              | onUrl             |
| on_status           | onStatus          |
| on_header_field     | onHeaderField     |
| on_header_value     | onHeaderValue     |
| on_headers_complete | onHeadersComplete |
| on_body             | onBody            |
| on_message_complete | onMessageComplete |
| on_chunk_header     | onChunkHeader     |
| on_chunk_complete   | onChunkComplete   |

#### Request

To create a request parser.
```lua
parser = lhp.new('request', {
    onUrl          = function(url)  ... end
    onHeaderField  = function(hkey) ... end
    onHeaderValue  = function(hval) ... end
    onBody         = function(body) ... end

    onMessageBegin    = function() ... end
    onMessageComplete = function() ... end
    onHeadersComplete = function() ... end

    onChunkHeader   = function(content_length) ... end
    onChunkComplete = function() ... end
})

```

#### Response

To create a response parser.

```
parser = lhp.new('request', {
    onStatus       = function(code, text) ... end
    onHeaderField  = function(hkey) ... end
    onHeaderValue  = function(hval) ... end
    onBody         = function(body) ... end

    onMessageBegin    = function() ... end
    onMessageComplete = function() ... end
    onHeadersComplete = function() ... end

    onChunkHeader   = function(content_length) ... end
    onChunkComplete = function() ... end
})
```

### Parser object

Create a new HTTP parser to handle either an HTTP request or HTTP response 
respectively.  Pass in a table of callbacks that are ran when the parser 
encounters the various fields.  All callbacks will be ran with the full value
for that field with onBody being the notible exception.


#### `parser:execute(input_bytes[, i=1[, j=-1]])`

Feed the parser some partial input, that starts at i and continues until j; i and j can be negative. If j is absent, then it is assumed to be equal to -1 (which is the same as the string length), same with `string.sub(...)`. The first character is at position 1 (not at 0, as in C). Indices are allowed to be negative and are interpreted as indexing backwards, from the end of the string. Thus, the last character is at position -1, and so on.

Returns how many bytes where read.  A short read may happen if a request is being "upgraded" or was an invalid format.  See parser:is_upgrade() below to differentiate between these two events (if you want to support an upgraded protocol).

#### `parser:finish()`

Tell the parser end of input.

#### `parser:http_version()`

Returns HTTP version as two numbers (major, minor), only valid on requests.

#### `parser:status_code()`

Returns the HTTP status code of a response.  This is only valid on responses.

#### `parser:method()`

Return name of method which maybe `DELETE`, `GET`, `HEAD`, `POST`, `PUT`,
`CONNECT`, `OPTIONS`, `TRACE`, `COPY`, `LOCK`, `MKCOL`, `MOVE`, `PROPFIND`,
`PROPPATCH`, `UNLOCK`, `REPORT`, `MKACTIVITY`, `CHECKOUT`, `MERGE`, `MSEARCH`,
`NOTIFY`, `SUBSCRIBE`, `UNSUBSCRIBE` or `UNKNOWN_METHOD`, only valid on
requests.

#### `parser:should_keep_alive()`

Returns true if this TCP connection should be "kept alive" so that it can be 
re-used for a future request/response. If this returns false and you are the
server, then respond with the "Connection: close" header.  If you are the 
client, then simply close the connection.

####  `parser:is_upgrade()`

Returns true if the input request wants to "upgrade" to a different protocol.

#### `parser:http_errno()`

Returns errno(number), error name(string), error description(string).

#### `parser:reinitialize('request|response', tables)`
Re-initialize HTTP parser clearing any previous error/state.

## Documentation

Detailed API documentation is available in LDoc format. To generate the documentation:

```shell
# Install LDoc if not already installed
luarocks install ldoc

# Generate documentation
ldoc .
```

The documentation will be generated in the `doc/` directory.

## Quality Improvements

This library has been improved with:

1. **Enhanced Memory Management**: 
   - Proper cleanup of Lua registry references in the garbage collector
   - Safe handling of Lua state pointers to prevent use-after-free issues
   - Protection against invalid reference cleanup

2. **Improved Error Handling**:
   - Added safety checks for NULL Lua state pointers in callbacks
   - Consistent error reporting across all parser methods
   - Better error propagation with detailed error reasons

3. **Comprehensive Documentation**:
   - Full LDoc API documentation for all functions and callbacks
   - Detailed usage examples for common scenarios
   - Complete callback reference with parameter descriptions


HPARSER	?= llhttp
uname_S	 =$(shell uname -s)
OBJS	 =lhttp_parser.o lhttp_parser_url.o http_parser.o url_parser.o lhttp_url.o

ifeq (Darwin, $(uname_S))
  CFLAGS=-Ihttp-parser -I/usr/local/include/luajit-2.1 -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -Wall -Werror -fPIC
  LIBS=-lm -lpthread -lluajit-5.1 -L/usr/local/lib/
else
  CFLAGS=-Ihttp-parser -I/usr/local/include/luajit-2.1 -I/usr/include/luajit-2.1 -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -Wall -Werror -fPIC
  LIBS=-lm -lpthread -lrt
endif

TARGET  = $(MAKECMDGOALS)
# asan {{{

ifeq (asan, ${TARGET})
ifeq (Darwin, $(uname_S))
  CC             = clang
  ASAN_LIB       = $(shell dirname $(shell dirname $(shell clang -print-libgcc-file-name)))/darwin/libclang_rt.asan_osx_dynamic.dylib
  LDFLAGS       +=-g -fsanitize=address
endif
ifeq (Linux, $(uname_S))
  CC             = clang
  ASAN_LIB       = $(shell dirname $(shell cc -print-libgcc-file-name))/libasan.so
  LDFLAGS       +=-g -fsanitize=address -lubsan
endif
CC            ?= clang
LD            ?= clang
CFLAGS	+=-g -O0 -fsanitize=address,undefined
endif
# asan }}}


SHARED_LIB_FLAGS=-shared -o
ifeq (llhttp, $(HPARSER))
  CFLAGS+=-DUSE_LLHTTP -Illhttp/include
  OBJS+=api.o llhttp.o http.o
endif

all: lhttp_parser.so lhttp_url.so

http_parser.o: http-parser/http_parser.c
	$(CC) -c $< -o $@ ${CFLAGS}

url_parser.o: http-parser/contrib/url_parser.c
	$(CC) -c $< -o $@ ${CFLAGS}

http.o: llhttp/src/http.c
	$(CC) -c $< -o $@ ${CFLAGS}

api.o: llhttp/src/api.c
	$(CC) -c $< -o $@ ${CFLAGS}

llhttp.o: llhttp/src/llhttp.c
	$(CC) -c $< -o $@ ${CFLAGS}

lhttp_parser.o: lhttp_parser.c
	$(CC) -c $< -o $@ ${CFLAGS}

lhttp_url.o: lhttp_url.c
	$(CC) -c $< -o $@ ${CFLAGS}

lhttp_parser_url.o: lhttp_parser_url.c
	$(CC) -c $< -o $@ ${CFLAGS}

lhttp_parser.so: ${OBJS}
	$(CC) ${CFLAGS} ${SHARED_LIB_FLAGS} $@ ${OBJS} ${LIBS}

lhttp_url.so: ${OBJS}
	$(CC) ${CFLAGS} ${SHARED_LIB_FLAGS} $@ ${OBJS} ${LIBS}

asan: all
ifeq (Darwin, $(uname_S))
	ASAN_LIB=$(ASAN_LIB) \
	LSAN_OPTIONS=suppressions=${shell pwd}/.github/asan.supp \
	DYLD_INSERT_LIBRARIES=$(ASAN_LIB) \
	luajit tests/run.lua
endif
ifeq (Linux, $(uname_S))
	ASAN_LIB=$(ASAN_LIB) \
	LSAN_OPTIONS=suppressions=${shell pwd}/.github/asan.supp \
	LD_PRELOAD=$(ASAN_LIB) \
	luajit tests/run.lua
endif

clean:
	rm -f *.so *.o

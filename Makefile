uname_S	 =$(shell uname -s)
OBJS	 =lhttp_parser.o llhttp_url.o lhttp_url.o api.o llhttp.o http.o

ifeq (Darwin, $(uname_S))
  LJDIR ?= /usr/local/opt/luajit
  LIBS=-lm -lpthread -lluajit-5.1 -L${LJDIR}/lib/
else
  LJDIR ?= /usr
  LIBS=-lm -lpthread -lrt -lluajit-5.1
endif

CFLAGS	?= -g
CFLAGS  +=-Ihttp-parser -Illhttp/include -I${LJDIR}/include/luajit-2.1 -Wall -Werror -fPIC

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

.PHONY:  doc test

all: lhttp_parser.so lhttp_url.so

doc:
	ldoc -f markdown .

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

llhttp_url.o: llhttp_url.c
	$(CC) -c $< -o $@ ${CFLAGS}

lhttp_parser.so: ${OBJS}
	$(CC) ${CFLAGS} ${SHARED_LIB_FLAGS} $@ ${OBJS} ${LIBS}

lhttp_url.so: ${OBJS}
	$(CC) ${CFLAGS} ${SHARED_LIB_FLAGS} $@ ${OBJS} ${LIBS}

test: all
	busted
	luajit bench.lua

asan: all
ifeq (Darwin, $(uname_S))
	ASAN_LIB=$(ASAN_LIB) \
	LSAN_OPTIONS=suppressions=${shell pwd}/.github/asan.supp \
	DYLD_INSERT_LIBRARIES=$(ASAN_LIB) \
	luajit tests/test-basic.lua
endif
ifeq (Linux, $(uname_S))
	ASAN_LIB=$(ASAN_LIB) \
	LSAN_OPTIONS=suppressions=${shell pwd}/.github/asan.supp \
	LD_PRELOAD=$(ASAN_LIB) \
	luajit tests/test-basic.lua
endif

clean:
	rm -f *.so *.o

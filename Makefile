HPARSER	?= llhttp
uname_S	 =$(shell uname -s)
OBJS	 =lhttp_parser.o

ifeq (Darwin, $(uname_S))
  CFLAGS=-Ihttp-parser -I/usr/local/include/luajit-2.1 -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -Wall -Werror -fPIC
  LIBS=-lm -lpthread -lluajit-5.1 -L/usr/local/lib/
else
  CFLAGS=-Ihttp-parser -I/usr/local/include/luajit-2.1 -I/usr/include/luajit-2.1 -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -Wall -Werror -fPIC
  LIBS=-lm -lpthread -lrt
endif

ifeq (llhttp, $(HPARSER))
  CFLAGS+=-DUSE_LLHTTP -Illhttp/include
  OBJS+=api.o llhttp.o http.o
  SHARED_LIB_FLAGS=-shared -o lhttp_parser.so
else
  OBJS+=http_parser.o
  SHARED_LIB_FLAGS=-shared -o lhttp_parser.so
endif
CFLAGS += -g

all: lhttp_parser.so

http_parser.o: http-parser/http_parser.c
	$(CC) -c $< -o $@ ${CFLAGS}

http.o: llhttp/src/http.c
	$(CC) -c $< -o $@ ${CFLAGS}

api.o: llhttp/src/api.c
	$(CC) -c $< -o $@ ${CFLAGS}

llhttp.o: llhttp/src/llhttp.c
	$(CC) -c $< -o $@ ${CFLAGS}

lhttp_parser.o: lhttp_parser.c
	$(CC) -c $< -o $@ ${CFLAGS}

lhttp_parser.so: ${OBJS}
	$(CC) ${SHARED_LIB_FLAGS} ${OBJS} ${LIBS}

clean:
	rm -f *.so *.o

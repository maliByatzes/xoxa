# Makefile for server files

CC=clang
CFLAGS=-fsanitize=address -O2 -fno-omit-frame-pointer -Wall -Wextra -lsqlite3

SRC_DIR=.
EXE_DIR=../bin

EXE= ${EXE_DIR}/server

${EXE_DIR}/%: *.o
	${CC} -o $@ $^ ${CFLAGS}

%.o: ${SRC_DIR}/%.c
	${CC} -c $^ ${CFLAGS}

all:		${EXE}

clean:
	@rm -rf ${EXE} *.o

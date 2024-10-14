# Makefile for server files

CC=clang
CFLAGS=-fsanitize=address -O2 -fno-omit-frame-pointer -Wall -Wextra
LIBS=-I ../include

SRC_DIR=.
EXE_DIR=../bin

EXE= ${EXE_DIR}/server

${EXE_DIR}/%: *.o
	${CC} -o $@ $^ ${CFLAGS} ${LIBS}

%.o: ${SRC_DIR}/%.c
	${CC} -c $^ ${CFLAGS} ${LIBS}

all:		${EXE}

clean:
	@rm -rf ${EXE} *.o

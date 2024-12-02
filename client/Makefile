# Makefile for client files

CC=clang
CFLAGS=-fsanitize=address -O2 -fno-omit-frame-pointer -Wall -Wextra -lncurses -pthread

SRC_DIR=.
EXE_DIR=../bin

EXE= ${EXE_DIR}/client

${EXE_DIR}/%: *.o
	${CC} -o $@ $^ ${CFLAGS}

%.o: ${SRC_DIR}/%.c
	${CC} -c $^ ${CFLAGS}

all:		${EXE}

clean:
	@rm -rf ${EXE} *.o

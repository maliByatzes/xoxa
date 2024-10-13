# Makefile for server files

CC=clang
LIBS=-I ../include

SRC_DIR=.
EXE_DIR=../bin

EXE= ${EXE_DIR}/server

${EXE_DIR}/%: *.o
	${CC} -o $@ $^ ${LIBS}

%.o: ${SRC_DIR}/%.c
	${CC} -c $^ ${LIBS}

all:		${EXE}

clean:
	@rm -rf ${EXE} *.o

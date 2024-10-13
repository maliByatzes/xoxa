# Makefile for client files

CC=clang
LIBS=-I ../include

SRC_DIR=.
EXE_DIR=../bin

EXE= ${EXE_DIR}/client

${EXE_DIR}/%: %.o
	${CC} -o $@ $< ${LIBS}

%.o: ${SRC_DIR}/%.c
	${CC} -o $@ -c $< ${LIBS}

all:		${EXE}

clean:
	@rm -rf ${EXE}

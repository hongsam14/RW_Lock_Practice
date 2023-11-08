#*****************************#
#          makefile           #
#*****************************#

NAME = RWLOCK
CC = gcc
CFLAGS = -Wall -Werror -pthread

OS = $(shell uname -s)

LIBS =
ifeq (${OS}, linux)
	LIBS += -pthread
endif

SRCS = reader_writer.c

OBJS = ${SRCS:c=o}


all : ${NAME}

${NAME}: ${OBJS}
	${CC} ${LIBS} $^ -o $@

%.o : %.c
	${CC} ${CFLAGS} -c $<

clean :
	rm -f ${NAME} ${OBJS}

re : clean all

.PHONY:
	all clean re

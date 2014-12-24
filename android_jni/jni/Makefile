CC=gcc
DEPENDFLAG= 
LIBS=-lpthread
INCLUDES=-I.

CFLAGS:=$(INCLUDES) -g -Wall

LDFLAGS= $(LIBS) $(DEPENDFLAG) 

SRCDIRS=.
PROGRAMS=speed
SRCS=main.c message_list.c tcp_conn.c tcp_echo.c udp_echo.c report.c

objects=$(SRCS:.c=.o)

all : $(objects)
	$(CC) -o $(PROGRAMS) $(filter %.o ,$+) $(LDFLAGS)
	@rm *.o

$(objects): %.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	@rm -rf $(OBJS) $(PROGRAMS) *.o

cl:clean


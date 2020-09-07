#############################################################################
# Makefile for: Syspro: Project 3: webserver				    #
#############################################################################


CC            = gcc
CFLAGS        = -g -Wall -W -pthread
DEL_FILE      = rm -f

OBJECTS       = main.o webserver.o command_line_utils.o tools.o list.o 

default: server

server: main.o webserver.o tools.o list.o command_line_utils.o
	$(CC) $(CFLAGS) -o server $(OBJECTS)

main.o: main.c webserver.h \
	       command_line_utils.h 
	$(CC) -c $(CFLAGS) -o main.o main.c

webserver.o: webserver.c list.h \
			 tools.h \
			 webserver.h \
			 command_line_utils.h 
	$(CC) -c $(CFLAGS) -o webserver.o webserver.c

command_line_utils.o: command_line_utils.c command_line_utils.h
	$(CC) -c $(CFLAGS) -o command_line_utils.o command_line_utils.c

tools.o: tools.c list.h \
		 tools.h \
		 webserver.h \
		 command_line_utils.h 
	$(CC) -c $(CFLAGS) -o tools.o tools.c

list.o: list.c list.h \
	       tools.h \
	       webserver.h 
	$(CC) -c $(CFLAGS) -o list.o list.c


clean:
	-$(DEL_FILE)  server
	-$(DEL_FILE)  $(OBJECTS)

CC = gcc
CFLAGS_OSX = -lusb -framework CoreFoundation -framework IOKit -lreadline
CFLAGS_LNX = -lusb -lreadline

all:
		@echo 'ERROR: no platform defined.'
		@echo 'LINUX USERS: make linux'
		@echo 'MAC OS X USERS: make macosx'
	
macosx:
		$(CC) irecovery.c -o irecovery $(CFLAGS_OSX)

linux:
		$(CC) irecovery.c -o irecovery $(CFLAGS_LNX)


CC = gcc
CFLAGS_OSX = -lusb -framework CoreFoundation -framework IOKit -lreadline
CFLAGS_LNX = -lusb -lreadline

all:
		@echo 'ERROR: no platform defined.'
		@echo 'LINUX USERS: make linux'
		@echo 'MAC OS X USERS: make macosx'
	
macosx:
		$(CC) iRecovery.c -o iRecovery $(CFLAGS_OSX)

linux:
		$(CC) iRecovery.c -o iRecovery $(CFLAGS_LNX)


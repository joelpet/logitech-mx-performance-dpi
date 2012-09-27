##################################################
## General configuration
## =====================

# Every Makefile should contain this line:
SHELL=/bin/sh

# Program for compiling C programs.
CC=gcc

# Extra flags to give to the C preprocessor and programs that use it (the C and
# Fortran compilers). 
CPPFLAGS=

# Extra flags to give to the C compiler (both at compiling and linking)
# Notice: `make CFLAGS='-g -O' will replace the value of CFLAGS given below
CFLAGS=-Wall -Wextra

# Add any custom flags required for this build before $(CFLAGS) below.
all_cflags=-lusb-1.0 --std=c99 $(CFLAGS)

# Extra flags to give compilers when invoking the linker, `ld'. 
LDFLAGS=-lusb-1.0


##################################################
## Variables setup
## ==============

objects=performance_mx_dpi.o
executable=performance_mx_dpi


##################################################
## Targets
## =======

all: $(executable)

$(executable): $(objects)
	$(CC) $^ -o $@ $(LDFLAGS)

%.o : %.c
	$(CC) -c $(CPPFLAGS) $(all_cflags) $< -o $@

clean:
	-rm -f $(objects) $(executable)

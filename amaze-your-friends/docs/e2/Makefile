#
# E version 1.3, November 1988
# Terry Jones (tcjones@dragon.waterloo.edu)
#



# UNIX *must* be one of BSD, SUN, SYSV, ULTRIX or DYNIX.
UNIX = Sun


# There's probably no need to change anything below this.

CFLAGS = -O -D$(UNIX)
HEADERS = e.h
OBJ = \
	abandon.o \
	ask_hist.o \
	check_hist.o \
	dir_check.o \
	dir_find.o \
	do_vi.o \
	e.o \
	e_error.o \
	find_hist.o \
	find_match.o \
	get_temp.o \
	inheritance.o \
	insert_cmd.o \
	io.o \
	main.o \
	multiple.o \
	new_vi.o \
	normal.o \
	nth_hist.o \
	read_hist.o \
	reconstruct.o \
	signal.o \
	sp_dist.o \
	spell_help.o \
	terminal.o

SRC = \
	abandon.c \
	ask_hist.c \
	check_hist.c \
	dir_check.c \
	dir_find.c \
	do_vi.c \
	e.c \
	e_error.c \
	find_hist.c \
	find_match.c \
	get_temp.c \
	inheritance.c \
	insert_cmd.c \
	io.c \
	main.c \
	multiple.c \
	new_vi.c \
	normal.c \
	nth_hist.c \
	read_hist.c \
	reconstruct.c \
	signal.c \
	sp_dist.c \
	spell_help.c \
	terminal.c

e : $(HEADERS) $(OBJ)
	cc $(CFLAGS) -o e $(OBJ)
	strip e

abandon.o        : e.h
ask_hist         : e.h
check_hist       : e.h
dir_check        : e.h
dir_find.o       : e.h dir_find.h
do_vi.o          : e.h
e.o              : e.h
e_error.o        : e.h
find_hist.o      : e.h
find_match.o     : e.h
get_temp.o       : e.h
inheritance.o    : e.h
insert_cmd.o     : e.h
io.o             : e.h
main.o           : e.h
multiple.o       : e.h
new_vi.o         : e.h
normal.o         : e.h
nth_hist.o       : e.h
read_hist.o      : e.h
reconstruct.o    : e.h
signal.o         : e.h
spell_help.o     : e.h
terminal.o       : e.h

lint :
	lint -D$(UNIX) *.[ch]

tags : $(SRC)
	ctags *.[ch]

shar : tags
	manifest.sh
	makekit -s30k README e.1 Makefile MANIFEST MODS *.[ch] e_update.sh tags

manifest:
	manifest.sh

doc : e.1
	troff -man -Tdumb e.1 | ddumb > e.cat 

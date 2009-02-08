## Process this file with automake to produce Makefile.in

SUBDIRS = . runtime

bin_PROGRAMS = clewn
clewn_SOURCES = clewn.c clewn.h gdb.h gdb_lvl2.c gdb_lvl3.c \
	       misc.c misc.h netbeans.c obstack.c obstack.h pty.c

AM_CFLAGS = -DHAVE_CLEWN
if DEBUG
AM_CFLAGS += -g -DGDB_MTRACE
endif




#
# Copyright (c) 1992 Jill Kliger and Wesley Craig.  All Rights Reserved.
#

# BINDIR is where the vilearn shell script will live. It should be in
# the path of new users.
BINDIR=	/usr/local/bin

# TUTORIALS is the directory where the master copies of the tutorials
# will live.
TUTORIALS=	/usr/local/lib/vilearn

TUTS=	1basics 2moving 3cutpaste 3temp 4inserting 5tricks

all :

install : all
	-mkdir ${TUTORIALS}
	for i in ${TUTS} ; do \
	    rm -f ${TUTORIALS}/$$i; \
	    install -c -m 444 $$i ${TUTORIALS}/$$i ; \
	done
	sed -e s@:TUTORIALS:@${TUTORIALS}@ < vilearn.sh > vilearn
	install -m 755 vilearn ${BINDIR}/vilearn

clean :

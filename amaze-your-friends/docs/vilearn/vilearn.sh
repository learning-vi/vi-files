#!/bin/sh
#
# Copyright (c) 1992 Jill Kliger and Wesley Craig.  All Rights Reserved.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted,
# provided that the above copyright notice appears in all copies and that
# the copyright notice, this permission notice, and an explicit record of
# any local changes, appear in supporting documentation.  This software
# is supplied as is without expressed or implied warranties of any kind.
#

TUTORIALS=:TUTORIALS:

TUTORIAL=

echo "The following tutorials are available:"
echo

echo -n "	Tutorial 1 -- Basic Editing"
if [ -f ${HOME}/VILEARN/1basics ]; then
    echo " (previously viewed)"
else
    echo
fi

echo -n "	Tutorial 2 -- Moving Efficiently"
if [ -f ${HOME}/VILEARN/2moving ]; then
    echo " (previously viewed)"
else
    echo
fi

echo -n "	Tutorial 3 -- Cutting and Pasting"
if [ -f ${HOME}/VILEARN/3cutpaste ]; then
    echo " (previously viewed)"
else
    echo
fi

echo -n "	Tutorial 4 -- Inserting"
if [ -f ${HOME}/VILEARN/4inserting ]; then
    echo " (previously viewed)"
else
    echo
fi

echo -n "	Tutorial 5 -- Tricks and Timesavers"
if [ -f ${HOME}/VILEARN/5tricks ]; then
    echo " (previously viewed)"
else
    echo
fi

echo
echo "You will be editing your own copy of the tutorial. Enter the number"
echo -n "of the tutorial you'd like to see (or q to quit): "

while [ x$TUTORIAL = x ]; do
    read _ans
    case "$_ans" in
	[qQ]*)
	    echo "You lose. Thanks for playing."
	    exit
	    ;;

	*[0-9][0-9]*)
	    echo "No sir, didn't like it."
	    echo -n "Type 1, 2, 3, 4, 5, or q: "
	    ;;

	*1*|*[Bb]asic*)
	    TUTORIAL=1basics
	    ;;

	*2*|*[Mm]ov*)
	    TUTORIAL=2moving
	    ;;

	*3*|*[Cc]ut*|*[Pp]ast*)
	    TUTORIAL=3cutpaste
	    EXTRA=3temp
	    ;;

	*4*|*[Ii]sert*)
	    TUTORIAL=4inserting
	    ;;

	*5*|*[Tt]rick*)
	    TUTORIAL=5tricks
	    ;;

	*)
	    echo "No sir, didn't like it."
	    echo -n "Type 1, 2, 3, 4, 5, or q: "
	    ;;

    esac
done

if [ ! -d ${HOME}/VILEARN ]; then
    mkdir ${HOME}/VILEARN
fi

if [ -f ${HOME}/VILEARN/${TUTORIAL} ]; then
    echo
    echo "There is already a copy of the tutorial in your home directory."
    echo -n "Would you like a fresh copy of the tutorial? "
    read _ans
    if [ `expr x"${_ans}" : 'x[Yy].*'` -ne 0 ]; then
	echo "Copying tutorial..."
	sleep 2
	rm -f ${HOME}/VILEARN/${TUTORIAL}
	cp ${TUTORIALS}/${TUTORIAL} ${HOME}/VILEARN/${TUTORIAL}
	if [ x${EXTRA} != x ]; then
	    rm -f ${HOME}/VILEARN/${EXTRA}
	    cp ${TUTORIALS}/${EXTRA} ${HOME}/VILEARN/${EXTRA}
	fi
    else
	echo "Using existing tutorial..."
	sleep 2
    fi
else
    echo "Copying tutorial..."
    sleep 2
    cp ${TUTORIALS}/${TUTORIAL} ${HOME}/VILEARN/${TUTORIAL}
    if [ x${EXTRA} != x ]; then
	cp ${TUTORIALS}/${EXTRA} ${HOME}/VILEARN/${EXTRA}
    fi
fi

cd ${HOME}/VILEARN; vi ${TUTORIAL}

#
# Feel free to delete this on your local system. Do not remove
# if you are redistributing: we'd like the truth to be known :)
#
cat << EOF

	We hope you have enjoyed and profitted from the tutorial you
	have just completed. It is unfortunate that we did not profit
	from these tutorials in any way. We would like to sincerely
	thank the Information Technology Division of the University of
	Michigan, a huge bureaucracy with a budget exceeding two
	million dollars.  We offered ITD complete ownership of these
	tutorials, including user testing and staff training sessions,
	for the contemptibly small fee of two thousand dollars. For
	some reason, they could not justify the cost...

EOF

echo
echo "Here endeth the lesson."
exit 0

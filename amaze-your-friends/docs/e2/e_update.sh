#!/bin/sh

#
# e_update.sh - Walk your directory tree, finding all the old .e files and
#               turning the contents into a single file ~/.e
#
# To be used when changing from old versions of e to version 1.3.
#
# DON'T expand(1) this file - there's a TAB in the second sed command.
# This assumes you don't have .e files hanging around that weren't put
# there by e.
#

tmp=/tmp/dotty.$$

if [ -f $tmp ]
then
    echo $tmp exists! Try again.
    exit
fi

cd
HOME=`pwd`
cd /
for i in `find $HOME -name .e -print`
do
    #
    # remove "/.e" from the end.
    #
    echo $i | sed -e 's/\/\.e$//'         

    #
    # insert a TAB at the front of each name
    #
    cat $i | tail -8 | sed -e 's/^/	/'

    #
    # remove the old file.
    #
    rm $i

done > $tmp

mv $tmp $HOME/.e

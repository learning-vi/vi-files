From gwc@root.co.uk Tue Sep 24 20:15:06 1991
From: Geoff Clare <gwc@root.co.uk>
Date: Tue, 24 Sep 1991 17:10:57 WET DST

> Thank you for your interest.
> 
> I have the macros, but I would like to have a "pure" version from
> you... Eg. the laitest version.
> 
> if you could uuencode the macros, all would be fine.

OK, here's the latest.  The only change from the last version I posted
to comp.editors is the addition of a *D macro, to delete from a given
column to end-of-line over a range of lines (it handles ragged right,
whereas *d can't).

In case you need some explanatory text to put with them, I have included
selected remarks from my articles.

---------------------------------------------------------------------

For years people have been complaining that vi doesn't have block
operations.  It has long been an ambition of mine to come up with some
macros to do it.  So here they are!

To use them, first mark the block to be deleted/copied by going to the
top left and typing "ma", then to the bottom right and typing "mb".
It's vital that these marks are set right, or strange things will happen
(you can check by using "`a" and "`b" to move the cursor to each mark).

To delete the marked block, just type "*d".  To copy it, move the cursor
to the new top left position and hit "*t" to insert the copy after the
cursor, or "*T" to insert it before the cursor.  There is also a "*y"
(yank) command used by "*t" and "*T", which can be used on it's own.

Using combinations of these two commands you can do any block delete,
copy or move operation (to do an overwrite-copy you would have to do
an insert-copy followed by a delete).  I have not implemented separate
commands for these because I am running up against the macro text limit.

Since "*d" can't handle "ragged right" blocks, there is also a "*D"
macro, used to delete from the column marked by "ma" to end-of-line over
the lines marked by "ma" and "mb".  (The position on the line of mark b
is not important).

On one system I tested the copy macros on, I got an intermittent problem.
Occasionally I would get a "can't undo in global" error.  I assume this
is a bug in that version of vi since I can't consistently repeat the
problem, and I'm not doing an undo in a global.  To get round this I came
up with an alternative way of executing the final substitute commands
(using a recursive macro).  This is slower than the other method because
it updates the screen after each command instead of once at the end. 
It's also messy in that it terminates by referencing an undefined mark. 
On the system with the problem I have arranged things so that if the
normal block copy fails, I source a file containing the alternative
version and redo it.  The alternative version is commented out at the end
of the macro definitions.

#
# uuencoded file exstracted to  'blocks' by archive maintaner.
#

-- 
Geoff Clare <gwc@root.co.uk>  (USA UUCP-only mailers: ...!uunet!root.co.uk!gwc)
UniSoft Limited, London, England.   Tel: +44 71 729 3773   Fax: +44 71 729 3273


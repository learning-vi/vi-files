#         EVI -- The Emacs mode VI or the Expert's VI
# 
#                              by
# 
#                          Bo Thide'
#              Swedish Institute of Space Physics
#                   S-755 90 Uppsala, Sweden
#        UUCP: ..enea!kuling!irfu!bt  or  bt@irfu.UUCP
#
# evi 0.1, Jan 1, 1988
# 
# 
# This  is  a  test  version  of  an  advanced  macro  package
# providing an  Emacs-like,  modeless  user  interface  to the
# vi(1) display  editor  family.  Run as a Bourne shell script
# 'evi' replaces the existing  EXINIT string by another EXINIT
# string  consisting  mainly of macros in input mode (":map!")
# that emulate a subsest of Emacs (or Word  Perfect)  commands
# as  exactly  as  possible.  Then  vi(1) is run, with the new
# EXINIT string,  through an exec(1)  command.  Since ESC is a
# reserved  (unmappable)  character  in vi(1)  the meta key is
# used to map  8-bit  characters.  This  works  for the  HP-UX
# vi(1) but will not work for other versions of UNIX that only
# support 7-bit ASCII editors and shells.
# 
# A limiting factor for the number of commands emulated is the
# size of the vi(1)  macro  buffer.  The  current  size of the
# macro package is nearly  maximum for HP-UX.  Other  versions
# of vi(1) are known to have smaller buffers.
# 
# Unfortunately the vi(1) commands 'h' and 'l' cannot move the
# cursor to another line (I consider this a serious drawback!)
# so special tricks have been used to emulate line wrap-around
# for the Emacs C-f and C-b commands.  So, for instance,  will
# the C-b command (in vi(1) input mode) at the  beginning of a
# line  cause an error  (beep or  flash!)  that will  take you
# into vi(1)  command mode.  However,  since the vi(1) command
# 'b' can back to the previous  line I have included a command
# mode macro (:map ^B bA) which backs to the last  position of
# the  previous  line and places  vi(1) in input mode again so
# that  subsequent  C-b commands work as expected.  A nuisance
# with vi(1) is that  changing from input to command mode will
# reposition  the cursor one step left.  This is really stupid
# and causes  strange  cursor  movements at the beginning of a
# line.  I have found no way to fool vi(1) here!
# 
# Hitting ",," toggles  between vi and Emacs modes letting you
# change mode instantaneously.  Many commands are missing from
# the Emacs mode and hence vi mode is the only way to get what
# you want.
#
# The following script has been run through 'cat -tv' in order
# that it can be e-mailed safely.   Insert the proper CTRL and
# META sequences where needed!
#
EXINIT="map! ^V^V ,,^F,,\
|map! M-v ,,^B,,\
|map! ,, ^[:so /users/res/bt/vi/e-mode.unmap^[:\"Vi mode^[\
|map! ^A ^[I\
|map! ^D ^[lxi\
|map! ^E ^[A\
|map! ^V^H ^[xi\
|map! ^K ^[lDa\
|map! ^L ^[lmzz.\`zi\
|map! ^N ^[ja\
|map! ^O ^[o\
|map! ^P ^[ka\
|map! ^^ ~\
|map! M-a ^[l(i\
|map! M-b ^[bi\
|map! M-e ^[l)i\
|map! M-f ^[lwi\
|map! M-d ^[ldwi\
|map! M-k ^[ld)i\
|map! M-h ^[dbxi\
|map! M-> ^[GA\
|map! M-< ^[1GI\
|map! ^X:  ^[:so /users/res/bt/vi/e-command.map^[mzLo^[:\
|map! ^X^C ^[ZZ\
|map! ^_ ^X:\"Type /regexp<CR>^[I/\
|map! ^R ^X:\"Type /regexp<CR>^[I?\
|map! M-5 ^X:\"Type /old/new/[g]<CR>^[I:%substitute///g^[2hi\
|map ,, :so /users/res/bt/vi/e-mode.map^[:\"Emacs mode^[i\
|map ^D xJi\
"
export EXINIT 
echo "$CLEAR$TSL$REV Vi mode --- Hit ,, to toggle between Emacs and Vi modes --- $SGR0$FSL"
sleep 1
exec /usr/bin/vi $*


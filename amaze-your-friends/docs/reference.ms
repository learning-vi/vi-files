# From: kevin@ttidca.TTI.COM (Kevin Carothers)
# Subject: another version of vi.ref
# Summary: Just what the world needs. Another vi reference chart :-)
# Posted: Thu Sep 21 20:53:46 1989
# 
# 
# For the benefit of those sites which have access to a typesetter, I have
# tbl'ized the originally posted "vi.ref". 
# 
# Some may like this version, and some may think it really sucks rocks. I
# however like it and find it much easier to read than the original version. 
# There are some minor formatting flaws that I am aware of
# ("=" overlapping "_" box lines, and  some spots where I should tblize some
# ".nf" regions). Otherthan that, it seems to look OK.
# 
# I have run this with good results on an Imagen (tm) brand typesetter.
# I wish everyone else the best of luck :)
# 
# -----------------------------------------------------------------------------
# This is a shell archive.  Remove anything before this line, then
# unpack it by saving it in a file and typing "sh file".  (Files
# unpacked will be owned by you and have default permissions.)
#
# This archive contains:
# vi.ms

echo x - vi.ms
cat > "vi.ms" << '//E*O*F vi.ms//'
.\" To format and print on Imagen printer: 'tbl vi.ms | itroff -ms'
.pl 66v
.if t .pl 11i
.\" .if n .pl 60v
.nr PO 1i
.nr LL 6.5i
.ds LH "VI REFERENCE (maart@cs.vu.nl, tbl by kevin@ttidca)
.ds CH "
.nr % 1
.ds RH "Page: %
.ds LF \*(DY
.ds CF 
.\".P1
\s+2
\fB
.sp
.ce
VI REFERENCE CHART
\fR
\s-2
.ce
.sp
.TS
box, tab (~);
cfB sfP s
l | lfI .
ABBREVIATIONS
=
Command~Meaning
(default value = 1)
_
^X~<ctrl>x
[*]~* is optional
<*>~* must not be taken literally
<sp>~space
<cr>~carriage return
<lf>~linefeed
<ht>~horizontal tab
<esc>~escape
<del>~delete
<a-z>~an element in the range
N~number (* = allowed, - = not used)
CHAR~char unequal to <ht>|<sp>
WORD~word followed by <ht>|<sp>|<lf>
.TE
.bp
.TS
box, tab (~);
cfB s
cfB | cw(5i)
l | lfI .
VI STARTUP
=
vi [file]~T{
.nf
Edit the file and display the first page.
The editor can be initialized by the shell variable 
EXINIT, which looks like:

        EXINIT='<cmd>|<cmd>|...'
        <cmd>:  set options
                map ...
                ab ...
        export EXINIT (in the Bourne shell)

However, the list of initializations can also be 
put into a file. If this file is located in your 
home directory, and is named ".exrc" AND the variable 
EXINIT is NOT set, the list will be automatically 
excuted at startup time. If the 3 conditions are not 
met, you have to give the execute
command yourself:      ":source file (or- :so file)

On-line initializations can be given with 
"vi +<cmd> file", e.g.:

      vi +x file                :cursor is positioned to line x
      vi +/<string> file        :to first occurrence of <string>

Sometimes (e.g. if the system crashed while you were 
editing) it is possible to recover files lost in the 
editor by "vi -r file".  If you just want to view a 
file by using vi, and you want to avoid any change,
instead of vi you can use the "view" command: 
.fi
T}
.TE
.bp
.TS
allbox, tab (~);
cfB s s
cfB c cw(3i)
l | l | lfI .
MOVE COMMANDS
=
Args~Command~Meaning
=
*~h , ^H ~ <*> chars to the left
*~j, <lf>, ^N ~ <*> lines downward
*~l , <sp>~ <*> chars to the right
*~k , ^P~ <*> lines upward
*~$~ to the end of line <*> from the cursor
-~ ^ ~ to the first CHAR of the line
*~\\_~ to the first CHAR <*> - 1 lines lower
*~-~ to the first CHAR <*> lines higher
*~+ , <cr>~ to the first CHAR <*> lines lower
-~0~ to the first char of the line
*~|~ to column <*> (<ht>: only to the endpoint !)
*~f<char>~ <*> <char>s to the right (find)
*~t<char>~ till before <*> <char>s to the right
*~F<char>~ <*> <char>s to the left
*~T<char>~ till after <*> <char>s to the left
*~;~ repeat latest "f"|"t"|"F"|"T" <*> times
*~,~ idem in opposite direction
*~w~ <*> words forward
*~W~ <*> WORDS forward
*~b~ <*> words backward
*~B~ <*> WORDS backward
*~e~ to the end of word <*> forward
*~E~ to the end of WORD <*> forward
*~G~ go to line <*> (default EOF)
*~H~ to line <*> from top of the screen (home)
*~L~ to line <*> from bottom of the screen (last)
-~M~ to the middle line of the screen
*~)~ <*> sentences forward
*~(~ <*> sentences backward
*~}~ <*> paragraphs forward
*~{~ <*> paragraphs backward
-~]]~ to the next section (default EOF)
-~[[~ to the previous section (default begin of file)
-~`<a-z>~ to the mark
-~'<a-z>~ to the first CHAR of the line with the mark
-~``~T{
.na
to the cursor position before the latest absolute
jump (of which are examples "/" and "G") 
.ad
T}
-~''~T{
.na
to the first CHAR of the line on which the cursor
was placed before the latest absolute jump
.ad
T}
.TE
.bp
.TS
allbox, tab (~);
cfB s s
cfB c cw(3i)
l | l | lfI .
MOVE COMMANDS (continued)
=
Args~Command~Meaning
=
-~/<string>~ to the next occurrence of <string>
-~?<string>~ to the previous occurrence of <string>
-~n~ repeat latest "/"|"?" (next)
-~N~ idem in opposite direction
-~%~T{
.na
find the next bracket and go to its match
(also { } and [ ]) 
.ad
T}
.TE
.sp 2
.TS
allbox, tab (~);
cfB s
cfB cw(3i)
l | lfI .
SEARCHING (SEE ABOVE)
=
Command~Meaning
=
^]~T{
.na
search in the tags file where the function under the
cursor is defined (file, line) and go to it
.ad
T}
:[x,y]g/<string>/<cmd>~T{
.na
search globally [from line x to y] after <string>
and execute the "ex" <cmd> on each occurrence
.ad
T}
=
.T&
cfB s
l | lfI .
UNDOING CHANGES
=
Args~Command
=
u~ undo the latest change
U~ undo all changes on a line, 
~while not having moved off it (unfortunately)
:q!~ quit vi without writing
:e!~ re-edit a messed-up file
.TE
.bp
.TS
allbox, tab (^);
cfB s s
cfB c cw(3i)
l | l | lfI .
APPENDING TEXT (end with <esc>) 
=
Args^Command^Meaning
=
*^a^ <*> times after the cursor
*^ A^ <*> times at the end of line
*^ i^ <*> times before the cursor (insert)
*^ I^ <*> times before the first CHAR of the line
*^ o^T{
.na
on a new line below the current (open)
the count is only useful on a slow terminal
.ad
T}
* ^ O^T{
.na
on a new line above the current
the count is only useful on a slow terminal
T}
* ^ ><move>^T{
.na
shift the lines described by <*><move> one
shiftwidth to the right (layout)
T}
* ^ >>^ shift <*> lines one shiftwidth to the right
* ^ .^ repeat latest command <*> times
* ^ ["<a-z1-9>]p^T{
.na
put the contents of the (default undo) buffer <*>
times after the cursor
a buffer containing lines is put only once, below
the current line
.ad
T}
* ^ ["<a-z1-9>]P^T{
.na
put the contents of the (default undo) buffer <*>
times before the cursor
a buffer containing lines is put only once, above

the current line
.ad
T}
.TE
.TS
allbox, tab (^);
cfB s s
cfB c cw(3i)
l | l | lfI .
CHANGING TEXT (END WITH <esc>)
=
Args^Command^Meaning
=
*^ r<char>^ replace <*> chars by <char> - no <esc>
*^ R^ overwrite the rest of the line, append <*> - 1 times
*^ s^ substitute <*> chars
*^ S^ <*> lines
*^ c<move>^ change from begin to endpoint of <*><move>
*^ cc^ <*> lines
*^ C^ the rest of the line and <*> - 1 next lines
*^ =<move>^T{
.na
if the option 'lisp' is set, this command will
realign the lines described by <*><move> as though
they had been typed with the option 'ai' set too
.ad
T}
-^~^switch lower and upper cases
* ^J^ join <*> lines (default 2)
* ^.^ repeat latest command <*> times ("J" only once)
- ^&^T{
.na
repeat latest "ex" substitute command, e.g.
":s/wrong/good"
.ad
T}
.TE
.bp
.TS
box, tab (^);
c s s .
DELETING TEXT
=
T{
Everything deleted can be stored into a buffer. This is achieved 
by putting a " and a letter <a-z> before the delete command. The 
deleted text will be in the buffer with the used letter. 
If <A-Z> is used as buffer name, the adjugate buffer <a-z> will 
be augmented instead of overwritten with the text. The undo buffer 
always contains the latest change. Buffers "<1-9> contain the 
latest 9 LINE deletions ("1 is most recent).
T}
=
.T&
cfB | c | cw(4i)
l | l | lfI . 
Args^Command^Meaning
=
*^x^ delete <*> chars under and after the cursor
_
*^X^ <*> chars before the cursor
_
*^d<move>^ from begin to endpoint of <*><move>
_
*^dd^ <*> lines
_
-^D^ the rest of the line
_
*^<<move>^T{
.na
shift the lines described by <*><move> one
shiftwidth to the left (layout)
.ad
T}
_
*^<<^ shift <*> lines one shiftwidth to the left
_
*^.^ repeat latest command <*> times
.TE
.sp 2
.TS
box, tab (^);
c s s .
REMEMBERING TEXT (yanking) 
=
T{
With yank commands you can put "<a-z> before the command, just as with
delete commands. Otherwise you only copy to the undo buffer. The use of buffers
<a-z> is THE way of copying text to another file: see the ":e <file>" command.
T}
=
.T&
cfB | c | cw(4i) 
l | l | lfI .
Args^Command^Meaning
=
*^y<move>^yank from begin to endpoint of <*><move>
_
*^yy^<*> lines
_
*^Y^idem (should be equivalent to "y$" though)
_
-^m<a-z>^mark the cursor position with a letter
.TE
.bp
.TS
allbox, tab (~);
cfB s
cfB cw(4.5i)
l | lfI .
COMMANDS WHILE IN APPEND or CHANGE MODE
=
~Command~Meaning
^@~T{
.na
if typed as the first character of the insertion, it
is replaced with the previous text inserted (max. 128
chars), after which the insertion is terminated
T}
.ad
^V~T{
.na
deprive the next char of its special meaning ~ (e.g. <esc>)
.ad
T}
^D~one shiftwidth to the left
0^D~T{
.na
remove all indentation on the current line
(there must be no other chars on the line)
.ad
T}
^^D~T{
.na
idem, except that it is restored on the next line
.ad
T}
^T~one shiftwidth to the right
^H~one char back
^W~one word back
^U~T{
.na
back to the begin of the change on the current line
.ad
T}
~(generally your kill char)
<del>~like <esc>
.TE
.bp
.TS
allbox, tab (~);
cfB s
cfB cw(4.5i)
l | lfI .
WRITING, EDITING OTHER FILES, AND QUITTING VI 
=
Command~Meaning
=
:q~ quit vi after writing
:q!~quit vi without writing
:w~write the file
:w <name>~write to the file <name>
:w >> <name>~T{
.na
append the buffer to the file <name>
.ad
T}
:w! <name>~ overwrite the file <name>
:x,y w <name>~T{
.na
write lines x through y to the file <name>
.ad
T}
:wq~T{
.na
write the file and quit vi, WITHOUT checking if the
write has been successful (use "ZZ" instead)
.ad
T}
ZZ~T{
.na
write if the buffer has been changed, and quit vi.
if you have invoked vi with the "-r" option, you'd
better write the file explicitly ("w" or "w!"), or
quit the editor explicitly ("q!") if you don't want
to overwrite the file - some versions of vi don't
handle the "recover" option very well
.ad
T}
:x~ idem
:x!~ ":w!" and ":q"
:e <file>~T{
.na
edit another file without quitting vi - the buffers
are not changed (except the undo buffer), so text can
be copied from one file to another this way
.ad
T}
:e! <file>~T{
.na
idem, without writing the current buffer
.ad
T}
:e#~T{
.na
edit the previous file
.ad
T}
^^~idem
:rew~T{
.na
edit the first file (when "vi file1 file2 ...")
.ad
T}
:rew!~T{
.na
idem, without writing the current buffer
.ad
T}
:n [<file>]~edit the next file
:n! [<file>]~T{
.na
idem, without writing the current buffer
.ad
T}
.TE
.bp
.TS
box, tab (~);
c s
cfB | cw(4i)
l | lfI . 
DISPLAY COMMANDS
=
Command~Meaning
=
^G~T{
.na
give current line number and relative position
.ad
T}
_
^L~T{
.na
refresh the screen (sometimes "^P" or "^R")
.ad
T}
_
^R~T{
.na
sometimes vi replaces a deleted line by a '@', to be
deleted by "^R" (also with option 'noredraw')
.ad
T}
_
[*]^E~ scroll <*> lines downward
_
[*]^Y~ scroll <*> lines upward
_
[*]^D~T{
.na
scroll <*> lines downward
(default the number of the previous scroll;
initialization: half a page)
.ad
T}
_
[*]^U~T{
.na
scroll <*> lines upward
(default the number of the previous scroll;
initialization: half a page)
.ad
T}
_
[*]^F~ <*> pages forward
_
[*]^B~T{
.na
<*> pages backward (in older versions only ^B works)
.ad
T}
_
.T&
c s .
T{
.na
If in the next commands the field <wi> is present, the windowsize will change
to <wi>. The window will always be displayed at the bottom of the screen.
T}
_
.ad
.T&
cfB | cw(4i)
l | lfI . 
[*]z[wi]<cr>~T{
.na
put line <*> at the top of the window
(default the current line)
.ad
T}
_
[*]z[wi]+~T{
.na
put line <*> at the top of the window
(default the first line of the next page)
.ad
T}
_
[*]z[wi]-~T{
.na
put line <*> at the bottom of the window
(default the current line)
.ad
T}
_
[*]z[wi].~T{
.na
put line <*> in the centre of the window
(default the current line)
.ad
T}
.TE
.bp
.TS
box, tab (~);
c s .
\fBMAPPING AND ABBREVIATION\fP
=
T{
When mapping try to remember to turn off the 
option 'timeout' by ":set noto", and with 'map!'
turn on 'remap' by ":set remap". This will  greatly enhance the
vi mapping features for you.
T}
=
.T&
cfB | cfI 
lfP | lw(4.5i)fI . 
Command~Meaning
=
:map <string> <seq>~T{
.na
<string> is interpreted as <seq>, e.g.
":map ^C :!cc %^V<cr>" to compile from within vi
(vi replaces % by the current file name)
.ad
T}
_
:map~ show all mappings
_
:unmap <string>~T{
.na
deprive <string> of its mapping
when vi complains about non-mapped macros (whereas
no typos are made), first do something like
":map <string> Z", followed by ":unmap <string>"
('Z' must not be a macro itself)
T}
.ad
_
:map! <string> <seq>~T{
.na
mapping in append mode, e.g.
":map! \ebe begin^V<cr>end;^V<esc>O<ht>"
when <string> is preceded by ^V, no mapping is done
.ad
T}
_
:map!~ show all append mode mappings
_
:unmap!<string>~T{
.na
deprive <string> of its mapping (see ":unmap")
.ad
T}
_
:ab <string> <seq>~T{
.na
whenever in append mode <string> is followed by a
breakpoint (e.g. <sp> or ','), it is interpreted as
<seq>, e.g. ":ab p procedure"
a char preceded by ^V is not considered a breakpoint
.ad
T}
_
:ab~show all abbreviations
_
:unab <string>~T{
.na
do not consider <string> an abbreviation anymore
(see ":unmap")
.ad
T}
_
@@~repeat last register command
_
@<a-z>~T{
.na
consider the contents of the named register a
command, e.g.:
.nf
   o0^D:s/wrong/good/<esc>"zdd
      explanation:
      o              - open a new line
      0^D            - remove indentation
      :s/wrong/good/ - this input text is an "ex"
                       substitute command
      <esc>          - finish the input
      "zdd           - delete the line just created,
                       into register 'z'
      now you can type @z to substitute 'wrong' by 'good'
      on the current line
.fi
.ad
T}
.TE
.bp
.TS
allbox, tab (~);
cfB s
cfB cw(3i)
l | lfI .
SWITCH AND SHELL COMMANDS
=
Command~Meaning
=
Q | <del><del>~switch from vi to "ex"
:~an "ex" command can be given
:vi~switch from "ex" to vi
:sh~execute a subshell, back to vi by ^D
:!<cmd>~execute a shell <cmd>
:!!~repeat the last shell command
[*]!<move><cmd>~T{
.na
the shell executes <cmd>, with as standard input the
lines described by <*><move>, next the standard
output replaces those lines
(think of cb, sort, nroff, etc.)
.ad
T}
[*]!!<cmd>~T{
.na
give <*> lines as standard input to the shell <cmd>,
next let the standard output replace those lines
.ad
T}
:x,y w !<cmd>~T{
.na
let lines x to y be standard input for <cmd>
(notice the space between 'w' and '!')
.ad
T}
:r!<cmd>~put the output of <cmd> onto a new line
:r <name>~read the file <name> into the buffer
.TE
.bp
.TS
box, tab (~);
c s .
OPTIONS
=
T{
the option 'readonly' will be set
automatically (with ":w!" you can override this option).
The most important commands to set options are:
T}
_
.T&
cfB c
l | lw(5i)fI .
Command~Meaning
=
:set option ~ turn option on
_
:set no option~ turn option off; no <sp> between "no" and the option
_
:set option=value~ give an option a value
_
:set~ show all non-default options and their values
_
:set option?~ show an option's value
_
:set all~ show all options and their values
=
.T&
c s .
MOST USED OPTIONS
=
.T&
cfB | cw(5i)
l | lfI .
Option~Meaning
=
ai~T{
\fBautoindent\fP - in append mode after a <cr> the cursor
will move directly below the first CHAR on the
previous line.
however, if the option 'lisp' is set, the cursor
will align at the first argument to the last open
list.
T}
_
aw~T{
\fBautowrite\fP- write at every shell escape
(useful when compiling from within vi)
T}
_
dir=<string>~T{
\fBdirectory\fP - the directory for vi to make temporary
files (default /tmp)
T}
_
eb~T{
\fBerrorbells\fP - beeps when you goof
(not on every terminal)
T}
_
ic~T{
\fBignorecase\fP - no distinction between upper and lower
cases when searching
T}
_
lisp~T{
.nf
redefine the following commands:
    "(", ")"   - move back/forward over S-expressions
    "{", "}"   - idem, but don't stop at atoms
    "[[", "]]" - go to prev/next line beginning with '('
see option 'ai'
.fi
T}
_
list~<lf> is shown as '$', <ht> as '^I'
_
magic~T{
.nf
some metachars can be used when searching:
      ^<string>    - <string> must begin the line
      <string>$    - <string> must end the line
      .            - matches any char
      [a-z]               - matches any char in the range
      [<string>]   - matches any char in <string>
      [^<string>]  - matches any char not in <string>
      <char>*      - 0 or more <char>s
      \e<<string>\e> - <string> must be a word
.fi
T}
.TE
.bp
.TS
allbox, tab (~);
cfB s
cfB cw(5i)
l | lfI .
MOST USED OPTIONS (continued)
=
Option~Meaning
=
nu~ number - numbers before the lines
para=<string>~T{
\fBparagraphs\fP - every pair of chars in <string> is
considered a paragraph delimiter nroff macro (for "{"
and "}").
A <sp> preceded by a '\e' indicates that the previous
char is a single letter macro.
":set para=P\e bp" introduces '.P' and '.bp' as
paragraph delimiters.
\fBremember:\fP completely empty lines and section
boundaries are paragraph boundaries too.
redraw~ the screen remains up to date
T}
report=<*>~T{
vi reports whenever e.g. a delete
or yank command affects <*> or more lines
T}
ro~T{
\fBreadonly\fP - the file is not to be changed
however, ":w!" will override this option
T}
sect=<string>~T{
\fBsections\fP - gives the section delimiters (for "[[" and
"]]"); see option 'para', however a '{' as first
char on a line also starts a section (C functions!)
T}
sh=<string>~T{
\fBshell\fP - which program is to be used for shell escapes
T}
sw=<*>~T{
\fBshiftwidth\fP- gives the swiftwidth (default sw=8)
T}
sm~T{
\fBshowmatch\fP- whenever you append a ')', vi tries to
show its match by putting for a moment the cursor
onto it (also with { })
T}
terse~ short error messages
ts=<*>~T{
\fBtabstop\fP - the length of a <ht>;
\fBwarning:\fP this is only IN the editor, outside of it
<ht>s have their normal length (default ts=8)
T}
wa~T{
\fBwriteany\fP - no checks when writing (dangerous)
T}
warn~T{
warn you when you try to quit without writing
T}
wi=<*>~T{
\fBwindow\fP - the number of lines vi is to show default
T}
wm=<*>~T{
\fBwrapmargin\fP - when in append mode vi automatically
puts a <lf> whenever there is a breakpoint (e.g. <sp>
or ',') within <wm> columns from the right margin
T}
ws~T{
\fBwrapscan\fP - when searching, the end is considered
'stuck' to the begin of the file
T}
.TE

//E*O*F vi.ms//

exit 0



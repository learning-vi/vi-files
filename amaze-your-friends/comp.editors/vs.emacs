From: jim@blade.stack.urc.tue.nl (Jim Rump)
Newsgroups: comp.unix.misc
Subject: Vi vs EMACS ans C-editing
Date: 23 Mar 92 21:51:08 GMT
Sender: news@tuegate.tue.nl
Organization: MCGV Stack, Eindhoven University of Technology, the Netherlands.
Lines: 408
Status: RO

Hello all,

recently I posted something about wheter to use VI or EMACS for c-source
editing. This resulted in some interesting reactions. This is a summery.

If you edit a lot of c-code then this is something for you to read.  It
mostly gives objective opinios. 

(1) from: Rich

    I am someone who used to be a vi-head.  I decided to bite the bullet and
learn emacs, and I love it.  I actually use epoch which is an X windows
version of emacs.  This is one advantage over vi.  It is an X program
rather than an xterm running vi.
    The main reason I learned emacs is because the choice of using emacs or
vi is not an either/or type of a choice.  The real question is:
        Do I want to know 2 editors or am I happy just knowing one?

    I decided that learning emacs was a good idea because then I would 
would be more knowledgeable regardless of which editor I eventually decided
I liked better.  I am better at my job because I know both vi
and emacs whereas all of the vi people only know vi.

    The main advantage of knowing both editors is that they each are better
for various things.  I generally use vi for any quick edits I am going
to make.  I also use it when I have to perform the same changes on a list
of files.  The ":n" command works wonderfully for this.  I generally use emacs
when I am sitting down and doing long coding sessions.  One great thing
about emacs is that it buffers up all of your files and you can very easily
switch between several files.  Another execellent feature is the undo
facility which will usually allow you to undo every single change to
a specific file 1 by 1.  Emacs has several other great features but I
won't go into any more of them.

    In conclusion,  you will never forget how to use vi, and you will
always need to use it for somethings. (ie.  bringing up a new O.S. or 
compiling emacs).  The real question you should be asking yourself is
whether or not you want to learn a second editor.  The question should never
be "emacs or vi?";  the question should always be "vi or (emacs and vi)?"
I believe that the answer to this question is obvious: the more you know, the
better off you are.  You should realize that most emacs people know vi, whereas
the reverse is not true.  I think this is because most people that have the
patience to learn emacs decide they like it better, and they don't switch
back to vi.

    -rich

P.S.  If you really do only want to know one editor, then I think the
choice has to be vi because this is what comes bundled with most
operating systems.
    
******************************************************************************
(2) from: ahcc!ebenv@uunet.UU.NET (eben) 


I like vi.  I used it for years, and my fingers can do anything in
their sleep, from editing multiple files to sorting a range to writing
a macro.

So, when one day a colleague brought in a copy of emacs (this was
several years ago), I thought, "Great! Now we're going to have THREE
way religious wars!" (we had one vocal "j" user).

Instead, miraculously, everyone switched to emacs.  Even me, who had
sworn a life of love and allegiance to vi.

I can still vi happily when need be.  But when emacs is available, I
am more productive.  I always have several shells around, have an
interactive sql session in a buffer, have lots of files in buffers,
and read and write mail in an integrated way.  If you're stuck on an
antique dumb ascii terminal, rather than at a real terminal (an X
workstation), it gives you windowing, multiple shells, and many of the
things that are productive about a modern GUI.

Over the 7 years since I met emacs, I've helped quite a few colleagues
to consider it.  Without exception, they've stayed switched, and many
have come raving back to me months later wondering how they ever lived
without it.

It's like unix--it's advantages over dos perhaps aren't visible in
your first day or week or month.  But as you grow, you come to be more
productive and less frustrated.  Good luck.

--
Eben

******************************************************************************
(3) from: James E. Leinweiber.


In the interests of avoiding yet another outbreak of the dreaded
editor flame wars (editor preferences are intensely personal, and all too
often dictated by whatever one learned first), allow me to offer the
following teaser document.  I was trying to lure vi users to try
emacs, without much success, but we have only 3-4 programmers in my
organization, so I'm not heartbroken.  Ignore all the references to
local customizations.  I use both vi and emacs every day, learned vi
first, and greatly prefer emacs.  Caveat Lector!

James Leinweber		State Laboratory of Hygiene/Univ. of Wisconsin-Madison
jiml@slh.wisc.edu	       uunet!uwvax!uwslh!jiml	       +1-608-262-0736

-----------------------------------------------------------------------------

			   Emacs versus Vi

Emacs is yet another text editor, which is older, larger and more
sophisticated than Vi. To learn to use it you should get a copy of the
quick reference guide and do the on-line tutorial.  You may run the
tutorial by "emacs", 'backspace' 't'.  In case of problems, abort
commands with '^G' and exit emacs by 'C-x' 'C-c'.  (You may be asked
about saving buffers; just type 'n' or "no" 'RET' when you are
prompted for a response at the bottom of the screen.)

Roughly speaking, Vi is superior for line-oriented editing, while
Emacs is better for other kinds of editing.  For editing program text
it may be a toss-up, but Emacs can also edit odd files (binary, empty,
very long lines, ...)  and is definitely superior for entering new
text.  The main functional differences between Emacs and Vi are that
Emacs supports multiple window editing, horizontal scrolling, is fully
programmable, and has on-line help.  However, only Vi will show line
numbers on the screen. (Vi is a screen interface to a line-oriented
editor; Emacs is not.)

The feel of the two editors is very different.  In Vi you are always
switching modes in order to give commands and enter text; in Emacs
text is self-inserting and commands are activated by a plethora of
control character sequences, using both the control and meta or Escape
keys.  Vi has a larger repertoire of cursor motion commands, a handful
of operators, and short command sequences.  Emacs relies more on
arguments to commands, and on applying commands to regions of text.
Many commands in Emacs are similar to using Vi commands with marks.

Both  editors understand  indenting, filling, and   abbreviating text,
although  Emacs is better  at these.  Both allow  named locations  and
named buffers or registers  for chunks of yanked or  deleted text.  Vi
is more convenient for some things, such as ":g/pattern/m$", which are
not directly built into Emacs.  Emacs has nice commands such as  "C-t"
to switch the  last two  characters,  and "M-- M-c"  to capitalize the
previous word.  Undo is much better in Emacs, as are macros.

For basic editing, using the two dozen or so most common commands, Vi
and Emacs are quite comparable in difficulty of learning and ease of
use.  Since emacs has more concepts than Vi, about four times as many
commands, and a complete Lisp programming language built in, it is
significantly harder to master.  Intermediate users may find that the
on-line help in Emacs outweighs the additional complexity.  The extra
functionality in Emacs comes at a price: it plus its documentation use
large amounts of memory and disk, and it uses slightly more CPU time
than Vi.

			    Learning Emacs

The easiest way to learn emacs is just to type "emacs", and follow the
tutorial directions.  After the tutorial you will know enought to
use emacs.  Next you should study a quick reference card for about 20
minutes, trying out some of the commands.  Don't hesitate to use the
on-line help, and remember that you can undo mistakes regardless of
how many commands you have used in the meantime.  It took me about a
week to get confortable using emacs and stop referring constantly to
the reference card, and it was worth it.  Features of Emacs not found
in Vi and worth investigating include: windows, incremental search,
keyboard macros, recursive edits (particularly from query-replace),
narrow versus wide bounds, and shell mode.


				Quirks

Emacs expects to use C-g for interrupt, C-h for help, and DEL for
erase character, which conflicts with the standard SLH keyboard
conventions. You have three choices: live with the difference, give in
to emacs and put 'stty dec' in your .login file,  or have me remap
your emacs keyboard (I use C-h for character erase, DEL for interrupt,
and C-g for help.).  All three have annonying features, namely
cognitive dissonance, incompatibility with co-workers and the 1100, or
disagreements with the emacs manual.  (E.g., references to C-x DEL
must be translated to C-X C-h).  Start by trying plain emacs, and let
me know which you eventually decide on.

The TAB key behaves unusually in Emacs: in indenting modes it restores
the indent of the current line, rather than inserting a TAB.  In the
minibuffer it does command and filename completion, along with space
and CR.  To insert one, use M-i or quote it (C-q C-i).

Your emacs startup file is called ~/.emacs, and is written in Lisp.  I
have provided a self-installing one to get things started.  There are
a few differences from the documentation, which I will describe.

First, the default mode is "indented-text" rather than "fundamental".
Furthermore, a comment character "#" is defined, so it should be very
easy to edit shell scripts.  You can turn off the indenting with "M-x
text-mode RET".  You can toggle auto-fill with "M-x auto-fill RET".
Mail mode will turn on auto-fill for you.  C-mode will use the usual
Unix indenting style, rather than the default Gnu style.  Personally,
I prefer the Gnu style (see /usr/src/local/screendump.c for an example).

On an HDS terminal, the arrow, scroll and page keys will work, but not
on other types.  As a side effect, the paragraph motion commands have
been moved from M-[ and M-] to M-{ and M-}.  By the way, the Meta key
on the HDS terminals works; otherwise type M-{ etc. as "ESC {". On
other kinds of terminals, you will have to do this.

Emacs prefers to run on terminals with flow control turned off, so
that C-s and C-q can be used as command characters.  For this reason,
it uses alternate termcap entries that delay after slow operations.
Let me know if you have any problems with screen updating, as these
are usually caused by insufficient padding.

CR and LF are changed: CR always indents and LF never does.  Finally,
C-o (open-line) is modified to insert the current fill prefix.  The fill
prefix is set by "C-x ." to the text before the cursor, and cancelled
by "C-a C-x ." (setting it at the beginning of a line).

******************************************************************************
(4) from : Glen Larsen

I use both vi and emacs.  If you're going to be on a u*ix system,
you're going to have to know vi.  You can't beat it for quick edits
and those times that you aren't in your cozy little emacs environment.

For me, emacs is a productivity tool for coding.  It indents as I
write, I can change styles easily (or reformat somebody elses code so
I can view it the way I like it), and writing comments is a breeze.
Compilations are done with a single keystroke, if there are any errors
a single key sequence can get me to that line of code (and pull in the
file into another buffer if necessary).  I can run shell commands on a
region or whole buffer.  I have RCS commands bound to keys and the
dired (directory editing) mode so that I can check revisions out and
in (*and* write the commentary in an editor buffer).  Also, I like
multi-buffer editing and writing lisp macros.

Emacs is a different mindset.  I'm sure your friends have shown you
how great emacs is, and what it can do that vi can't, you better be
prepared to hear that vi can do some things that emacs can't.  Don't
let anyone tell you that it will make you a better software engineer.

For me, vi has become another unix tool just like sed & awk --- great to
have around for those odd moments.  Do what fits your style, but I
think you would be doing yourself a disservice if you don't learn
emacs sufficiently enough to see if it fits that style.

- glen
-- 
Glen Larsen                                               glenl@hadar.fai.com
Fujitsu Network Transmission Systems                            (408)456-7890
Software Technology Group, San Jose, CA, USA 95134


(5) from: Jim Thompson

I have been a unix programmer for the last four years, and for the first
three I used vi.  I always had people telling me what a great editor
emacs was, and how I ought to be using it because it was so great.  But
vi was doing the job just fine for me, and besides, every time I used
emacs, I couldn't figure out how to do *anything*, because of all those
complicated control sequences.

When I started a new job about a year ago, I decided I would bite the
bullet, and learn emacs no matter how hard and frustrating it was.  It
took me about a month to be able to use emacs effectively, and another
six or so before I was able to program lisp well enough to really
customize it.  But it was well worth; I'm noticably more productive
using emacs than I was using vi.

Your friends were right; for editing C code, emacs is fantastic--far
superior to vi.  About the only support vi has for editing code are
auto-indent and the shift commands.  Emacs, on the other hand, has a
"major mode" for editing C code, and it makes the job much easier.

It's most effective when you're entering new code.  As you type in the
code, emacs will automatically adjust the indentation levels to match
the nesting of the C control structures such as for-loops and if-
statements.  Emacs will also automatically match paired characters for
you, such as (, ), {, }, [, ], to help cut down on syntax errors.

Emacs is also better at entering long comments; it can automatically
break long comments and reindent them so they line up with the first
comment line.  You just type the words, no matter how many; emacs does
the rest for you.  Emacs also handles end-of-line comments well; you
can type M-; and the cursor will jump past the end of the line to a
predefined comment column and enter the comment-start characters.

My best advice for you, should you choose to learn emacs, is to get a
book and read it first; it will help you greatly to know something about
emacs before you start trying to learn it.

Good luck!
Jim
--
   _     Jim Thompson           |  Western Geophysical
  | ~-   thompson@wg2.waii.com  |  Exploration Products
 \,  _}  jimt@sugar.neosoft.com |  3600 Briarpark
   \(    (713) 964-6213         |  Houston, TX 77042



******************************************************************************
(5) from: Gwyn Evans

  I find that GNU Emacs very useful for me when C coding as I like it's
options of auto-indent, auto-newline on ';','{' and '}'. All configurable
to match your required coding style, of course!

  I find that it's also very useful being able to have a number of files
open at the same time and to be able to have multiple windows on the screen.
It's best if you have a workstation :-) as you can have a large emacs
window but even on a tty, I find that I use vi for some quick edits but
for more than a couple of lines, I use emacs and then shell out of that
it I need to get to a cli.

  Another useful point is that it's got built-in help, which I found 
very useful when starting. It also have a vi mode, although I don't
know if this has any ':' options as opposed to just the i,r,x,cw, etc 
commands.

Gwyn
--
+======================================================================+
| Gwyn Evans  | evansg@uproar.enet.dec.com |  Uxbridge, Middlesex, UK  |
| DESISCo - Digital Equipment Service Industries Solutions Company Ltd |
|  Views expressed and statements made are mine and not those of DEC   |
+======================================================================+

******************************************************************************
(6) from: Eric Wang


No contest.  I was a vi power-user for two years, and find emacs to be
infinitely more powerful, especially for moderately large, nicely
modularized C (and Lisp and ksh) applications.  In fact, I bet I can
make you switch with just one sentence:

Emacs can edit multiple files on the screen simultaneously.  vi can't.

emacs will let you split your screen into 2 (or more) windows and look
at any 2 (or more) files simultaneously (or the same one in two places).
You can jump back and forth between windows.  You can cut and paste
between them.  They scroll separately.

vi can maintain multiple files in memory, but it can display only one at
a time, and you MUST save or discard ALL your changes to it before vi
lets you switch to another one.

I find that this one distinction between the two is one of the most
significant from a programmer's standpoint.  If you have a moderately
large project of ~10 .c files and ~5 .h files, you will often want to
look at a .h file in one window while you edit a .c file in another
window.  In emacs, this is EASY.  In vi, it's IMPOSSIBLE.  End of case!

:-)

(There are, of course, many more reasons to prefer emacs over vi.
Basically, whatever vi can do, emacs can do better.  The *only*
advantage vi really has over emacs is that it's small, so it takes much
less memory and starts up more quickly.  But then, the average tricycle
is smaller than the average car, too.)

Eric Wang
wang@ibma0.cs.uiuc.edu


******************************************************************************
(7)from: Nick Vargish

I just switched to Emacs from vi myself... Emacs takes a while to get
used to (especially after vi), but you'll really like the difference. The
biggest selling point for me was that emacs is a mostly-modeless,
which means one moves, inserts, deletes, without having to switch
modes -- you know, like REAL text editors.

vi is, after all, just a full-screen display grafted onto a line-based
editor. You'll see this more clearly if you try emacs for a couple of
days in your editing routine.

Emacs can be daunting, but there are some good books out there on it,
and the time you invest in learning Emacs will be well rewarded...




-- 
Nick Vargish
SURAnet Operations
vargish@sura.net


******************************************************************************
(8)from: Linus Tolke

What I believe is:
If you like to start an editor for every file then vi is *much*
faster. This is the only thing I find in favor of vi.

Emacs is best used as an entire environment. You have all the files
you are editing within emacs, do the compilation within emacs, running
and debugging from within emacs.
-- 
	/Linus
*****	Wherever I exec my `which emacs`, is my $HOME.	*****
Linus Tolke				SM7OUU, linus@lysator.liu.se
Student at the				member of SK5EU LiTHSA
Link|ping institute of technology, LiTH	LiTH S{ndare Amat|rer (Ham-club)

-- 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Jim Rump        % E-mail:              %
% Eindhoven       % jim@stack.urc.tue.nl %        
% Netherlands     % (TU-Eindhoven)       %



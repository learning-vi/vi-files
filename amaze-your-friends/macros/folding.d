From: roger@quantime.co.uk (Roger Phillips)
Subject: vi macro to approximate folding editor
Summary: suggestions, not a fix
Date: 21 Aug 91 10:15:27 GMT

In article <8605@ecs.soton.ac.uk> mrd@ecs.soton.ac.uk (Mark Dobie) writes:
>[...]
>Here is attempt 1 at a vi macro which uses the word under the cursor
>as a filename and edits the file.
>:map ;[ yWo:n ^[p"add@a
1. I recommend using [; instead of ;[
	because if ;[ is mapped to something,
	then the single-keystroke command ;
	can't be recognised until another key is hit
	(or until timeout, if timeout is set on).
2. If you use :n <filename> then
	the new filename REPLACES the list of files you were editing.
	It looks like you want to use :e <filename> instead
	(Ctrl-^ still works,
	and :n<RETURN> will take you to
	the next file in the original list).

> [...] A typical
>use is to move to the filename in a #include statement and type ;[
>to edit the include file. When you have finished, ^^ (control ^) takes
>you back where you came from. I bound it to ;[ because it is easy to
>type and close to ^] which is used for tag following.
3. Great thanks! At last I know what Ctrl-^ does
	(until now I'd mapped # to do the same thing).
4. And I'd never heard of ^] before -
	and I use tags a lot.
	This merits a new smiley 8^]

>Now to the problems with it. 
>It uses yW to get the name to make sure that dots are included. This
>means that anything after the filename and before whitespace is also
>included.
>#include	"foo.h"			/* we get foo.h" */
>#include	<foo.h>			/* we get foo.h> */
5. All I can think of is something like:
:map [; y$o^[p:s/^\([a-zA-Z0-9_.]*\)[^a-zA-Z0-9_.].*$/:e! \1/^M"add@a
	that is, yank the rest of the line, and massage it.
	a-zA-Z0-9_. is a guess at acceptable chars in file names:
	it's too restrictive.
6. I look forward to the day
	when there's a comp.editors.vi group
	so I don't have to flaunt all this vi stuff
	under the noses of the viophobes.

Roger

------------


From: mrd@ecs.soton.ac.uk (Mark Dobie)
Subject: Re: vi macro to approximate folding editor
Date: 22 Aug 91 09:43:07 GMT

In <1991Aug21.101527.15530@quantime.co.uk> roger@quantime.co.uk (Roger Phillips) writes:

>In article <8605@ecs.soton.ac.uk> mrd@ecs.soton.ac.uk (Mark Dobie) writes:
>>[...]
>>Here is attempt 1 at a vi macro which uses the word under the cursor
>>as a filename and edits the file.
>>:map ;[ yWo:n ^[p"add@a

Thanks for the response Roger. As this was one of my first vi macros I
was hoping for some feedback.

>1. I recommend using [; instead of ;[
>	because if ;[ is mapped to something,
>	then the single-keystroke command ;
>	can't be recognised until another key is hit
>	(or until timeout, if timeout is set on).

I always have trouble deciding what to bind macros to. I am preparing
a chart of keys (normal,shifted,ctrled) and their vi functions so its
easier to see which combinations are free.

>2. If you use :n <filename> then
>	the new filename REPLACES the list of files you were editing.
>	It looks like you want to use :e <filename> instead
>	(Ctrl-^ still works,
>	and :n<RETURN> will take you to
>	the next file in the original list).

Good point. The only reason I used :n was that I use it at the
keyboard with the wildcards.

>3. Great thanks! At last I know what Ctrl-^ does
>	(until now I'd mapped # to do the same thing).
>4. And I'd never heard of ^] before -
>	and I use tags a lot.
>	This merits a new smiley 8^]

Tags are one of vi's best features. They could be used for so much
more...

>>It uses yW to get the name to make sure that dots are included. This
>>means that anything after the filename and before whitespace is also
>>included.
>>#include	"foo.h"			/* we get foo.h" */
>>#include	<foo.h>			/* we get foo.h> */
>5. All I can think of is something like:
>:map [; y$o^[p:s/^\([a-zA-Z0-9_.]*\)[^a-zA-Z0-9_.].*$/:e! \1/^M"add@a
>	that is, yank the rest of the line, and massage it.
>	a-zA-Z0-9_. is a guess at acceptable chars in file names:
>	it's too restrictive.

Its a problem. The compiler objects to space between the filename and
the " or >. I hear a voice saying...  "It's just a question of finding
the right tool for the job" :-)

>6. I look forward to the day when there's a comp.editors.vi group so I
>   don't have to flaunt all this vi stuff under the noses of the viophobes.

Flaunt ahead. We're all learning here!

				Mark.
-- 
Mark Dobie                              M.Dobie@uk.ac.soton.ecs (JANET)
University of Southampton		M.Dobie@ecs.soton.ac.uk (Bitnet)

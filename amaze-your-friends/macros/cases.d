From: roger@quantime.co.uk (Roger Phillips)
Subject: vi case change [was vi and ordering [was Cntrl D in .exrc]]
Keywords: vi case upper lower
Date: 21 Aug 91 10:46:15 GMT

In article <88138@eerie.acsu.Buffalo.EDU> xiaofei@acsu.buffalo.edu (XiaoFei Wang) writes:
> [...]
>Another question is how vi do lower/upper cases ? The only thing that I
>know of is ~ which shift case one at a time. How do I, for example: 
>
>1) make a word all upper/low cases
>2) make a paragraph all upper/low cases
>3) make region from line m to n all upper/low cases
>4) make a the first letter of a word uppercase and then the cursor goes
>   to the next word 
>5) make a the first letter of every word uppercases in a paragraph
>6) make a the first letter of every word uppercases from line m to n

In the replacement part of a substitute command
(i.e. between the second / and the third /),
\u means make the following character upper case
\l means make the following character lower case
\U means make the rest of the replacement upper case
\L means make the rest of the replacement lower case
e.g.
:12,34s/\<./\u&/g
	upper-cases the first letter of every word
	from line 12 to line 34
:s/[UuLl][PpOo][PpWw][Ee][Rr]/\L&/
	changes "uPpeR" and "lowEr" in any mixture of cases
	to the lower-case version.
	(If ignorecase is set, then the search expression
	could be simply /[ul][po][pw]er/.)
:%s/.*/\U&/
	makes the whole file upper case.

Roger

------------

From: dylan@ibmpcug.co.uk (Matthew Farwell)
Subject: Re: vi and ordering [ was Re: Cntrl D in .exrc ]
Date: 21 Aug 91 08:18:15 GMT

In article <88138@eerie.acsu.Buffalo.EDU> xiaofei@acsu.buffalo.edu (XiaoFei Wang) writes:
>Another question is how vi do lower/upper cases ? The only thing that I
>know of is ~ which shift case one at a time. How do I, for example: 

What you need to look at are the \L, \U, \l, \u, \E and \e replacement
operators [*]. I'll put the examples first, and then more explanation of
them later.

>1) make a word all upper/low cases

If you know the word, then
:1,$s/foo/FOO/g will work.

>2) make a paragraph all upper/low cases

:?^$?,/^$/s/.*/\U&/
(for lower case, replace the \U with \L)

>3) make region from line m to n all upper/low cases
:'m,'ns/.*/\U&/
(same as above for lower case)

>4) make a the first letter of a word uppercase and then the cursor goes
>   to the next word 

~w
If you want to map this, use something like
map q ~w

>5) make a the first letter of every word uppercases in a paragraph

:?^$?,/^$/s/\([^ ][^ ]*\)/\u&/g
(its probably a good idea to change the [^ ] to [a-zA-Z0-9] depending on
what you want)
(replace \u with \l for lower case)

>6) make a the first letter of every word uppercases from line m to n
:'m,'ns/\([^ ][^ ]*\)/\u&/g
(as above for lower case)

Basically, what \U et al do is replace the matched character with its
uppercase(for U) equivalent, or lowercase(for L).  \u and \l only
replace the first letter (as in examples 5 and 6), whereas \U and \L
replace all following letters (as in 2 and 3), until the end of line, or
a \E.  I think (can someone confirm this please) that \e and \E are
interchangeable.

\e becomes useful if you only want to uppercase part of a line.
Say you want to uppercase the second word on each line, like so

:1,$s/^\([^ ]*\) \([^ ]*\) \(.*\)/\1 \U\2\e \3/

Dylan.
[*] I can't think of a better term for them. Tom probably can though.
-- 
Matthew J Farwell: dylan@ibmpcug.co.uk || ...!uunet!ukc!ibmpcug!dylan
	Never trust a programmer with a screwdriver.

-------------------------

From: tchrist@convex.COM (Tom Christiansen)
Subject: Re: vi and ordering [ was Re: Cntrl D in .exrc ]
Date: 21 Aug 91 17:41:49 GMT

From the keyboard of dylan@ibmpcug.CO.UK (Matthew Farwell):
:In article <88138@eerie.acsu.Buffalo.EDU> xiaofei@acsu.buffalo.edu (XiaoFei Wang) writes:
:>Another question is how vi do lower/upper cases ? The only thing that I
:>know of is ~ which shift case one at a time. How do I, for example: 
:
:~w
:If you want to map this, use something like
:map q ~w

Not all vi's support this.  Here's my emulation for it:

"
"       INVERT CASE ON WORDS -- V is like W, v is like w.  3V is fine, but only to EOL.
"       uses both register n and mark n.
map \v ywmno^[P:s/./\~/g^M0"nDdd`n@n
"       abc -> ABC    ABC->abc
map \V yWmno^[P:s/./\~/g^M0"nDdd`n@n
"       abc.xyz -> ABC.XYZ    ABC.XYZ->abc.xyz


--tom

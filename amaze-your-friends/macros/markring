" From: daniel@island.uu.net (Daniel Smith "innovation, not litigation...")
" Newsgroups: comp.unix.questions
" Subject: "too much macro text" in Vi
" Date: 29 Aug 90 16:56:46 GMT
" 
" 
"           Sun OS 4.0.3, Sparcstation 1+, /usr/ucb/vi
"
" I came up with some macros to implement a "mark-ring" in Vi.
" My problem is that if I use the macros a lot, I get the message
" "too much macro text".  I can repeat this behavior by starting
" up Vi and tapping away at the macro keys for about a minute or two.
" Here's what I'm doing in my .exrc
" 
"       jump to entries in our mark ring..keep tapping '=' to traverse
map ]1 :unmap =:map = ]2`a:"to mark 'a'
map ]2 :unmap =:map = ]3`b:"to mark 'b'
map ]3 :unmap =:map = ]4`c:"to mark 'c'
map ]4 :unmap =:map = ]5`d:"to mark 'd'
map ]5 :unmap =:map = ]1`e:"to mark 'e'
map = ]1
"	add current point to the mark ring
map +1 :unmap -:map - +2ma:"marked 'a'
map +2 :unmap -:map - +3mb:"marked 'b'
map +3 :unmap -:map - +4mc:"marked 'c'
map +4 :unmap -:map - +5md:"marked 'd'
map +5 :unmap -:map - +1me:"marked 'e'
map - +1

" 	I can type the '-' key to mark my current point.  Typing '='
" moves me back through the last 5 points (the marks a-e).  It works
" pretty well, for perhaps a few hundred invocations.  I'm wondering
" what is building up in memory to give me the error.  At first I
" tried this without the calls to unmap, then added them thinking that
" they would clear the problem up.  As any Vi macro fan will
" tell you, substitute C-m for all the ^M's.  Thanks for any tidbits.
" 
" 				Daniel
" 
" p.s. yes, this is kludge, but it's the sort of neat one that Vi can
" be arm-wrestled into.  When I have some time between projects I'll
" learn GNU Emacs...don't waste your fingertips trying to convert me :-)

" c.ik -- c-mode  insert-mode keys
"	
"	{, }	indent-and-newline
"	^K	comment, Kommentar
"	^R	paiR of cuRly bRaces, Rumpf
"	^W	while-header
"	^F	for-header
"	^B	break! (fortfahren hinter Rumpf)
"	^A	'()' mAtched pArAntheses, Auf Klammerpaar
"	^L	cursor right
"	^U	erase-last-word (Ersatz fuer ^W)
"	^O	open-line
"	^P	open-line & indent
"
"	^X	context-dependent
"	^Y	helps to define ^X as side effect
"
map!  :map!  
"
map! { {	
map! } }Xo
map!  /*  */ * hhi
map!  {} O	
map!  while (  ) A hi
map!  for ( ;; ) A hhhi
map!  /}o
map!  ()i
map!  la
map!  bdwi
map!  o
map!  o	

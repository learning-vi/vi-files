" From: wyle@ethz.UUCP (Wyle)
" Newsgroups: comp.sources.misc
" Subject: Modula-2 vi macro package
" Date: 8 Aug 87 00:23:21 GMT
" Sender: allbery@ncoast.UUCP
" 
" As promised (ages ago) here follows a set of vi macros inspired by the
" Sara* macro package on Lilith and Ceres computers.  Just put this text
" in the .exrc file in you modula-2 directory, and away you go!
" 
" >>> Please Notice: <<<
" 
" You will have to change the ascii text ^V ^[ and ^M to the ACTUAL
" control characters.  This task is done by the vi commands:
" 
" :1,$s/^V^[/^V^V^V^[/g
" :1,$s/^V^M/^V^V^V^M/g
"
" ########
" #
" # The above have been done by the ARCHIVE MAINTAINER.
" #
" ######## 
" 
" where the text before the slash is a carat "^" and letter combination,
" and the text after the second slash is ACTUAL control characters.
" Why does one need three (3) levels of control v?  I dunno, maybe my
" C-shell eats one.
" 
" One must no longer be jealous of the "electric-C" emacs macro package.
" Here is "electric-Modula-2" for us real quiche eaters.
" 
" Please send me corrections, fixes, suggestions, fan-mail, money, and
" bagels (there are NO bagels in Switzerland).
" 
" Enjoy!   -Mitch
" - ----------------------- cut here ---------------------------------
"
" The vi modula-2 macro set inspired by sara macros:
" Copyright (C) 1987   M F Wyle   Computer Science Department
" Swiss Federal Institute of Technology, Zuerich, Switzerland
" Permission is hereby granted to copy this code provided that
" this copyright notice is copied with it.
"
"
map ;ce iCARDINAL var bcw
map ;C iLONGCARD var bcw
map ;in iINTEGER var bcw
map ;re iREAL var bcw
map ;R iLONGREAL var bcw
map ;bi iBITSET var bcw
map ;bo iBOOLEAN var bcw
map ;se iSET OF var bcw
map ;ar iARRAY OF var bcw
map ;pr iPROCEDURE proc(vars) : BOOLEAN;(* PreConditions:  *)(* PostConditions: *)END proc;kkkkwcw
map ;pt iPOINTER TO var bcw
map ;ex iEXPORT var bcw
map ;im iIMPLEMENTATION MODULE mod; bbcw
map ;fr iFROM mod IMPORT vars; bbbbcw
map ;im iIMPORT module; bbcw
map ;if iIF ( cond ) THEN  statement END;kkkwwcw
map ;ie iIF ( cond ) THEN  statement ELSE statement2END;5kwwcw
map ;ii iELSEIF ( cond2 ) THEN  stmnt2ELSEIF ( cond3 ) THEN stmnt3
map ;wh iWHILE ( cond ) DO  statementEND;kkkwwcw
map ;wi iWITH var DO  statementEND;kkkwcw
map ;fo iFOR var := low TO high DO  statementEND;kkkwcw
map ;rp iREPEAT  statementUNTIL ( condition );kkwcw
map ;lo iLOOP  statement  IF ( cond ) THEN EXIT END;END;kkkwcw
map ;rc ivar = RECORD  var1 : t1  var2 : t2END;kkkkcw
map ;ca iCASE var OF  case1 : stmnt1 ^V|  case2 : stmnt2 ^V|case3 : stmnt3  ELSE defaultEND; (* case *)
map ;ws iWriteString("str");kwwcw
map ;wl iWriteLn;
map ;de i(* debug *)WriteString("string"); WriteInt(var); WriteLn;kwwcw
" - ---------------------- end of vi m-2 macro set ------------------
" 
" Mitchell F. Wyle           | csnet or arpa:  wyle%ifi.ethz.ch@relay.cs.net
" Instituet fuer Informatik  | uucp:           wyle@ethz.uucp
" ETH Zentrum / SOT          | Telephone:      011 41 1 256 5237
" 8092 Zuerich, Switzerland



" From: gregm@otc.otca.oz.au (Greg McFarlane)
" Newsgroups: comp.sources.d,alt.sources,comp.editors
" Subject: VI SOLVES HANOI
" Date: 19 Feb 91 01:32:14 GMT
" 
" Submitted-by: gregm@otc.otca.oz.au
" Archive-name: hanoi.vi.macros/part01
" 
" Everyone seems to be writing stupid Tower of Hanoi programs.
" Well, here is the stupidest of them all: the hanoi solving vi macros.
" 
" Save this article, unshar it, and run uudecode on hanoi.vi.macros.uu.
" This will give you the macro file hanoi.vi.macros.
" Then run vi (with no file: just type "vi") and type:
" 	:so hanoi.vi.macros
" 	g
" and watch it go.
" 
" The default height of the tower is 7 but can be easily changed by editing
" the macro file.
" 
" The disks aren't actually shown in this version, only numbers representing
" each disk, but I believe it is possible to write some macros to show the
" disks moving about as well. Any takers?
" 
" (For maze solving macros, see alt.sources or comp.editors)
" 
" Greg
" 
"
" ------------ REAL FILE STARTS HERE ---------------
set remap
set noterse
set wrapscan
" to set the height of the tower, change the digit in the following
" two lines to the height you want (select from 1 to 9)
map t 7
map! t 7
map L 1G/tX/^0$P1GJ$An$BGC0e$X0E0F$X/T@f@h$A1GJ@f0l$Xn$PU
map g IL
map I KMYNOQNOSkRTV
map J /^0[^t]*$
map X x
map P p
map U L
map A "fyl
map B "hyl
map C "fp
map e "fy2l
map E "hp
map F "hy2l
map K 1Go
map M dG
map N yy
map O p
map q tllD
map Y o0123456789Z0q
map Q 0iT
map R $rn
map S $r$
map T ko00
map V Go/

"
" These are some example vi macros.
" The simplicity and usefulness of these examples make
" them good examples for vi beginners. I hope these
" macros would make life worth living once more for lots
" of vi beginners. 
" 
" You can either include the "jl.ex" file into your 
" vi init file ".exrc" or use the vi command ":source jl.ex"
" to load them during your vi session.
" 
" Comment and suggestions are welcomed.
" 
"              - Joel Loo Peing Ling, Mar 14, 89
"
"
"Some vi macros
"Joel Loo, Feb 15 1989
map V :s$/\* \(.*\) \*/$\1$
map v :s$.*$/* & */$
map W mt`m"dy`t
map  mt`m"dd`t
map Y "dPmm
map K :'m,.s/.*/# &/
map  :'m,.s/# \(.*\)/\1/
map  :e#
abbr cmt /* */hhi
abbr dcm /*
abbr ilv I love vi
map ip oprintf("  >\n");4hi

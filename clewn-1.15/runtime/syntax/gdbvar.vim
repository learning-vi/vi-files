" Vim syntax file
" Language:	gdb variables window syntax file
" Maintainer:	<xdegaye at users dot sourceforge dot net>
" Last Change:	Ap 29 2004

if exists("b:current_syntax")
    finish
endif

syn region gdbVarChged display contained matchgroup=gdbIgnore start="={\*}"ms=s+1 end="$"
syn region gdbDeScoped display contained matchgroup=gdbIgnore start="={-}"ms=s+1 end="$"
syn region gdbVarUnChged display contained matchgroup=gdbIgnore start="={=}"ms=s+1 end="$"

syn match gdbItem display transparent "^.*$"
    \ contains=gdbVarUnChged,gdbDeScoped,gdbVarChged,gdbVarNum

syn match gdbVarNum display contained "^\s*\d\+:"he=e-1

high def link gdbVarChged   Special
high def link gdbDeScoped   Comment
high def link gdbVarNum	    Identifier
high def link gdbIgnore	    Ignore

let b:current_syntax = "gdbvar"


" Clewn run time file
" Maintainer:	<xdegaye at users dot sourceforge dot net>
" Last Change:	12 Nov 2006
"
" Setup VIM to be used with Clewn and NetBeans
"
if exists("b:did_clewn")
    finish
endif
let b:did_clewn = 1

" source key mappings - they are not needed anymore starting
" with vim 7.0
if v:version < 700
    run macros/clewn_mappings.vim
endif

" enable balloon_eval for some key mappings
if has("balloon_eval")
    set ballooneval
    set balloondelay=100
endif


augroup clewn
autocmd!
" set options for gdb-variables.gdbvar
autocmd BufWinEnter *.gdbvar silent! set bufhidden=hide | set buftype=nowrite
						    \ | set filetype=gdbvar
						    \ | setlocal expandtab
						    \ | setlocal wrap
" set options for asm buffers
autocmd BufWinEnter *.clasm silent! setlocal autoread

" Warning message when using a project file
autocmd VimLeavePre * if exists("clewn_project")
	\ | echohl ErrorMsg
	\ | echo "The breakpoints are not saved to the project file,"
	\ | echo "when you quit from Vim instead of quitting from Clewn."
	\ | echo " "
	\ | exe input("It is safe to hit return when Clewn is closing or is terminated.")
	\ | echohl None
	\ | endif
augroup END


let s:clewn_tmpfile=tempname() . ".clewn-watched-vars.gdbvar"

" call this function to edit the watched
" variables window
" ONLY when doing remote debugging (option -x)
function EditWatchedVariables()
    edit `=s:clewn_tmpfile`
endfun

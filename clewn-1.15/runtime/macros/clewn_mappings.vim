" Mappings example for use with Clewn
" Maintainer:	<xdegaye at users dot sourceforge dot net>
" Last Change:	21 Oct 2006

nmap <F7> :call <SID>Toggle()<CR>

" Map keys with Vim version 7.0 or above
function! s:Map_vim_70()
    nmap <silent> <C-Z> :nbkey C-Z<CR>
    nmap <silent> B :nbkey S-B<CR>
    nmap <silent> L :nbkey S-L<CR>
    nmap <silent> A :nbkey S-A<CR>
    nmap <silent> S :nbkey S-S<CR>
    nmap <silent> I :nbkey S-I<CR>
    nmap <silent> <C-N> :nbkey C-N<CR>
    nmap <silent> X :nbkey S-X<CR>
    nmap <silent> F :nbkey S-F<CR>
    nmap <silent> R :nbkey S-R<CR>
    nmap <silent> Q :nbkey S-Q<CR>
    nmap <silent> C :nbkey S-C<CR>
    nmap <silent> W :nbkey S-W<CR>
    nmap <silent> <C-U> :nbkey C-U<CR>
    nmap <silent> <C-D> :nbkey C-D<CR>

    " set/clear bp at current line
    nmap <silent> <C-B> :nbkey C-B<CR>
    nmap <silent> <C-E> :nbkey C-E<CR>

    " set/clear bp at asm address shown by mouse position
    nmap <silent> <C-K> :nbkey C-K<CR>
    nmap <silent> <C-H> :nbkey C-H<CR>

    " print value at mouse position
    nmap <silent> <C-P> :nbkey C-P<CR>

    " print value referenced by word at mouse position
    nmap <silent> <C-X> :nbkey C-X<CR>

    " add selected variable at mouse position to variables
    nnoremap <silent> <C-J> :nbkey C-J<CR>
endfunction

" Map keys with Vim version older than 7.0
function! s:Map_vim_old()
    nmap <silent> <C-Z> <F21>z
    nmap <silent> B <F21>B
    nmap <silent> L <F21>L
    nmap <silent> A <F21>A
    nmap <silent> S <F21>S
    nmap <silent> I <F21>I
    nmap <silent> <C-N> <F21>n
    nmap <silent> X <F21>N
    nmap <silent> F <F21>F
    nmap <silent> R <F21>R
    nmap <silent> Q <F21>Q
    nmap <silent> C <F21>C
    nmap <silent> W <F21>W
    nmap <silent> <C-U> <F21>u
    nmap <silent> <C-D> <F21>d

    " set/clear bp at current line
    nmap <silent> <C-B> <F21>b
    nmap <silent> <C-E> <F21>e

    " set/clear bp at asm address shown by mouse position
    nmap <silent> <C-K> <F21>k
    nmap <silent> <C-H> <F21>h

    " print value at mouse position
    nmap <silent> <C-P> <F21>p

    " print value referenced by word at mouse position
    nmap <silent> <C-X> <F21>a

    " add selected variable at mouse position to variables
    nnoremap <silent> <C-J> <F21>j
endfunction

" Toggle between vim default and clewn mappings
function! s:Toggle()
    if s:clewn_k
	let s:clewn_k = 0

	" Map the keys
	if v:version < 700
	    call s:Map_vim_old()
	else
	    call s:Map_vim_70()
	endif

	echohl ErrorMsg
	echo "clewn keys mapped"
	echohl None

    " Restore vim defaults
    else
	let s:clewn_k = 1

	nunmap <C-Z>
	nunmap B
	nunmap L
	nunmap A
	nunmap S
	nunmap I
	nunmap <C-N>
	nunmap X
	nunmap F
	nunmap R
	nunmap Q
	nunmap C
	nunmap W
	nunmap <C-U>
	nunmap <C-D>

	nunmap <C-B>
	nunmap <C-E>
	nunmap <C-K>
	nunmap <C-H>
	nunmap <C-P>
	nunmap <C-X>
	nunmap <C-J>

	echohl ErrorMsg
	echo "clewn keys reset to default"
	echohl None
    endif
endfunction

" Do map clewn keys
let s:clewn_k = 1
if v:version < 700
    call s:Toggle()
else
    let s:clewn_k = 0
endif


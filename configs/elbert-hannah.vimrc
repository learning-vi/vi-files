syntax on

set number
set autoindent
set ignorecase
set smartcase
set incsearch
set shiftwidth=1
set expandtab
" set relativenumber

map q :q<cr>
map Q :q!<cr>
nnoremap ' `

colorscheme darkblue

" set foldcolumn=2
"set foldmethod=indent
"set foldlevelstart=99
set js 
" set mouse=a
set terse sw=1 ai ic wm=15 showmatch nows ruler exrc wc=<Tab> more
set history=200 laststatus=2 report=1 tildeop ttyfast
set novisualbell
set nrformats="hex"
set dictionary=/usr/dict/words,~/.mydict
set statusline=%<%t%h%m%r\ \ %a\ %{strftime(\"%c\")}%=0x%B\ \ line:%l,\ \ col:%c%V\ %P
set scrolloff=3
set smartcase
set cpt+=k
set confirm
set nohls
set hl+=l:IncSearch
set restorescreen
set wrap
set sidescroll=5
set listchars+=tab:>-,eol:$,precedes:<,extends:+,trail:~
set wildmenu
" set backup
set backupdir=~/tmp/.vimbackups
" set statusline=%{localtime()}
" set statusline=%<%f%h%m%r\ \ %a%=0x%B\ \ line:%l,\ \ col:%c%V\ %P
set t_me=[0;1;36m     " normal mode (undoes t_mr and t_md)
set t_mr=[0;1;33;44m  " reverse (invert) mode
set t_md=[1;33;31m    " bold mode
set t_se=[1;36;40m    " standout end
set t_so=[1;32;45m    " standout mode
set t_ue=[0;1;36m     " underline end
set t_us=[1;32m       " underline mode start
if has("terminfo")
  set t_Co=16
  set t_AB=[%?%p1%{8}%<%t%p1%{40}%+%e%p1%{92}%+%;%dm
  set t_AF=[%?%p1%{8}%<%t%p1%{30}%+%e%p1%{82}%+%;%dm
else
  set t_Co=16
  set t_Sf=[3%dm
  set t_Sb=[4%dm
endif
set viminfo='50
set hls incsearch hl=l:StatusLine
set incsearch
set ww=bshl
map  
map!  
map q :q
imap <tab> 
map Q :q!
" map g 1G
map _ :resize -1
map + :resize +1
inoremap ^] ^X^]
inoremap ^F ^X^F
inoremap ^D ^X^D
inoremap ^L ^X^L
nnoremap :: q:
nnoremap // q/
nnoremap ?? q?
" map v 
abbrev Win95 LOSE95
abbrev W95 LOSE95
abbrev WIN95 LOSE95
abbrev elh Elbert Hannah450 110th Ave NE, rm 426Bellevue WA 98004(425)451-5408
abbrev ELH Elbert Hannah MTSU S West Communications450 110th Ave NE, rm 426Bellevue WA 98004(425)451-5408
abbrev hte the
abbrev bbb blah, blah, blah,
abbrev yyy yadda, yadda, yadda,
abbrev adn and
abbrev htis this
abbrev teh the
abbrev htat that
abbrev taht that
abbrev waht what
abbrev f---ed not acceptable
" syntax on
if &t_Co > 1
  syntax enable
endif
syntax sync minlines=2000
" source $VIM/vim56/syntax/syntax.vim
" source $VIM/syntax/syntax.vim
au BufReadPost * if line("'\"") | exe "normal '\"" | endif
match Boolean '\(prod\|newrel\)\(\.[[:alnum:]_]*\)\{4}\.[[:alnum:]_]\{1,}'

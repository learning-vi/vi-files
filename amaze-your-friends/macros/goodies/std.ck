" std.ck -- standard Command mode Keys
"
" goto-end-of-line
map q $
" goto-first-line-of-buffer
map g 1G
" toggle-case-of-current-word 
map = b~
" Visit-Vi-Package.  The user has to complete or correct the path.
map V :so /usr/local/lib/vi/
" vhere-is-the-previous-occurence-of-the-following-word?
map v dePOpI?"xdd@x
" adjust-current-line-near-the-top
map  z
" write-file-suspend-vi
map  :write:stop

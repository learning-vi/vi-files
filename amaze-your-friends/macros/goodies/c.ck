" c.ck -- c-mode command-keys
"
" goto the next/prev. statement delimiter
map ( ?[{}();,]
map ) /[{}();,]
" vhere-is-the-declaration-of-the-following-local-identifier
map v dePOpI/\<A\>"xdd[[@x
map @ :so /usr/local/lib/vi/std.mc:so /usr/local/lib/vi/c.mc:unmap @

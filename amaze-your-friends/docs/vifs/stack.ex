"
" edit-file stack ( #+:push, #-:pop, #=:list, #:pop, #:push & tag )
"
map	#+	:1,.w !exec vifs push %
map	#-	:!exec vifs pop
map	#=	:!exec vifs
map	#	:1,.w !exec vifs push %
map	#	:!exec vifs pop
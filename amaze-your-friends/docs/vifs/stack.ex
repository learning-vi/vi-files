"
" edit-file stack ( #+:push, #-:pop, #=:list, #:pop, #:push & tag )
"
map	#+	:1,.w !exec vifs push %
map	#-	:!exec vifs pop:so ~/tmp/vifs_pop.ex
map	#=	:!exec vifs
map	#	:1,.w !exec vifs push %
map	#	:!exec vifs pop:so ~/tmp/vifs_pop.ex

" c.mc -- c-mode macro loading
"
"	@b	C-beautify function
"	@c	cchk file
"	@f	show function declarations
"	@g	get the {{body structure of} this function}
"	@l	lint file
"	@m	make
"	@s	stop (suspend vi)
"	@t	toplevel-Definitionen zeigen
"	@u	up: Kommentar --> Code
"	@w	write&stop (save buffer & suspend vi)
"	@y	down: Code --> Kommentar
"
read /usr/local/lib/vi/c.ms
d b
d c
d f
d g
.,+d l
.,+d m
d s
d t
d u
.,+d w
d y

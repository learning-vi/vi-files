" std.mc -- standard Macro Calls: load macros into named buffers
"
"	@s	stop (suspend vi)
"	@w	write&stop (save buffer & suspend vi)
"
" std.mc and std.ms have to stay 'in tune' !!!
read /usr/local/lib/vi/std.ms
d s
.,+d w

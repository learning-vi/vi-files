[[!%cb
1G!Gcchk
:g/^[a-zA-Z].*)$/p
:g/[{}]/p
[[:.,/^}/g/[{}]/p
:w! /tmp/Extmp
1GO!!lint /tmp/Extmp
:w!
1GO!!make 
:stop
:g/^[#a-zA-Z}]/p
^3x$xxx
:w!
:stop
I/* A */

" From: smr@philips.oz.au (Stephen Riehm)
" Subject: Vi macros: share and enjoy
" Date: 13 Mar 92 14:52:49 GMT
" 
" I got so annoyed with not being able to do the following that I went
" out and made the following vi macros. I don't give any guarantee that
" they will work, but they work on my version of vi ( Hp9000-700 )
" 
" --- cut here --- file: ~/.exrc
:map! @! @Bi:r! F:"adt@mm@ais"bd$dd`m@b
:map! @@ @F@xi:r! F:"adt@mm@ais"bd$dd`m@b
" --- cut here ---
" note: all ^'s are ctrl's in these macros!
"
" ### CTRL's expanded by the archivemaintainer.
" 
" description:
" If you want to include the output from a command in the line you are
" currently editing ( for example to get the current path or date ) you
" can use these two macros in the following way:
" 
" I want to see the date here -> date@!
" 
" which will expand to something like:
" 
" I want to see the date here -> Sat Mar 14 00:44:00 EST 1992
" 
" or for more complicated commands that need arguments:
" 
" the number of words in the file is '@wc -w %@@'
" 
" which will expand to something like:
" 
" the number of words in the file is '8 /usr/tmp/nn.a29287'
" 
" and as it leaves you in insert mode, you can continue typing.
" 
" Hope you find these useful ( of course vi should be able to take
" commands like ~w and !w etc but thats a different issue )
" 
" catchya
" 
" -----------------------------------------------------------------
" Stephen Riehm					smr@philips.oz.au
" Philips Public Telecommunications Systems    Phone: +61 3 8813666
" Melbourne, Australia      		       Fax: +61 3 8813577

"  Authors : Krishna Kumar Gadepalli ( gadepall@cs.unc.edu )
"            Suresh Balu             ( balu@cs.unc.edu )
"            Srikanth Subramanian    ( subraman@uoftcse.cse.utoledo.edu )
"
"  filename : bounce.vi.macros
"
"             This file defines macros to simulate a bouncing ball
"             To test it source this file ( :so bounce.vi.macros )
"             in one of the given txt1, txt2, .. files ( or one of
"             your own, as long as it is in the required format )
"             and hit 'g' at any space character within the region
"             drawn in the file.
"
"  P.S : This can be established by a set of smaller macros,
"        but it might not be as fast as this. If you notice 
"        the macros are very similar and can expanded to add
"        more features.
"
"  The idea behind this is as follows :
"  We first establish a state table ( idea from macros for maze )
"  
"  The table is as follows prev_state X cur_char => new_state
"  The top row shows the set of valid characters which form the region
"  the outermost boundary being closed. Some arbitrary assumptions
"  have been made, such as when the ball hits a '-' while moving left
"  will start moving west ( please see defn of N, S, W, E directions 
"  below, in the mappings ).
"
"  Each state while making a move will write the direction in which
"  it is moving in the last line and then replace the current char
"  by what it was before ( each state will know it ) and then move
"  to the new postion and yank that char and write in the last line
"  before the direction and then replaces it with 'o' - the ball.
"  The next move is made by yanking what has been written in the last
"  line by the previous state and executing it.
"
"  The  following state table is easily extendable for arbitrary angles
"  and new characters.
"
"      | ' ' | '-' | '!' | '\' | '/' |
"   ___|_____|_____|_____|_____|_____|
"      |     |     |     |     |     |
"    N |  N  |  E  |  W  |  S  |  T  |
"   ___|_____|_____|_____|_____|_____|
"      |     |     |     |     |     |
"    S |  S  |  W  |  E  |  N  |  D  |
"   ___|_____|_____|_____|_____|_____|
"      |     |     |     |     |     |
"    E |  E  |  N  |  S  |  W  |  R  |
"   ___|_____|_____|_____|_____|_____|
"      |     |     |     |     |     |
"    W |  W  |  S  |  N  |  E  |  L  |
"   ___|_____|_____|_____|_____|_____|
"      |     |     |     |     |     |
"    T |  T  |  D  |  N  |  R  |  L  |
"   ___|_____|_____|_____|_____|_____|
"      |     |     |     |     |     |
"    D |  D  |  T  |  S  |  L  |  R  |
"   ___|_____|_____|_____|_____|_____|
"      |     |     |     |     |     |
"    R |  R  |  E  |  L  |  T  |  D  |
"   ___|_____|_____|_____|_____|_____|
"      |     |     |     |     |     |
"    L |  L  |  W  |  R  |  T  |  D  |
"   ___|_____|_____|_____|_____|_____|
"
" sample command :
" map -N   r-jltmaGvaE`aromaG0P0D`a@z
"     - : current char ( the char which will be left in the cur pos
"                        while making a move )
"     N : direction in which the ball was moving so far
"    r- : replace with '-'
"    t  : yank the char
"    maGv : mark position and go and put the yanked char 
"            in the last line
"    aE`a : add the new direction to the cur_char and go back to
"             current ball position
"    ro : replace with the ball
"    maGOP0D : go to the last line yank the command and delete it.
"    `a@z    : go back to ball postion and execute the yanked command
"
"   P.S : It is possible to reduce the number of mappings by using a
"         state table at the bottom of the file. But that method SEEMS
"         to be much slower than this one.
"         
"         The common parts in each macro also need not be repeated.
"         But these can easil;y be changed.
"
"   The actual mappings :
set remap
set edcompatible
"
" it MAY be necessary to unmap some characters which you might
" have mapped in your .exrc  or remove some of the following lines
" if you have not mapped the following characters.
"unmap g
"unmap -
"unmap Y
"
" the following three mapping are to get around vi's 'can't 
" yank inside global macro' and 'can't put partial line in a
" global macro' problems.
map t "zyl
map P :g/../y z
map v "zp
"
" North : kl
map  N r kltmaGvaN`aromaG0P0D`a@z
map -N   r-jltmaGvaE`aromaG0P0D`a@z
map !N   r!khtmaGvaW`aromaG0P0D`a@z
map \N   r\jhtmaGvaS`aromaG0P0D`a@z
map /N   r/ktmaGvaT`aromaG0P0D`a@z
"
" South : jh
map  S r jhtmaGvaS`aromaG0P0D`a@z
map -S   r-khtmaGvaW`aromaG0P0D`a@z
map !S   r!jltmaGvaE`aromaG0P0D`a@z
map \S   r\kltmaGvaN`aromaG0P0D`a@z
map /S   r/jtmaGvaD`aromaG0P0D`a@z
"
" East : jl
map  E r jltmaGvaE`aromaG0P0D`a@z
map -E   r-kltmaGvaN`aromaG0P0D`a@z
map !E   r!jhtmaGvaS`aromaG0P0D`a@z
map /E   r/khtmaGvaW`aromaG0P0D`a@z
map \E   r\ltmaGvaR`aromaG0P0D`a@z
"
" West : kh
map  W r khtmaGvaW`aromaG0P0D`a@z
map -W   r-jhtmaGvaS`aromaG0P0D`a@z
map !W   r!kltmaGvaN`aromaG0P0D`a@z
map /W   r/jltmaGvaE`aromaG0P0D`a@z
map \W   r\htmaGvaL`aromaG0P0D`a@z
"
" Top : k
map  T r ktmaGvaT`aromaG0P0D`a@z
map -T   r-jtmaGvaD`aromaG0P0D`a@z
map !T   r!kltmaGvaN`aromaG0P0D`a@z
map /T   r/ltmaGvaR`aromaG0P0D`a@z
map \T   r\htmaGvaL`aromaG0P0D`a@z
"
" Down : j
map  D r jtmaGvaD`aromaG0P0D`a@z
map -D   r-ktmaGvaT`aromaG0P0D`a@z
map !D   r!jhtmaGvaS`aromaG0P0D`a@z
map /D   r/htmaGvaL`aromaG0P0D`a@z
map \D   r\ltmaGvaR`aromaG0P0D`a@z
"
" Right : l
map  R r ltmaGvaR`aromaG0P0D`a@z
map -R   r-jltmaGvaE`aromaG0P0D`a@z
map !R   r!htmaGvaL`aromaG0P0D`a@z
map /R   r/ktmaGvaT`aromaG0P0D`a@z
map \R   r\jtmaGvaD`aromaG0P0D`a@z
"
" Left : h
map  L r htmaGvaL`aromaG0P0D`a@z
map -L   r-khtmaGvaW`aromaG0P0D`a@z
map !L   r!ltmaGvaR`aromaG0P0D`a@z
map /L   r/jtmaGvaD`aromaG0P0D`a@z
map \L   r\ktmaGvaT`aromaG0P0D`a@z
"
" arbitrarily chose the eastern direction to move assuming that
" current character is a space.
map g  E

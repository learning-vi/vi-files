Some remarks regarding the c-mode-package:

0. The files are organized like std-mode.  To see what is done in
   c-mode, look into files.

1. You can use c.top in your environment variable EXINIT to startup VI
   in c-mode:   sentenv EXINIT 'so /usr/local/lib/vi/c.top'

2. If you are in std-mode, you can add the c-mode by 'V'ing
   /usr/local/lib/vi/c

3. Look at c.ik to see how a key can be made context dependend.  If
   you prefer a different indentation style than me, you will have to
   make same changes in this file, too. 

4. Problem:  Some key mappings from std-mode are redefined in c-mode.
   I would assume that one has to source std-files first, then
   c-mode-files.  However, I had to source them in the opposite order.
   ???

5. Beside c-mode, I also have a text-mode.  C-mode is definitely not
   a 'general purpose mode'.  I won't post the text-mode, because it
   is to language dependent, and you surely can adapt a package like
   c-mode to your own, new modes.

6. One file, c.cab, is another approach to a c-mode.  I am not the
   author of this file, and must admit that I even forgot who it
   actually is.  You can source this file and ignore my package.

7. "vtkeys" and "fkeys" are not part of the c-mode, but I think, the
   few extra bytes won't hurt anybody.  Actually, using the backquotes
   for ESC is really one of our favourites here.

				have fun,
				Martin Neitzel
				...uunet!mcvax!unido!infbs!neitzel

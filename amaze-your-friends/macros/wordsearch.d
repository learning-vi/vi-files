From alf.uib.no!ugle.unit.no!nuug!ulrik!ifi.uio.no!kth.se!eru!bloom-beacon!gatech!ncar!elroy.jpl.nasa.gov!swrinde!cs.utexas.edu!uunet!mcsun!ukc!pyrltd!root44!gwc Wed Sep  4 13:43:57 MET DST 1991
Article: 2935 of comp.editors
Path: alf.uib.no!ugle.unit.no!nuug!ulrik!ifi.uio.no!kth.se!eru!bloom-beacon!gatech!ncar!elroy.jpl.nasa.gov!swrinde!cs.utexas.edu!uunet!mcsun!ukc!pyrltd!root44!gwc
From: gwc@root.co.uk (Geoff Clare)
Newsgroups: comp.editors
Subject: Re: Wanted: "search-for-word-under-cursor" command
Keywords: search cursor vi
Message-ID: <3029@root44.co.uk>
Date: 3 Sep 91 13:29:33 GMT
References: <24735@rdmei.isl.mei.co.jp> <1991Sep2.093311.10670@world.std.com>
Distribution: comp.editors
Organization: UniSoft Ltd., London, England
Lines: 20
Status: R

ekw@world.std.com (Elliott C Winslow) writes:

>I use the following:

>map  [z  "zye o/^[   "zp       "zdd0 @z
>map  [Z  "zye o/\<^[ "zp A\>^[ "zdd0 @z
>map  \z  "zye :e ztmp^[ go/^[  "zp      "zyy @z :e! #^[jn
>map  \Z  "zye :e ztmp^[ go/\<^["zpA\>^[ "zyy @z :e! #^[jn

>where '^[' is an escaped escape.

The problem with using "ye" is that on most versions of vi it leaves off
the last letter of the yanked word.  Presumably Elliott has a version
which doesn't have this feature (bug?), or [Z and \Z wouldn't work for
him.  I get round the problem by using "deu" in place of "ye" in these
situations, since "de" does delete right to the end of the word on all
versions of vi.
-- 
Geoff Clare <gwc@root.co.uk>  (USA UUCP-only mailers: ...!uunet!root.co.uk!gwc)
UniSoft Limited, London, England.   Tel: +44 71 729 3773   Fax: +44 71 729 3273


From alf.uib.no!ugle.unit.no!sunic!psinntp!uunet!world!ekw Tue Sep 10 19:46:06 MET DST 1991
Article: 3016 of comp.editors
Newsgroups: comp.editors
Path: alf.uib.no!ugle.unit.no!sunic!psinntp!uunet!world!ekw
From: ekw@world.std.com (Elliott C Winslow)
Subject: Re: Wanted: "search-for-word-under-cursor" command
Message-ID: <1991Sep2.093311.10670@world.std.com>
Summary: one changes the file, the other uses a tmpfile
Keywords: search cursor vi
Sender: Elliott Winslow
Organization: The World @ Software Tool & Die
References: <24735@rdmei.isl.mei.co.jp>
Distribution: comp.editors
Date: Mon, 2 Sep 1991 09:33:11 GMT
Lines: 27

In article <24735@rdmei.isl.mei.co.jp> ben@cit3sun1.it3.crl.mei.co.jp (Ben Reaves) writes:
>Dear Readers,
>
>I am looking for a command like ^] but the word under the cursor is not a
>function name - it is a variable name.  

I use the following:

map  [z  "zye o/^[   "zp       "zdd0 @z
map  [Z  "zye o/\<^[ "zp A\>^[ "zdd0 @z
map  \z  "zye :e ztmp^[ go/^[  "zp      "zyy @z :e! #^[jn
map  \Z  "zye :e ztmp^[ go/\<^["zpA\>^[ "zyy @z :e! #^[jn

where '^[' is an escaped escape.

The [ ones create the macro on the next line, then delete it (to the
z buffer), so the current file *gets* changed.
The \ ones write to a tmpfile (assume that current file *hasn't* been
changed, or they will fail).
The z ones search for the string as a reg exp.
The Z ones search for the string as a discrete word.

They operate from the next line on.

---
  Elliott Winslow IM  {uunet,xylogic}!world.std.com!ekw
  (718) 429-5793 {apple,pacbell,hplabs,ucbvax}!well!ekw




START	{a:a:L:START}    {b:b:L:START}    {c:c:L:START}    {####:####:R:kill_a}
	{a':a':L:START}  {b':b':L:START}  {c':c':L:START}
kill_a	{a:a':R:kill_b}  {b:NO:N:NO}       {c:NO:N:NO}       {%%%%:YES:N:YES}
	{a':a':R:kill_a} {b':b':R:kill_a} {c':c':R:kill_a}
kill_b	{a:a:R:kill_b}   {b:b':R:kill_c}  {c:NO:N:NO}       {%%%%:NO:N:NO}
	{a':NO:N:NO}     {b':b':R:kill_b} {c':NO:N:NO}
kill_c	{a:NO:N:NO}       {b:b:R:kill_c}   {c:c':L:START}   {%%%%:NO:N:NO}
	{a':NO:N:NO}     {b':NO:N:NO}     {c':c':R:kill_c}

####
a
a
a
b
b
b
c
c
c
%%%%

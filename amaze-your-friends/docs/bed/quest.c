extern char	*getpar ();

quest (prompt)
	char	*prompt;
	{ char	*reply;
	  return ((reply = getpar (prompt)) ? ((*reply | 040) == 'y') : 0);
	}

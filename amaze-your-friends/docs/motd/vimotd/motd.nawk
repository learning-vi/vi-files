#
# motd.nawk  -	Parse global motd file format from stdin and
# 		generate individual motd's, one per host.
#		By Paul Balyoz <pab@rainbow.cse.nau.edu>
#
#	This program writes to files in the current directory,
#      	so make sure that you "cd" to the right directory
#	before running nawk on this file.
#
#	Nawk was used in place of awk for two reasons.  First, a bug was
#	encountered in awk under SunOS 4.1.1 that was not easy to work around.
#	Second, awk is more limited in the number of current open files
#	than nawk is; even with nawk, the limit is about 25 files.
#	This means you cannot use this package with more than 25 hosts.
#
BEGIN {
	#
	# Enter all host names here.  These do not have to be the canonical
	# hostname.  They must be accessable via NFS, using the names here
	# as part of the NFS path:
	#
	hosts["mv1"] = "mv1";
	hosts["mv2"] = "mv2";
	hosts["mv3"] = "mv3";
	hosts["rai"] = "rai";
	hosts["aga"] = "aga";
	hosts["hum"] = "hum";
	hosts["eld"] = "eld";
	hosts["fre"] = "fre";
	hosts["tur"] = "tur";

	#
	# Aliases for the hosts listed above:
	#
	hosts["mvaxcs1"] = "mv1";
	hosts["mvaxcs2"] = "mv2";
	hosts["mvaxcs3"] = "mv3";
	hosts["naucse"] = "mv3";
	hosts["rainbow"] = "rai";
	hosts["rain"] = "rai";
	hosts["agassiz"] = "aga";
	hosts["humphreys"] = "hum";
	hosts["elden"] = "eld";
	hosts["fremont"] = "fre";
	hosts["turing"] = "tur";

	#
	# Multi-host abbreviations:
	#
	hosts["all"] = "mv1 mv2 mv3 rai aga hum eld fre tur";
	hosts["rahef"] = "rai aga hum eld fre";
	hosts["ahef"] = "aga hum eld fre";
	hosts["123"] = "mv1 mv2 mv3";
	hosts["12"] = "mv1 mv2";
	hosts["mv*"] = "mv1 mv2 mv3";

	#
	# Extension appended to each temporary motd file:
	#
	filesuffix = ".text";

	#
	# Don't change anything below this line.  ----------------------------
	#
	err = 0;
}

#
# Ignore all comment lines.
#
/^#/ {
	next;
}

#
# Special Token at beginning of a line: set up the file list.
#
/^>>/ {
	#
	# Accumulate host aliases in filestr and reduce to host names.
	#
	filestr = "";
	len = split ($1, temp, ">");
	for (i in temp)
		if (temp[i] != "")
			filestr = filestr " " temp[i];
	for (i=2; i<=NF; i++)
		filestr = filestr " " $i;

	#
	# Accumulate ACTUAL host names in filestr then break them out.
	#
	len = split (filestr, file);
	filestr = "";
	for (i in file) {
		if (hosts[file[i]] != "")
			filestr = filestr " " hosts[file[i]];
		else {
			print "host \"" file[i] "\" unknown";
			err = 1;
		}
	}
	len = split (filestr, file);

	#
	# Add a blank line before the next motd entry is printed.
	#
	if (len > 0)
		for (i in file) {
			filename = file[i] filesuffix;
			print "" >> filename;
		}

	next;
}

#
# All other lines are part of one motd entry, and are appended to
# each host motd file.
#
{
	if (len > 0)
		for (i in file) {
			filename = file[i] filesuffix;
			print $0 >> filename;
		}
}

#
# Exit nawk with the proper error condition.
#
END {
	exit err;
}

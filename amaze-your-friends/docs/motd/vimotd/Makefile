#
#	Makefile for installing vimotd package.
#
#	By Paul Balyoz <pab@rainbow.cse.nau.edu>
#			02/08/92
#

# Where to put the vimotd executable:
BIN=/usr/local/etc

# Where vimotd's working directory goes:  (remember, modify vimotd, too!)
MOTDDIR=/usr/local/lib/motd

all:
	@echo 'Nothing to make, see Makefile for installation.'

#
# Install the executable shell script "vimotd":
#
install:
	install -c -m 750 vimotd $(BIN)

#
# Install vimotd's working directory and files:
#
installdir:
	-mkdir $(MOTDDIR)
	install -c -m 640 motd $(MOTDDIR)
	install -c -m 640 motd.nawk $(MOTDDIR)
	pwd > $(MOTDDIR)/SOURCE

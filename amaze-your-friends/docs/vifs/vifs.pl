#!/usr/local/bin/perl
#
# vifs : vi edit-file stack
#	usage: vifs [show|push file|pop|clear|clean]
#
$PPID = getppid;
$HOME = $ENV{'HOME'};
$STACKFILEBASE = $HOME . '/tmp/#vifs_stack';
$STACKFILE = $STACKFILEBASE . ".$PPID";
$POPEXFILE = $HOME . '/tmp/vifs_pop.ex';

if ($ARGV[0] eq 'push' && $#ARGV == 1) {
    &countline;
    &getstack;
    push(@stack, "$ARGV[1] $line\n");
    &putstack;
} elsif ($ARGV[0] eq 'show' || $#ARGV == -1) {
    &getstack;
    print @stack;
} elsif ($ARGV[0] eq 'pop') {
    &getstack;
    $_ = pop(@stack);
    chop;
    ($file, $line) = split(' ');
    &makepopex;
    &putstack;
} elsif ($ARGV[0] eq 'clear') {
    truncate($STACKFILE, 0);
} elsif ($ARGV[0] eq 'clean') {
    unlink <$STACKFILEBASE.*>, $POPEXFILE;
} else {
    &usage;
}
exit 0;

sub usage {
    print STDERR "usage: vifs [show|push file|pop|clear|clean]\n";
    exit 1;
}

sub countline {
    while (<STDIN>) {}
    $line = $.;
}

sub getstack {
    if (open(STACK, "<$STACKFILE")) {
	@stack = <STACK>;
	close(STACK);
    }
}

sub putstack {
    open(STACK, ">$STACKFILE") || die "can not open $STACKFILE\n";
    print STACK @stack;
    close(STACK);
}

sub makepopex {
    open(POPEX, ">$POPEXFILE") || die "can not open $POPEXFILE\n";
    if ($file && $line) {
	print POPEX "e $file\n$line\n";
    } else {
	print STDERR "edit-file stack empty\n";
	print POPEX qq/" edit-file stack empty\n/;
    }
    close(POPEX);
}

#!/usr/bin/perl

$recipients = "sctp-coders\@lakerest.net\n";

$login = (getpwuid($<))[0] || "nobody";
$fullname = (getpwuid($<))[6] || "nobody";

$hostname = "stewart.chicago.il.us";

# force the envelope sender to an existing hostname
$sendmail = "/usr/sbin/sendmail -oi -t -om";


#
# turn off setgid
#
$) = $(;

#
# parse command line arguments
#
@files = split(/ /,$ARGV[0]);
$logfile = $ARGV[1];
$cvsroot = $ENV{'CVSROOT'};
#
# Some date and time arrays
#
@mos = (January,February,March,April,May,June,July,August,September,
        October,November,December);
@days = (Sunday,Monday,Tuesday,Wednesday,Thursday,Friday,Saturday);
($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime;

#
# open log file for appending
#
if ((open(OUT, ">>" . $logfile)) != 1) {
    die "Could not open logfile " . $logfile . "\n";
}

# 
# open mail pipe
#
  $PIPE = "|$sendmail";
  open (PIPE) || die "$0: couldn't open $PIPE: $!\n";
  printf PIPE "From: The Repository Clerk <daemon\@$hostname>\n";
  printf PIPE "To: $recipients \n";
  printf PIPE "Subject: Changes to 'SCTP KAME tree' repository\n\n";
  printf PIPE "CVSROOT=> stewart.chicago.il.us:/usr/sctpCVS/ \n\n";
  printf PIPE "Our team member " . $fullname . " just made the following changes:\n\n";

# 
# Header
# 
print OUT "\n";
print OUT "**************************************\n";
print OUT "date: " . $days[$wday] . ", " . $mos[$mon] . " " . $mday . ", 19" . $year .
    " @ " . $hour . ":" . sprintf("%02d", $min) . "\n";
print OUT "author: " . $login . "\n";


#
#print the stuff on stdin to the logfile
#
open(IN, "-");
while(<IN>) {
   print OUT $_;
   print PIPE $_;
}
close(IN);

print OUT "\n";

#
# after log information, do an 'cvs -Qn status' on each file in the arguments.
#
for $file (@files[1..$#files]) {
    if ($file eq "-") {
	last;
    }
    open(RCS,"-|") || exec 'cvs', '-Qn', 'status', $file;
    while (<RCS>) {
        if (substr($_, 0, 7) eq "    RCS") {
            print OUT;
            print PIPE;
        }
    }
    close (RCS);
}

close (OUT);
close (PIPE);

@rem = '--*-Perl-*--
@echo off
if "%OS%" == "Windows_NT" goto WinNT
perl -x -S "%0" %1 %2 %3 %4 %5 %6 %7 %8 %9
goto endofperl
:WinNT
perl -x -S %0 %*
if NOT "%COMSPEC%" == "%SystemRoot%\system32\cmd.exe" goto endofperl
if %errorlevel% == 9009 echo You do not have Perl in your PATH.
if errorlevel 1 goto script_failed_so_exit_with_non_zero_val 2>nul
goto endofperl
@rem ';
#!perl
#line 15
use strict;
use Time::localtime;


###########################################################
# Version

my $tm = localtime;
my $ver = shift || sprintf "internal %02d/%02d/%02d %02d:%02d", $tm->mday, $tm->mon+1, $tm->year-100, $tm->hour, $tm->min;
my $ver1 = $ver;
$ver1 =~ s/[\s\.]/_/g;
$ver1 =~ s/[\\\/\:]//g;

print "Building version $ver\n";

###########################################################
# Other vars

my $package = "..\\winampac3_$ver1";
my $bin = "..\\winampac3_$ver1.exe";
my $src = "winampac3_${ver1}_src.zip";

my $make_bin = "\"c:\\Devel\\NSIS\\makensis\" /NOCONFIG /DVERSION=\"$ver\" /DSETUP_FILE=$bin /DSOURCE_DIR=$package winampac3.nsi";
my $make_src = "pkzip25 -add -rec -dir -excl=CVS -excl=pic_psd -lev=9 $src winampac3\\*.* valib\\*.*";


###########################################################
# Some funcitons

sub read_file
{
  my ($filename) = (@_);
  open (FILE, "< $filename") || die "cannot open file $filename";
  my @result = <FILE>;
  close (FILE);
  return @result;
}

sub write_file
{
  my $filename = shift;
  open (FILE, "> $filename") || die "cannot open file $filename";
  print FILE @_;
  close (FILE);
}

sub set_ver
{
  my $ver = shift;
  open VER, "> winampac3_ver.h";
  printf VER <<EOVER;
#ifndef WINAMPAC3_VER_H
#define WINAMPAC3_VER_H

#define WINAMPAC3_VER "$ver"

#endif
EOVER
  close VER;
}

sub build_project
{
  my $dir = shift;
  my $project = shift;
  system("msdev $dir\\$project.dsp /MAKE \"$project - Win32 Release\" /REBUILD")
    && die "failed!!!";
  `_clear.bat`;
}



###########################################################
# Build projects

print "Building project...\n";

set_ver($ver);
build_project('.', 'winampac3');
`_clear.bat`;
set_ver("internal");



###############################################################################
# Build package

printf "Building package...\n";
`rmdir /s /q $package 2> nul`;
`mkdir $package`;

`copy release\\in_winampac3.dll $package`;

`copy _readme.txt $package`;
`copy _changes_eng.txt $package`;
`copy _changes_rus.txt $package`;
`copy GPL_eng.txt $package`;
`copy GPL_rus.txt $package`;

`copy winampac3_presets.reg $package`;


###############################################################################
## Make distributive

`del $bin 2> nul`;
`del $src 2> nul`;

system($make_bin);

chdir("..\\valib");
`_clear.bat`;
chdir("..");
system($make_src);

__END__
:endofperl

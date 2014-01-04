#!/usr/bin/perl

# Read the planetsplitter log file

open(SECTION   ,">gnuplot.section.tmp");
open(SUBSECTION,">gnuplot.subsection.tmp");

$count=1;
$startcount=0;
$totaltime=0;

while(<STDIN>)
  {
   s%\r*\n%%;

   next if(! $_);

   next if(m%^=%);

   if( m%^\[ *([0-9]+):([0-9.]+)\] ([^:]+)% && ! m%Complete$% )
     {
      $time=(60.0*$1)+$2;
      $description=$3;

      print SUBSECTION "$count $time \"$description\"\n";

      $totaltime+=$time;
     }
   else
     {
      if($startcount>0)
        {
         $boxcentre=($count+$startcount+0.5)/2;
         $boxwidth=$count-$startcount-1;

         print SECTION "$boxcentre $totaltime $boxwidth\n";
        }

      $startcount=$count-0.5;
      $totaltime=0;
     }

   $count++;
  }

close(SECTION);
close(SUBSECTION);

# Plot using gnuplot

open(GNUPLOT,"|gnuplot");

print GNUPLOT <<EOF

set title "Planetsplitter Execution Time"

set noxtics

set ylabel "Sub-section Time (seconds)"
set logscale y
set yrange [0.001:]

set style fill solid 1.0
set boxwidth 0.8

set nokey

set style line 1 lt rgb "#FFC0C0" lw 1
set style line 2 lt rgb "#FF0000" lw 1

set term png size 1000,750
set output "planetsplitter.png"

plot "gnuplot.section.tmp" using 1:2:3 with boxes linestyle 1, \\
     "gnuplot.section.tmp" using 1:(\$2*1.1):(sprintf("%.1f",\$2)) with labels font "Sans,9" center textcolor rgbcolor "#000000", \\
     "gnuplot.subsection.tmp" using 1:2 with boxes linestyle 2, \\
     "gnuplot.subsection.tmp" using (\$1+0.1):(0.0013):3 with labels font "Sans,8" left rotate textcolor rgbcolor "#000000"

exit
EOF
;

close(GNUPLOT);

unlink "gnuplot.section.tmp";
unlink "gnuplot.subsection.tmp";

                      Planetsplitter Execution Time Analysis
                      ======================================

A Perl script that uses Gnuplot to plot a graph of the time taken by the
planetsplitter program to run.


plot-planetsplitter-time.pl
---------------------------

Example usage:

planetsplitter --loggable --logtime ... > planetsplitter.log

plot-planetsplitter-time.pl < planetsplitter.log

This will generate a file called planetsplitter.png that contains the graph of
the execution time.

                                 ROUTINO EXTRAS
                                 ==============

This directory contains some programs and scripts that although distributed with
Routino are not necessary components of a working OSM router.  They are
generally either programs that use some components of Routino (i.e. they are
compiled and linked with some of the Routino source code) or they are scripts to
be used to process the outputs of Routino.

Each program or script has its own directory which contains all of the necessary
source code, documentation and/or web pages for that program or script.  None of
them will be installed when Routino is installed.

--------------------------------------------------------------------------------

tagmodifier - A program to read an OSM XML file and process it using a Routino
              tagging rules file to create a modified output XML file.

errorlog    - Scripts for processing the error log file (created by running
              planetsplitter with the --errorlog option).

plot-time   - Plots the output of 'planetsplitter --loggable --logtime' to show
              how long each part of the processing takes.

find-fixme -  A modified version of the Routino planetsplitter and filedumper
              programs to scan an OSM file for "fixme" tags and create a
              database so that web pages provided can display them.

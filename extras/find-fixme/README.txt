                            Find and Display FIXME tags
                            ===========================

The "fixme" tag is often used in OSM data to mark an item whose details are not
completely known - as a reminder or request for somebody to check it.  Since
Routino can now generate a map of tagging problems that it finds it is easy to
extend this to finding all "fixme" tags.  The files in this directory provide a
complete set of executables and web pages for extracting and displaying all
items with "fixme" tags on a map.

Editing fixme.xml and changing the rules for selecting tags allows for creating
custom databases to display items containing any desired tag(s).


fixme-finder
------------

This program is a modified version of the Routino planetsplitter program and can
be used on an OSM file to extract the fixme tags and generate a database of
them.


Usage: fixme-finder [--help]
                    [--dir=<dirname>]
                    [--sort-ram-size=<size>] [--sort-threads=<number>]
                    [--tmpdir=<dirname>]
                    [--tagging=<filename>]
                    [--loggable] [--logtime]
                    [<filename.osm> ...
                     | <filename.pbf> ...
                     | <filename.o5m> ...
                     | <filename.(osm|o5m).bz2> ...
                     | <filename.(osm|o5m).gz> ...]

--help                    Prints this information.

--dir=<dirname>           The directory containing the fixme database.

--sort-ram-size=<size>    The amount of RAM (in MB) to use for data sorting
                          (defaults to 256MB otherwise.)
--sort-threads=<number>   The number of threads to use for data sorting.

--tmpdir=<dirname>        The directory name for temporary files.
                          (defaults to the '--dir' option directory.)

--tagging=<filename>      The name of the XML file containing the tagging rules
                          (defaults to 'fixme.xml' with '--dir' option)

--loggable                Print progress messages suitable for logging to file.
--logtime                 Print the elapsed time for each processing step.

<filename.osm>, <filename.pbf>, <filename.o5m>
                          The name(s) of the file(s) to read and parse.
                          Filenames ending '.pbf' read as PBF, filenames ending
                          '.o5m' read as O5M, others as XML.
                          Filenames ending '.bz2' will be bzip2 uncompressed.
                          Filenames ending '.gz' will be gzip uncompressed.


fixme-dumper
------------

This program is a modified version of the Routino filedumper program and is used
by the web page CGI to display the information on a map.


Usage: fixme-dumper [--help]
                    [--dir=<dirname>]
                    [--statistics]
                    [--visualiser --latmin=<latmin> --latmax=<latmax>
                                  --lonmin=<lonmin> --lonmax=<lonmax>
                                  --data=<data-type>]
                    [--dump--visualiser [--data=fixme<number>]]

--help                    Prints this information.

--dir=<dirname>           The directory containing the fixme database.

--statistics              Print statistics about the fixme database.

--visualiser              Extract selected data from the fixme database:
  --latmin=<latmin>       * the minimum latitude (degrees N).
  --latmax=<latmax>       * the maximum latitude (degrees N).
  --lonmin=<lonmin>       * the minimum longitude (degrees E).
  --lonmax=<lonmax>       * the maximum longitude (degrees E).
  --data=<data-type>      * the type of data to select.

  <data-type> can be selected from:
      fixmes              = fixme tags extracted from the data.

--dump-visualiser         Dump selected contents of the database in HTML.
  --data=fixme<number>    * the fixme with the selected index.

                        Tagging Rule Tester / Tag Modifier
                        ==================================

This program is used to run the tag transformation process on an OSM XML file
for test purposes.  This allows it to be used to test new tagging rules or to
make automatic rule-based modifications to tags within an XML file.


tagmodifier
-----------

Usage: tagmodifier [--help]
                   [--tagging=<filename>]
                   [--loggable] [--logtime]
                   [--errorlog[<name>]]
                   [<filename.osm> | <filename.osm.bz2> | <filename.osm.gz>]

--help
       Prints out the help information.

--tagging=<filename>
       The name of the XML file containing the tagging rules (defaults
       to 'tagging.xml' in the current directory).

--loggable
       Print progress messages that are suitable for logging to a file;
       normally an incrementing counter is printed which is more
       suitable for real-time display than logging.

--logtime
       Print the elapsed time for the processing.

--errorlog[=<name>]
       Log parsing errors to 'error.log' or the specified file name.

<filename.osm>
       Specifies the filename(s) to read data from. Filenames ending
       '.bz2' will be bzip2 uncompressed (if bzip2 support compiled
       in). Filenames ending '.gz' will be gzip uncompressed (if gzip
       support compiled in).

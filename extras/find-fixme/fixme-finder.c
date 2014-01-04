/***************************************
 OSM planet file fixme finder.

 Part of the Routino routing software.
 ******************/ /******************
 This file Copyright 2008-2013 Andrew M. Bishop

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU Affero General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Affero General Public License for more details.

 You should have received a copy of the GNU Affero General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ***************************************/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "types.h"
#include "ways.h"

#include "typesx.h"
#include "nodesx.h"
#include "waysx.h"
#include "relationsx.h"

#include "files.h"
#include "logging.h"
#include "errorlogx.h"
#include "functions.h"
#include "osmparser.h"
#include "tagging.h"
#include "uncompress.h"


/* Global variables */

/*+ The name of the temporary directory. +*/
char *option_tmpdirname=NULL;

/*+ The amount of RAM to use for filesorting. +*/
size_t option_filesort_ramsize=0;

/*+ The number of threads to use for filesorting. +*/
int option_filesort_threads=1;


/* Local functions */

static void print_usage(int detail,const char *argerr,const char *err);


/*++++++++++++++++++++++++++++++++++++++
  The main program for the find-fixme.
  ++++++++++++++++++++++++++++++++++++++*/

int main(int argc,char** argv)
{
 NodesX     *OSMNodes;
 WaysX      *OSMWays;
 RelationsX *OSMRelations;
 ErrorLogsX *OSMErrorLogs;
 char       *dirname=NULL,*prefix=NULL,*tagging="fixme.xml",*errorlog="fixme.log";
 int         option_keep=1;
 int         option_filenames=0;
 int         arg;

 printf_program_start();

 /* Parse the command line arguments */

 for(arg=1;arg<argc;arg++)
   {
    if(!strcmp(argv[arg],"--help"))
       print_usage(1,NULL,NULL);
    else if(!strncmp(argv[arg],"--dir=",6))
       dirname=&argv[arg][6];
    else if(!strncmp(argv[arg],"--sort-ram-size=",16))
       option_filesort_ramsize=atoi(&argv[arg][16]);
#if defined(USE_PTHREADS) && USE_PTHREADS
    else if(!strncmp(argv[arg],"--sort-threads=",15))
       option_filesort_threads=atoi(&argv[arg][15]);
#endif
    else if(!strncmp(argv[arg],"--tmpdir=",9))
       option_tmpdirname=&argv[arg][9];
    else if(!strncmp(argv[arg],"--tagging=",10))
       tagging=&argv[arg][10];
    else if(!strcmp(argv[arg],"--loggable"))
       option_loggable=1;
    else if(!strcmp(argv[arg],"--logtime"))
       option_logtime=1;
    else if(argv[arg][0]=='-' && argv[arg][1]=='-')
       print_usage(0,argv[arg],NULL);
    else
       option_filenames++;
   }

 /* Check the specified command line options */

 if(!option_filesort_ramsize)
   {
#if SLIM
    option_filesort_ramsize=64*1024*1024;
#else
    option_filesort_ramsize=256*1024*1024;
#endif
   }
 else
    option_filesort_ramsize*=1024*1024;

 if(!option_tmpdirname)
   {
    if(!dirname)
       option_tmpdirname=".";
    else
       option_tmpdirname=dirname;
   }

 if(tagging)
   {
    if(!ExistsFile(tagging))
      {
       fprintf(stderr,"Error: The '--tagging' option specifies a file that does not exist.\n");
       exit(EXIT_FAILURE);
      }
   }
 else
   {
    if(ExistsFile(FileName(dirname,prefix,"tagging.xml")))
       tagging=FileName(dirname,prefix,"tagging.xml");
    else
      {
       fprintf(stderr,"Error: The '--tagging' option was not used and the default 'tagging.xml' does not exist.\n");
       exit(EXIT_FAILURE);
      }
   }

 if(ParseXMLTaggingRules(tagging))
   {
    fprintf(stderr,"Error: Cannot read the tagging rules in the file '%s'.\n",tagging);
    exit(EXIT_FAILURE);
   }

 /* Create new node, segment, way and relation variables */

 OSMNodes=NewNodeList(0,0);

 OSMWays=NewWayList(0,0);

 OSMRelations=NewRelationList(0,0);

 /* Create the error log file */

 if(errorlog)
    open_errorlog(FileName(dirname,prefix,errorlog),0,option_keep);

 /* Parse the file */

 for(arg=1;arg<argc;arg++)
   {
    int fd;
    char *filename,*p;

    if(argv[arg][0]=='-' && argv[arg][1]=='-')
       continue;

    filename=strcpy(malloc(strlen(argv[arg])+1),argv[arg]);

    fd=OpenFile(filename);

    if((p=strstr(filename,".bz2")) && !strcmp(p,".bz2"))
      {
       fd=Uncompress_Bzip2(fd);
       *p=0;
      }

    if((p=strstr(filename,".gz")) && !strcmp(p,".gz"))
      {
       fd=Uncompress_Gzip(fd);
       *p=0;
      }

    printf("\nParse OSM Data [%s]\n==============\n\n",filename);
    fflush(stdout);

    if((p=strstr(filename,".pbf")) && !strcmp(p,".pbf"))
      {
       if(ParsePBFFile(fd,OSMNodes,OSMWays,OSMRelations))
          exit(EXIT_FAILURE);
      }
    else if((p=strstr(filename,".o5m")) && !strcmp(p,".o5m"))
      {
       if(ParseO5MFile(fd,OSMNodes,OSMWays,OSMRelations))
          exit(EXIT_FAILURE);
      }
    else
      {
       if(ParseOSMFile(fd,OSMNodes,OSMWays,OSMRelations))
          exit(EXIT_FAILURE);
      }

    CloseFile(fd);

    free(filename);
   }

 DeleteXMLTaggingRules();

 FinishNodeList(OSMNodes);
 FinishWayList(OSMWays);
 FinishRelationList(OSMRelations);

 /* Sort the data */

 printf("\nSort OSM Data\n=============\n\n");
 fflush(stdout);

 /* Sort the nodes, ways and relations */

 SortNodeList(OSMNodes);

 SortWayList(OSMWays);

 SortRelationList(OSMRelations);

 /* Process the data */

 RenameFile(OSMNodes->filename_tmp,OSMNodes->filename);
 RenameFile(OSMWays->filename_tmp,OSMWays->filename);
 RenameFile(OSMRelations->rrfilename_tmp,OSMRelations->rrfilename);
 RenameFile(OSMRelations->trfilename_tmp,OSMRelations->trfilename);

 close_errorlog();

 printf("\nCreate Error Log\n================\n\n");
 fflush(stdout);

 OSMErrorLogs=NewErrorLogList();

 ProcessErrorLogs(OSMErrorLogs,OSMNodes,OSMWays,OSMRelations);

 SortErrorLogsGeographically(OSMErrorLogs);

 SaveErrorLogs(OSMErrorLogs,FileName(dirname,prefix,"fixme.mem"));

 FreeErrorLogList(OSMErrorLogs);

 /* Free the memory (delete the temporary files) */

 FreeNodeList(OSMNodes,0);
 FreeWayList(OSMWays,0);
 FreeRelationList(OSMRelations,0);

 printf_program_end();

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  Print out the usage information.

  int detail The level of detail to use - 0 = low, 1 = high.

  const char *argerr The argument that gave the error (if there is one).

  const char *err Other error message (if there is one).
  ++++++++++++++++++++++++++++++++++++++*/

static void print_usage(int detail,const char *argerr,const char *err)
{
 fprintf(stderr,
         "Usage: fixme-finder [--help]\n"
         "                    [--dir=<dirname>]\n"
#if defined(USE_PTHREADS) && USE_PTHREADS
         "                    [--sort-ram-size=<size>] [--sort-threads=<number>]\n"
#else
         "                    [--sort-ram-size=<size>]\n"
#endif
         "                    [--tmpdir=<dirname>]\n"
         "                    [--tagging=<filename>]\n"
         "                    [--loggable] [--logtime]\n"
         "                    [<filename.osm> ...\n"
         "                     | <filename.pbf> ...\n"
         "                     | <filename.o5m> ..."
#if defined(USE_BZIP2) && USE_BZIP2
         "\n                     | <filename.(osm|o5m).bz2> ..."
#endif
#if defined(USE_GZIP) && USE_GZIP
         "\n                     | <filename.(osm|o5m).gz> ..."
#endif
         "]\n");

 if(argerr)
    fprintf(stderr,
            "\n"
            "Error with command line parameter: %s\n",argerr);

 if(err)
    fprintf(stderr,
            "\n"
            "Error: %s\n",err);

 if(detail)
    fprintf(stderr,
            "\n"
            "--help                    Prints this information.\n"
            "\n"
            "--dir=<dirname>           The directory containing the fixme database.\n"
            "\n"
            "--sort-ram-size=<size>    The amount of RAM (in MB) to use for data sorting\n"
#if SLIM
            "                          (defaults to 64MB otherwise.)\n"
#else
            "                          (defaults to 256MB otherwise.)\n"
#endif
#if defined(USE_PTHREADS) && USE_PTHREADS
            "--sort-threads=<number>   The number of threads to use for data sorting.\n"
#endif
            "\n"
            "--tmpdir=<dirname>        The directory name for temporary files.\n"
            "                          (defaults to the '--dir' option directory.)\n"
            "\n"
            "--tagging=<filename>      The name of the XML file containing the tagging rules\n"
            "                          (defaults to 'fixme.xml' with '--dir' option)\n"
            "\n"
            "--loggable                Print progress messages suitable for logging to file.\n"
            "--logtime                 Print the elapsed time for each processing step.\n"
            "\n"
            "<filename.osm>, <filename.pbf>, <filename.o5m>\n"
            "                          The name(s) of the file(s) to read and parse.\n"
            "                          Filenames ending '.pbf' read as PBF, filenames ending\n"
            "                          '.o5m' read as O5M, others as XML.\n"
#if defined(USE_BZIP2) && USE_BZIP2
            "                          Filenames ending '.bz2' will be bzip2 uncompressed.\n"
#endif
#if defined(USE_GZIP) && USE_GZIP
            "                          Filenames ending '.gz' will be gzip uncompressed.\n"
#endif
            );

 exit(!detail);
}

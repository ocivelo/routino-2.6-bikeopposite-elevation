/***************************************
 Error logging functions

 Part of the Routino routing software.
 ******************/ /******************
 This file Copyright 2013 Andrew M. Bishop

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
#include <stdarg.h>
#include <errno.h>

#include "typesx.h"

#include "files.h"
#include "logging.h"


/* Global variables */

/*+ The name of the error log file. +*/
char *errorlogfilename=NULL;

/*+ The name of the binary error log file. +*/
char *errorbinfilename=NULL;


/* Local variables */

/*+ The file handle for the error log file. +*/
static FILE *errorlogfile=NULL;

/*+ The file descriptor for the binary error log file. +*/
static int errorbinfile=-1;

/*+ The offset of the error message in the error log file. +*/
static off_t errorfileoffset=0;


/*++++++++++++++++++++++++++++++++++++++
  Create the error log file.

  const char *filename The name of the file to create.

  int append The option to append to an existing file.

  int bin The option to enable a binary log file.
  ++++++++++++++++++++++++++++++++++++++*/

void open_errorlog(const char *filename,int append,int bin)
{
 /* Text log file */

 errorlogfilename=(char*)malloc(strlen(filename)+8);

 strcpy(errorlogfilename,filename);

 errorlogfile=fopen(errorlogfilename,append?"a":"w");

 if(!errorlogfile)
   {
    fprintf(stderr,"Cannot open file '%s' for writing [%s].\n",errorlogfilename,strerror(errno));
    exit(EXIT_FAILURE);
   }

 /* Binary log file */

 if(bin)
   {
    errorbinfilename=(char*)malloc(strlen(filename)+8);

    sprintf(errorbinfilename,"%s.tmp",filename);

    errorfileoffset=0;

    if(append)
      {
       if(ExistsFile(filename))
          errorfileoffset=SizeFile(filename);

       errorbinfile=OpenFileBufferedAppend(errorbinfilename);
      }
    else
       errorbinfile=OpenFileBufferedNew(errorbinfilename);
   }
 else
    errorbinfile=-1;
}


/*++++++++++++++++++++++++++++++++++++++
  Close the error log file.
  ++++++++++++++++++++++++++++++++++++++*/

void close_errorlog(void)
{
 if(errorlogfile)
   {
    fclose(errorlogfile);

    if(errorbinfile!=-1)
       CloseFileBuffered(errorbinfile);
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Log a message to the error log file.

  const char *format The format string.

  ... The other arguments.
  ++++++++++++++++++++++++++++++++++++++*/

void logerror(const char *format, ...)
{
 va_list ap;

 if(!errorlogfile)
    return;

 va_start(ap,format);

 errorfileoffset+=vfprintf(errorlogfile,format,ap);

 va_end(ap);
}


/*++++++++++++++++++++++++++++++++++++++
  Store the node information in the binary log file for this message.

  node_t logerror_node Returns the node identifier.

  node_t id The node identifier.
  ++++++++++++++++++++++++++++++++++++++*/

node_t logerror_node(node_t id)
{
 if(errorbinfile!=-1)
   {
    ErrorLogObject error={0};

    error.id=id;
    error.type='N';

    error.offset=errorfileoffset;

    WriteFileBuffered(errorbinfile,&error,sizeof(ErrorLogObject));
   }

 return(id);
}


/*++++++++++++++++++++++++++++++++++++++
  Store the way information in the binary log file for this message.

  way_t logerror_way Returns the way identifier.

  way_t id The way identifier.
  ++++++++++++++++++++++++++++++++++++++*/

way_t logerror_way(way_t id)
{
 if(errorbinfile!=-1)
   {
    ErrorLogObject error={0};

    error.id=id;
    error.type='W';

    error.offset=errorfileoffset;

    WriteFileBuffered(errorbinfile,&error,sizeof(ErrorLogObject));
   }

 return(id);
}


/*++++++++++++++++++++++++++++++++++++++
  Store the relation information in the binary log file for this message.

  relation_t logerror_relation Returns the relation identifier.

  relation_t id The relation identifier.
  ++++++++++++++++++++++++++++++++++++++*/

relation_t logerror_relation(relation_t id)
{
 if(errorbinfile!=-1)
   {
    ErrorLogObject error={0};

    error.id=id;
    error.type='R';

    error.offset=errorfileoffset;

    WriteFileBuffered(errorbinfile,&error,sizeof(ErrorLogObject));
   }

 return(id);
}

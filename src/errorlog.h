/***************************************
 Header file for error log file data types and associated function prototypes.

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


#ifndef ERRORLOG_H
#define ERRORLOG_H    /*+ To stop multiple inclusions. +*/

#include <stdint.h>
#include <sys/types.h>

#include "types.h"
#include "typesx.h"

#include "files.h"


/*+ A structure containing information for an error message in the file. +*/
typedef struct _ErrorLog
{
 ll_off_t  latoffset;        /*+ The error message latitude offset within its bin. +*/
 ll_off_t  lonoffset;        /*+ The error message longitude offset within its bin. +*/

 uint32_t  offset;           /*+ The offset of the error message from the beginning of the text section. +*/
 uint32_t  length;           /*+ The length of the error message in the text section. +*/
}
 ErrorLog;


/*+ A structure containing the header from the error log file. +*/
typedef struct _ErrorLogsFile
{
 index_t  number;               /*+ The total number of error messages. +*/
 index_t  number_geo;           /*+ The number of error messages with a geographical location. +*/
 index_t  number_nongeo;        /*+ The number of error messages without a geographical location. +*/

 ll_bin_t latbins;              /*+ The number of bins containing latitude. +*/
 ll_bin_t lonbins;              /*+ The number of bins containing longitude. +*/

 ll_bin_t latzero;              /*+ The bin number of the furthest south bin. +*/
 ll_bin_t lonzero;              /*+ The bin number of the furthest west bin. +*/
}
 ErrorLogsFile;


/*+ A structure containing a set of error log messages read from the file. +*/
typedef struct _ErrorLogs
{
 ErrorLogsFile file;            /*+ The header data from the file. +*/

#if !SLIM

 void     *data;                /*+ The memory mapped data in the file. +*/

 index_t  *offsets;             /*+ A pointer to the array of offsets in the file. +*/

 ErrorLog *errorlogs_geo;       /*+ A pointer to the array of geographical error logs in the file. +*/
 ErrorLog *errorlogs_nongeo;    /*+ A pointer to the array of non-geographical error logs in the file. +*/

 char     *strings;             /*+ A pointer to the array of error strings in the file. +*/

#else

 int       fd;                  /*+ The file descriptor for the file. +*/

 off_t     offsetsoffset;       /*+ An allocated array with a copy of the file offsets. +*/

 off_t     errorlogsoffset_geo;    /*+ The offset of the geographical error logs within the file. +*/
 off_t     errorlogsoffset_nongeo; /*+ The offset of the non-geographical error logs within the file. +*/

 off_t     stringsoffset;       /*+ The offset of the error strings within the file. +*/

 ErrorLog  cached[2];           /*+ Some cached error logs read from the file in slim mode. +*/

 char      cachestring[1024];   /*+ A cached copy of the error string read from the file in slim mode. +*/

#endif
}
 ErrorLogs;


/* Error log functions in errorlog.c */

ErrorLogs *LoadErrorLogs(const char *filename);

void DestroyErrorLogs(ErrorLogs *errorlogs);

void GetErrorLogLatLong(ErrorLogs *errorlogs,index_t index,ErrorLog *errorlogp,double *latitude,double *longitude);


/* Macros and inline functions */

#if !SLIM

/*+ Return an ErrorLog pointer given a set of errorlogs and an index. +*/
#define LookupErrorLog(xxx,yyy,ppp)     (&(xxx)->errorlogs_geo[yyy])

/*+ Return the offset of a geographical region given a set of errorlogs. +*/
#define LookupErrorLogOffset(xxx,yyy)   ((xxx)->offsets[yyy])

/*+ Return the string for an error log. +*/
#define LookupErrorLogString(xxx,yyy)   (&(xxx)->strings[(xxx)->errorlogs_geo[yyy].offset])

#else

/* Prototypes */

static inline ErrorLog *LookupErrorLog(ErrorLogs *errorlogs,index_t index,int position);

static inline index_t LookupErrorLogOffset(ErrorLogs *errorlogs,index_t index);

static inline char *LookupErrorLogString(ErrorLogs *errorlogs,index_t index);

/* Inline functions */

/*++++++++++++++++++++++++++++++++++++++
  Find the ErrorLog information for a particular error log.

  ErrorLog *LookupErrorLog Returns a pointer to the cached error log information.

  ErrorLogs *errorlogs The set of errorlogs to use.

  index_t index The index of the error log.

  int position The position in the cache to store the value.
  ++++++++++++++++++++++++++++++++++++++*/

static inline ErrorLog *LookupErrorLog(ErrorLogs *errorlogs,index_t index,int position)
{
 SlimFetch(errorlogs->fd,&errorlogs->cached[position-1],sizeof(ErrorLog),errorlogs->errorlogsoffset_geo+(off_t)index*sizeof(ErrorLog));

 return(&errorlogs->cached[position-1]);
}


/*++++++++++++++++++++++++++++++++++++++
  Find the offset of error logs in a geographical region.

  index_t LookupErrorLogOffset Returns the index offset.

  ErrorLogs *errorlogs The set of error logs to use.

  index_t index The index of the offset.
  ++++++++++++++++++++++++++++++++++++++*/

static inline index_t LookupErrorLogOffset(ErrorLogs *errorlogs,index_t index)
{
 index_t offset;

 SlimFetch(errorlogs->fd,&offset,sizeof(index_t),errorlogs->offsetsoffset+(off_t)index*sizeof(index_t));

 return(offset);
}


/*++++++++++++++++++++++++++++++++++++++
  Find the string associated with a particular error log.

  char *LookupErrorString Returns the error string.

  ErrorLogs *errorlogs The set of error logs to use.

  index_t index The index of the string.
  ++++++++++++++++++++++++++++++++++++++*/

static inline char *LookupErrorLogString(ErrorLogs *errorlogs,index_t index)
{
 ErrorLog *errorlog=LookupErrorLog(errorlogs,index,2);

 SlimFetch(errorlogs->fd,errorlogs->cachestring,errorlog->length,errorlogs->stringsoffset+errorlog->offset);

 return(errorlogs->cachestring);
}

#endif


#endif /* ERRORLOG_H */

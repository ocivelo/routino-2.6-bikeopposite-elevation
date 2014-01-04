/***************************************
 Error log data type functions.

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


#include <stdlib.h>
#include <math.h>

#include "types.h"
#include "errorlog.h"

#include "files.h"


/*++++++++++++++++++++++++++++++++++++++
  Load in an error log list from a file.

  ErrorLogs *LoadErrorLogs Returns the error log list.

  const char *filename The name of the file to load.
  ++++++++++++++++++++++++++++++++++++++*/

ErrorLogs *LoadErrorLogs(const char *filename)
{
 ErrorLogs *errorlogs;

 errorlogs=(ErrorLogs*)malloc(sizeof(ErrorLogs));

#if !SLIM

 errorlogs->data=MapFile(filename);

 /* Copy the ErrorLogsFile header structure from the loaded data */

 errorlogs->file=*((ErrorLogsFile*)errorlogs->data);

 /* Set the pointers in the ErrorLogs structure. */

 errorlogs->offsets         =(index_t* )(errorlogs->data+sizeof(ErrorLogsFile));
 errorlogs->errorlogs_geo   =(ErrorLog*)(errorlogs->data+sizeof(ErrorLogsFile)+(errorlogs->file.latbins*errorlogs->file.lonbins+1)*sizeof(index_t));
 errorlogs->errorlogs_nongeo=(ErrorLog*)(errorlogs->data+sizeof(ErrorLogsFile)+(errorlogs->file.latbins*errorlogs->file.lonbins+1)*sizeof(index_t)+errorlogs->file.number_geo*sizeof(ErrorLog));
 errorlogs->strings         =(char*    )(errorlogs->data+sizeof(ErrorLogsFile)+(errorlogs->file.latbins*errorlogs->file.lonbins+1)*sizeof(index_t)+errorlogs->file.number*sizeof(ErrorLog));

#else

 errorlogs->fd=SlimMapFile(filename);

 /* Copy the ErrorLogsFile header structure from the loaded data */

 SlimFetch(errorlogs->fd,&errorlogs->file,sizeof(ErrorLogsFile),0);

 errorlogs->offsetsoffset         =sizeof(ErrorLogsFile);
 errorlogs->errorlogsoffset_geo   =sizeof(ErrorLogsFile)+(errorlogs->file.latbins*errorlogs->file.lonbins+1)*sizeof(index_t);
 errorlogs->errorlogsoffset_nongeo=sizeof(ErrorLogsFile)+(errorlogs->file.latbins*errorlogs->file.lonbins+1)*sizeof(index_t)+errorlogs->file.number_geo*sizeof(ErrorLog);
 errorlogs->stringsoffset         =sizeof(ErrorLogsFile)+(errorlogs->file.latbins*errorlogs->file.lonbins+1)*sizeof(index_t)+errorlogs->file.number*sizeof(ErrorLog);

#endif

 return(errorlogs);
}


/*++++++++++++++++++++++++++++++++++++++
  Destroy the node list.

  ErrorLogs *errorlogs The node list to destroy.
  ++++++++++++++++++++++++++++++++++++++*/

void DestroyErrorLogs(ErrorLogs *errorlogs)
{
#if !SLIM

 errorlogs->data=UnmapFile(errorlogs->data);

#else

 errorlogs->fd=SlimUnmapFile(errorlogs->fd);

#endif

 free(errorlogs);
}


/*++++++++++++++++++++++++++++++++++++++
  Get the latitude and longitude associated with an error log.

  ErrorLogs *errorlogs The set of error logs to use.

  index_t index The errorlog index.

  ErrorLog *errorlogp A pointer to the error log.

  double *latitude Returns the latitude.

  double *longitude Returns the logitude.
  ++++++++++++++++++++++++++++++++++++++*/

void GetErrorLogLatLong(ErrorLogs *errorlogs,index_t index,ErrorLog *errorlogp,double *latitude,double *longitude)
{
 ll_bin_t latbin,lonbin;
 ll_bin2_t bin=-1;
 ll_bin2_t start,end,mid;
 index_t offset;

 /* Binary search - search key nearest match below is required.
  *
  *  # <- start  |  Check mid and move start or end if it doesn't match
  *  #           |
  *  #           |  A lower bound match is wanted we can set end=mid-1 or
  *  # <- mid    |  start=mid because we know that mid doesn't match.
  *  #           |
  *  #           |  Eventually either end=start or end=start+1 and one of
  *  # <- end    |  start or end is the wanted one.
  */

 /* Search for offset */

 start=0;
 end=errorlogs->file.lonbins*errorlogs->file.latbins;

 do
   {
    mid=(start+end)/2;                  /* Choose mid point */

    offset=LookupErrorLogOffset(errorlogs,mid);

    if(offset<index)                    /* Mid point is too low for an exact match but could be lower bound */
       start=mid;
    else if(offset>index)               /* Mid point is too high */
       end=mid?(mid-1):mid;
    else                                /* Mid point is correct */
      {bin=mid;break;}
   }
 while((end-start)>1);

 if(bin==-1)
   {
    offset=LookupErrorLogOffset(errorlogs,end);

    if(offset>index)
       bin=start;
    else
       bin=end;
   }

 while(bin<=(errorlogs->file.lonbins*errorlogs->file.latbins) && 
       LookupErrorLogOffset(errorlogs,bin)==LookupErrorLogOffset(errorlogs,bin+1))
    bin++;

 latbin=bin%errorlogs->file.latbins;
 lonbin=bin/errorlogs->file.latbins;

 /* Return the values */

 if(errorlogp==NULL)
    errorlogp=LookupErrorLog(errorlogs,index,2);

 *latitude =latlong_to_radians(bin_to_latlong(errorlogs->file.latzero+latbin)+off_to_latlong(errorlogp->latoffset));
 *longitude=latlong_to_radians(bin_to_latlong(errorlogs->file.lonzero+lonbin)+off_to_latlong(errorlogp->lonoffset));
}

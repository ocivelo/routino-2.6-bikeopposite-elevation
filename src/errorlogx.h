/***************************************
 Header file for error log file data types and processing function prototypes.

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


#ifndef ERRORLOGX_H
#define ERRORLOGX_H    /*+ To stop multiple inclusions. +*/

#include <stdint.h>

#include "types.h"
#include "typesx.h"


/*+ A structure containing information for an error message during processing. +*/
typedef struct _ErrorLogX
{
 latlong_t latitude;         /*+ The error message latitude. +*/
 latlong_t longitude;        /*+ The error message longitude. +*/

 uint32_t  offset;           /*+ The offset of the error message from the beginning of the text file. +*/
 uint32_t  length;           /*+ The length of the error message in the text file. +*/
}
 ErrorLogX;


/*+ A structure containing a set of error logs (memory format). +*/
typedef struct _ErrorLogsX
{
 index_t   number;              /*+ The number of error logs. +*/

 index_t   latbins;             /*+ The number of bins containing latitude. +*/
 index_t   lonbins;             /*+ The number of bins containing longitude. +*/

 ll_bin_t  latzero;             /*+ The bin number of the furthest south bin. +*/
 ll_bin_t  lonzero;             /*+ The bin number of the furthest west bin. +*/
}
 ErrorLogsX;


/* Error log processing functions in errorlogx.c */

ErrorLogsX *NewErrorLogList(void);
void FreeErrorLogList(ErrorLogsX *errorlogsx);

void ProcessErrorLogs(ErrorLogsX *errorlogsx,NodesX *nodesx,WaysX *waysx,RelationsX *relationsx);

void SortErrorLogsGeographically(ErrorLogsX *errorlogsx);

void SaveErrorLogs(ErrorLogsX *errorlogsx,char *filename);


#endif /* ERRORLOGX_H */

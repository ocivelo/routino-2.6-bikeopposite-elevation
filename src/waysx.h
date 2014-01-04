/***************************************
 A header file for the extended Ways structure.

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


#ifndef WAYSX_H
#define WAYSX_H    /*+ To stop multiple inclusions. +*/

#include <stdint.h>

#include "types.h"

#include "typesx.h"
#include "ways.h"

#include "cache.h"
#include "files.h"


/* Data structures */


/*+ An extended structure containing a single way. +*/
struct _WayX
{
 way_t    id;                   /*+ The way identifier; initially the OSM value, later the Way index. +*/

 Way      way;                  /*+ The real way data. +*/
};


/*+ A structure containing a set of ways (memory format). +*/
struct _WaysX
{
 char    *filename;             /*+ The name of the intermediate file (for the WaysX). +*/
 char    *filename_tmp;         /*+ The name of the temporary file (for the WaysX). +*/

 int      fd;                   /*+ The file descriptor of the open file (for the WaysX). +*/

 index_t  number;               /*+ The number of extended ways still being considered. +*/
 index_t  knumber;              /*+ The number of extended ways kept for next time. +*/

 transports_t allow;            /*+ The types of traffic that were seen when parsing. +*/

#if !SLIM

 WayX    *data;                 /*+ The extended ways data (when mapped into memory). +*/

#else

 WayX     cached[3];            /*+ Three cached extended ways read from the file in slim mode. +*/
 index_t  incache[3];           /*+ The indexes of the cached extended ways. +*/

 WayXCache *cache;              /*+ A RAM cache of extended ways read from the file. +*/

#endif

 way_t   *idata;                /*+ The extended way IDs (sorted by ID). +*/
 off_t   *odata;                /*+ The offset of the way in the file (used for error log). +*/

 index_t *cdata;                /*+ The compacted way IDs (same order as sorted ways). +*/

 char    *nfilename_tmp;        /*+ The name of the temporary file (for the WaysX names). +*/

 int      nfd;                  /*+ The file descriptor of the temporary file (for the WaysX names). +*/

 uint32_t nlength;              /*+ The length of the string of name entries. +*/
};


/* Functions in waysx.c */


WaysX *NewWayList(int append,int readonly);
void FreeWayList(WaysX *waysx,int keep);

void AppendWayList(WaysX *waysx,way_t id,Way *way,node_t *nodes,int nnodes,const char *name);
void FinishWayList(WaysX *waysx);

index_t IndexWayX(WaysX *waysx,way_t id);

void SortWayList(WaysX *waysx);

SegmentsX *SplitWays(WaysX *waysx,NodesX *nodesx,int keep);

void SortWayNames(WaysX *waysx);

void CompactWayList(WaysX *waysx,SegmentsX *segmentsx);

void SaveWayList(WaysX *waysx,const char *filename);


/* Macros / inline functions */

#if !SLIM

#define LookupWayX(waysx,index,position)  &(waysx)->data[index]
  
#define PutBackWayX(waysx,wayx)           while(0) { /* nop */ }

#else

/* Prototypes */

static inline WayX *LookupWayX(WaysX *waysx,index_t index,int position);

static inline void PutBackWayX(WaysX *waysx,WayX *wayx);

CACHE_NEWCACHE_PROTO(WayX)
CACHE_DELETECACHE_PROTO(WayX)
CACHE_FETCHCACHE_PROTO(WayX)
CACHE_REPLACECACHE_PROTO(WayX)
CACHE_INVALIDATECACHE_PROTO(WayX)


/* Inline functions */

CACHE_STRUCTURE(WayX)
CACHE_NEWCACHE(WayX)
CACHE_DELETECACHE(WayX)
CACHE_FETCHCACHE(WayX)
CACHE_REPLACECACHE(WayX)
CACHE_INVALIDATECACHE(WayX)


/*++++++++++++++++++++++++++++++++++++++
  Lookup a particular extended way with the specified id from the file on disk.

  WayX *LookupWayX Returns a pointer to a cached copy of the extended way.

  WaysX *waysx The set of ways to use.

  index_t index The way index to look for.

  int position The position in the cache to use.
  ++++++++++++++++++++++++++++++++++++++*/

static inline WayX *LookupWayX(WaysX *waysx,index_t index,int position)
{
 waysx->cached[position-1]=*FetchCachedWayX(waysx->cache,index,waysx->fd,0);

 waysx->incache[position-1]=index;

 return(&waysx->cached[position-1]);
}


/*++++++++++++++++++++++++++++++++++++++
  Put back an extended way's data into the file on disk.

  WaysX *waysx The set of ways to use.

  WayX *wayx The extended way to be put back.
  ++++++++++++++++++++++++++++++++++++++*/

static inline void PutBackWayX(WaysX *waysx,WayX *wayx)
{
 int position1=wayx-&waysx->cached[0];

 ReplaceCachedWayX(waysx->cache,wayx,waysx->incache[position1],waysx->fd,0);
}

#endif /* SLIM */


#endif /* WAYSX_H */

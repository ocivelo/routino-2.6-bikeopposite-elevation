/***************************************
 A header file for the extended segments.

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


#ifndef SEGMENTSX_H
#define SEGMENTSX_H    /*+ To stop multiple inclusions. +*/

#include <stdint.h>

#include "types.h"

#include "typesx.h"

#include "cache.h"
#include "files.h"


/* Data structures */


/*+ An extended structure used for processing. +*/
struct _SegmentX
{
 index_t    node1;              /*+ The NodeX index of the starting node. +*/
 index_t    node2;              /*+ The NodeX index of the finishing node. +*/

 index_t    next2;              /*+ The index of the next segment with the same node2. +*/

 index_t    way;                /*+ The WayX index of the way. +*/

 distance_t distance;           /*+ The distance between the nodes. +*/
};


/*+ A structure containing a set of segments (memory format). +*/
struct _SegmentsX
{
 char      *filename_tmp;       /*+ The name of the temporary file (for the SegmentsX). +*/

 int        fd;                 /*+ The file descriptor of the open file (for the SegmentsX). +*/

 index_t    number;             /*+ The number of extended segments still being considered. +*/

#if !SLIM

 SegmentX  *data;               /*+ The extended segment data (when mapped into memory). +*/

#else

 SegmentX   cached[4];          /*+ Four cached extended segments read from the file in slim mode. +*/
 index_t    incache[4];         /*+ The indexes of the cached extended segments. +*/

 SegmentXCache *cache;          /*+ A RAM cache of extended segments read from the file. +*/

#endif

 index_t   *firstnode;          /*+ The first segment index for each node. +*/

 index_t   *next1;              /*+ The index of the next segment with the same node1 (used while pruning). +*/

 BitMask   *usedway;            /*+ A flag to indicate if a way is used (used for removing pruned ways). +*/
};


/* Functions in segmentsx.c */

SegmentsX *NewSegmentList(void);
void FreeSegmentList(SegmentsX *segmentsx);

void AppendSegmentList(SegmentsX *segmentsx,index_t way,index_t node1,index_t node2,distance_t distance);
void FinishSegmentList(SegmentsX *segmentsx);

SegmentX *FirstSegmentX(SegmentsX *segmentsx,index_t nodeindex,int position);
SegmentX *NextSegmentX(SegmentsX *segmentsx,SegmentX *segmentx,index_t nodeindex);

void SortSegmentList(SegmentsX *segmentsx);

void IndexSegments(SegmentsX *segmentsx,NodesX *nodesx,WaysX *waysx);

void ProcessSegments(SegmentsX *segmentsx,NodesX *nodesx,WaysX *waysx);

void RemovePrunedSegments(SegmentsX *segmentsx,WaysX *waysx);

void DeduplicateSuperSegments(SegmentsX *segmentsx,WaysX *waysx);

void SortSegmentListGeographically(SegmentsX *segmentsx,NodesX *nodesx);

void SaveSegmentList(SegmentsX *segmentsx,const char *filename);


/* Macros / inline functions */

/*+ Return true if this is a pruned segment. +*/
#define IsPrunedSegmentX(xxx)   ((xxx)->node1==NO_NODE)


#if !SLIM

#define LookupSegmentX(segmentsx,index,position)         &(segmentsx)->data[index]

#define IndexSegmentX(segmentsx,segmentx)                (index_t)((segmentx)-&(segmentsx)->data[0])

#define PutBackSegmentX(segmentsx,segmentx)              while(0) { /* nop */ }

#define ReLookupSegmentX(segmentsx,segmentx)             while(0) { /* nop */ }
  
#else

/* Prototypes */

static inline SegmentX *LookupSegmentX(SegmentsX *segmentsx,index_t index,int position);

static inline index_t IndexSegmentX(SegmentsX *segmentsx,SegmentX *segmentx);

static inline void PutBackSegmentX(SegmentsX *segmentsx,SegmentX *segmentx);

static inline void ReLookupSegmentX(SegmentsX *segmentsx,SegmentX *segmentx);

CACHE_NEWCACHE_PROTO(SegmentX)
CACHE_DELETECACHE_PROTO(SegmentX)
CACHE_FETCHCACHE_PROTO(SegmentX)
CACHE_REPLACECACHE_PROTO(SegmentX)
CACHE_INVALIDATECACHE_PROTO(SegmentX)


/* Inline functions */

CACHE_STRUCTURE(SegmentX)
CACHE_NEWCACHE(SegmentX)
CACHE_DELETECACHE(SegmentX)
CACHE_FETCHCACHE(SegmentX)
CACHE_REPLACECACHE(SegmentX)
CACHE_INVALIDATECACHE(SegmentX)


/*++++++++++++++++++++++++++++++++++++++
  Lookup a particular extended segment with the specified id from the file on disk.

  SegmentX *LookupSegmentX Returns a pointer to a cached copy of the extended segment.

  SegmentsX *segmentsx The set of segments to use.

  index_t index The segment index to look for.

  int position The position in the cache to use.
  ++++++++++++++++++++++++++++++++++++++*/

static inline SegmentX *LookupSegmentX(SegmentsX *segmentsx,index_t index,int position)
{
 segmentsx->cached[position-1]=*FetchCachedSegmentX(segmentsx->cache,index,segmentsx->fd,0);

 segmentsx->incache[position-1]=index;

 return(&segmentsx->cached[position-1]);
}


/*++++++++++++++++++++++++++++++++++++++
  Find the extended segment index for a particular extended segment pointer.

  index_t IndexSegmentX Returns the index of the extended segment.

  SegmentsX *segmentsx The set of segments to use.

  SegmentX *segmentx The extended segment whose index is to be found.
  ++++++++++++++++++++++++++++++++++++++*/

static inline index_t IndexSegmentX(SegmentsX *segmentsx,SegmentX *segmentx)
{
 int position1=segmentx-&segmentsx->cached[0];

 return(segmentsx->incache[position1]);
}


/*++++++++++++++++++++++++++++++++++++++
  Put back an extended segment's data into the file on disk.

  SegmentsX *segmentsx The set of segments to use.

  SegmentX *segmentx The extended segment to be put back.
  ++++++++++++++++++++++++++++++++++++++*/

static inline void PutBackSegmentX(SegmentsX *segmentsx,SegmentX *segmentx)
{
 int position1=segmentx-&segmentsx->cached[0];

 ReplaceCachedSegmentX(segmentsx->cache,segmentx,segmentsx->incache[position1],segmentsx->fd,0);
}


/*++++++++++++++++++++++++++++++++++++++
  Lookup an extended segment's data from the disk into file again after the disk was updated.

  SegmentsX *segmentsx The set of segments to use.

  SegmentX *segmentx The extended segment to refresh.
  ++++++++++++++++++++++++++++++++++++++*/

static inline void ReLookupSegmentX(SegmentsX *segmentsx,SegmentX *segmentx)
{
 int position1=segmentx-&segmentsx->cached[0];

 segmentsx->cached[position1]=*FetchCachedSegmentX(segmentsx->cache,segmentsx->incache[position1],segmentsx->fd,0);
}

#endif /* SLIM */


#endif /* SEGMENTSX_H */

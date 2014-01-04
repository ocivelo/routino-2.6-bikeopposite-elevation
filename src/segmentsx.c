/***************************************
 Extended Segment data type functions.

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


#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "segments.h"
#include "ways.h"

#include "typesx.h"
#include "nodesx.h"
#include "segmentsx.h"
#include "waysx.h"

#include "files.h"
#include "logging.h"
#include "sorting.h"


/* Global variables */

/*+ The command line '--tmpdir' option or its default value. +*/
extern char *option_tmpdirname;

/* Local variables */

/*+ Temporary file-local variables for use by the sort functions. +*/
static NodesX *sortnodesx;
static SegmentsX *sortsegmentsx;
static WaysX *sortwaysx;

/* Local functions */

static int sort_by_id(SegmentX *a,SegmentX *b);

static int delete_pruned(SegmentX *segmentx,index_t index);

static int deduplicate_super(SegmentX *segmentx,index_t index);

static int geographically_index(SegmentX *segmentx,index_t index);

static distance_t DistanceX(NodeX *nodex1,NodeX *nodex2);


/*++++++++++++++++++++++++++++++++++++++
  Allocate a new segment list (create a new file or open an existing one).

  SegmentsX *NewSegmentList Returns the segment list.
  ++++++++++++++++++++++++++++++++++++++*/

SegmentsX *NewSegmentList(void)
{
 SegmentsX *segmentsx;

 segmentsx=(SegmentsX*)calloc(1,sizeof(SegmentsX));

 logassert(segmentsx,"Failed to allocate memory (try using slim mode?)"); /* Check calloc() worked */

 segmentsx->filename_tmp=(char*)malloc(strlen(option_tmpdirname)+32);

 sprintf(segmentsx->filename_tmp,"%s/segmentsx.%p.tmp",option_tmpdirname,(void*)segmentsx);

 segmentsx->fd=OpenFileBufferedNew(segmentsx->filename_tmp);

#if SLIM
 segmentsx->cache=NewSegmentXCache();
#endif

 return(segmentsx);
}


/*++++++++++++++++++++++++++++++++++++++
  Free a segment list.

  SegmentsX *segmentsx The set of segments to be freed.
  ++++++++++++++++++++++++++++++++++++++*/

void FreeSegmentList(SegmentsX *segmentsx)
{
 DeleteFile(segmentsx->filename_tmp);

 free(segmentsx->filename_tmp);

 if(segmentsx->firstnode)
    free(segmentsx->firstnode);

 if(segmentsx->next1)
    free(segmentsx->next1);

#if SLIM
 DeleteSegmentXCache(segmentsx->cache);
#endif

 free(segmentsx);
}


/*++++++++++++++++++++++++++++++++++++++
  Append a single segment to an unsorted segment list.

  SegmentsX *segmentsx The set of segments to modify.

  index_t way The index of the way that the segment belongs to.

  index_t node1 The index of the first node in the segment.

  index_t node2 The index of the second node in the segment.

  distance_t distance The distance between the nodes (or just the flags).
  ++++++++++++++++++++++++++++++++++++++*/

void AppendSegmentList(SegmentsX *segmentsx,index_t way,index_t node1,index_t node2,distance_t distance)
{
 SegmentX segmentx;

 if(node1>node2)
   {
    index_t temp;

    temp=node1;
    node1=node2;
    node2=temp;

    if(distance&(ONEWAY_2TO1|ONEWAY_1TO2))
       distance^=ONEWAY_2TO1|ONEWAY_1TO2;
   }

 segmentx.node1=node1;
 segmentx.node2=node2;
 segmentx.next2=NO_SEGMENT;
 segmentx.way=way;
 segmentx.distance=distance;

 WriteFileBuffered(segmentsx->fd,&segmentx,sizeof(SegmentX));

 segmentsx->number++;

 logassert(segmentsx->number<SEGMENT_FAKE,"Too many segments (change index_t to 64-bits?)"); /* SEGMENT_FAKE marks the high-water mark for real segments. */
}


/*++++++++++++++++++++++++++++++++++++++
  Finish appending segments and change the filename over.

  SegmentsX *segmentsx The segments that have been appended.
  ++++++++++++++++++++++++++++++++++++++*/

void FinishSegmentList(SegmentsX *segmentsx)
{
 if(segmentsx->fd!=-1)
    segmentsx->fd=CloseFileBuffered(segmentsx->fd);
}


/*++++++++++++++++++++++++++++++++++++++
  Find the first extended segment with a particular starting node index.
 
  SegmentX *FirstSegmentX Returns a pointer to the first extended segment with the specified id.

  SegmentsX *segmentsx The set of segments to use.

  index_t nodeindex The node index to look for.

  int position A flag to pass through.
  ++++++++++++++++++++++++++++++++++++++*/

SegmentX *FirstSegmentX(SegmentsX *segmentsx,index_t nodeindex,int position)
{
 index_t index=segmentsx->firstnode[nodeindex];
 SegmentX *segmentx;

 if(index==NO_SEGMENT)
    return(NULL);

 segmentx=LookupSegmentX(segmentsx,index,position);

 return(segmentx);
}


/*++++++++++++++++++++++++++++++++++++++
  Find the next segment with a particular starting node index.

  SegmentX *NextSegmentX Returns a pointer to the next segment with the same id.

  SegmentsX *segmentsx The set of segments to use.

  SegmentX *segmentx The current segment.

  index_t nodeindex The node index.
  ++++++++++++++++++++++++++++++++++++++*/

SegmentX *NextSegmentX(SegmentsX *segmentsx,SegmentX *segmentx,index_t nodeindex)
{
#if SLIM
 int position=1+(segmentx-&segmentsx->cached[0]);
#endif

 if(segmentx->node1==nodeindex)
   {
    if(segmentsx->next1)
      {
       index_t index=IndexSegmentX(segmentsx,segmentx);

       if(segmentsx->next1[index]==NO_SEGMENT)
          return(NULL);

       segmentx=LookupSegmentX(segmentsx,segmentsx->next1[index],position);

       return(segmentx);
      }
    else
      {
#if SLIM
       index_t index=IndexSegmentX(segmentsx,segmentx);
       index++;

       if(index>=segmentsx->number)
          return(NULL);

       segmentx=LookupSegmentX(segmentsx,index,position);
#else
       segmentx++;

       if(IndexSegmentX(segmentsx,segmentx)>=segmentsx->number)
          return(NULL);
#endif

       if(segmentx->node1!=nodeindex)
          return(NULL);

       return(segmentx);
      }
   }
 else
   {
    if(segmentx->next2==NO_SEGMENT)
       return(NULL);

    return(LookupSegmentX(segmentsx,segmentx->next2,position));
   }
}
 
 
/*++++++++++++++++++++++++++++++++++++++
  Sort the segment list.

  SegmentsX *segmentsx The set of segments to sort.
  ++++++++++++++++++++++++++++++++++++++*/

void SortSegmentList(SegmentsX *segmentsx)
{
 int fd;

 /* Print the start message */

 printf_first("Sorting Segments");

 /* Re-open the file read-only and a new file writeable */

 segmentsx->fd=ReOpenFileBuffered(segmentsx->filename_tmp);

 DeleteFile(segmentsx->filename_tmp);

 fd=OpenFileBufferedNew(segmentsx->filename_tmp);

 /* Sort by node indexes */

 segmentsx->number=filesort_fixed(segmentsx->fd,fd,sizeof(SegmentX),NULL,
                                                                    (int (*)(const void*,const void*))sort_by_id,
                                                                    NULL);

 /* Close the files */

 segmentsx->fd=CloseFileBuffered(segmentsx->fd);
 CloseFileBuffered(fd);

 /* Print the final message */

 printf_last("Sorted Segments: Segments=%"Pindex_t,segmentsx->number);
}


/*++++++++++++++++++++++++++++++++++++++
  Sort the segments into id order, first by node1 then by node2, finally by distance.

  int sort_by_id Returns the comparison of the node fields.

  SegmentX *a The first segment.

  SegmentX *b The second segment.
  ++++++++++++++++++++++++++++++++++++++*/

static int sort_by_id(SegmentX *a,SegmentX *b)
{
 index_t a_id1=a->node1;
 index_t b_id1=b->node1;

 if(a_id1<b_id1)
    return(-1);
 else if(a_id1>b_id1)
    return(1);
 else /* if(a_id1==b_id1) */
   {
    index_t a_id2=a->node2;
    index_t b_id2=b->node2;

    if(a_id2<b_id2)
       return(-1);
    else if(a_id2>b_id2)
       return(1);
    else
      {
       distance_t a_distance=DISTANCE(a->distance);
       distance_t b_distance=DISTANCE(b->distance);

       if(a_distance<b_distance)
          return(-1);
       else if(a_distance>b_distance)
          return(1);
       else
         {
          distance_t a_distflag=DISTFLAG(a->distance);
          distance_t b_distflag=DISTFLAG(b->distance);

          if(a_distflag<b_distflag)
             return(-1);
          else if(a_distflag>b_distflag)
             return(1);
          else
             return(FILESORT_PRESERVE_ORDER(a,b)); /* preserve order */
         }
      }
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Process segments (non-trivial duplicates).

  SegmentsX *segmentsx The set of segments to modify.

  NodesX *nodesx The set of nodes to use.

  WaysX *waysx The set of ways to use.
  ++++++++++++++++++++++++++++++++++++++*/

void ProcessSegments(SegmentsX *segmentsx,NodesX *nodesx,WaysX *waysx)
{
 index_t duplicate=0,good=0,total=0;
 index_t prevnode1=NO_NODE,prevnode2=NO_NODE;
 index_t prevway=NO_WAY;
 distance_t prevdist=0;
 SegmentX segmentx;
 int fd;

 /* Print the start message */

 printf_first("Processing Segments: Segments=0 Duplicates=0");

 /* Map into memory /  open the file */

#if !SLIM
 nodesx->data=MapFile(nodesx->filename_tmp);
 waysx->data=MapFile(waysx->filename_tmp);
#else
 nodesx->fd=SlimMapFile(nodesx->filename_tmp);
 waysx->fd=SlimMapFile(waysx->filename_tmp);

 InvalidateNodeXCache(nodesx->cache);
 InvalidateWayXCache(waysx->cache);
#endif

 /* Allocate the way usage bitmask */

 segmentsx->usedway=AllocBitMask(waysx->number);

 logassert(segmentsx->usedway,"Failed to allocate memory (try using slim mode?)"); /* Check AllocBitMask() worked */

 /* Re-open the file read-only and a new file writeable */

 segmentsx->fd=ReOpenFileBuffered(segmentsx->filename_tmp);

 DeleteFile(segmentsx->filename_tmp);

 fd=OpenFileBufferedNew(segmentsx->filename_tmp);

 /* Modify the on-disk image */

 while(!ReadFileBuffered(segmentsx->fd,&segmentx,sizeof(SegmentX)))
   {
    NodeX *nodex1=LookupNodeX(nodesx,segmentx.node1,1);
    NodeX *nodex2=LookupNodeX(nodesx,segmentx.node2,2);

    if(prevnode1==segmentx.node1 && prevnode2==segmentx.node2)
      {
       if(prevway==segmentx.way)
         {
          WayX *wayx=LookupWayX(waysx,segmentx.way,1);

          logerror("Segment connecting nodes %"Pnode_t" and %"Pnode_t" in way %"Pway_t" is duplicated.\n",logerror_node(nodex1->id),logerror_node(nodex2->id),logerror_way(wayx->id));
         }
       else
         {
          if(!(prevdist&SEGMENT_AREA) && !(segmentx.distance&SEGMENT_AREA))
             logerror("Segment connecting nodes %"Pnode_t" and %"Pnode_t" is duplicated.\n",logerror_node(nodex1->id),logerror_node(nodex2->id));

          if(!(prevdist&SEGMENT_AREA) && (segmentx.distance&SEGMENT_AREA))
             logerror("Segment connecting nodes %"Pnode_t" and %"Pnode_t" is duplicated (discarded the area).\n",logerror_node(nodex1->id),logerror_node(nodex2->id));

          if((prevdist&SEGMENT_AREA) && !(segmentx.distance&SEGMENT_AREA))
             logerror("Segment connecting nodes %"Pnode_t" and %"Pnode_t" is duplicated (discarded the non-area).\n",logerror_node(nodex1->id),logerror_node(nodex2->id));

          if((prevdist&SEGMENT_AREA) && (segmentx.distance&SEGMENT_AREA))
             logerror("Segment connecting nodes %"Pnode_t" and %"Pnode_t" is duplicated (both are areas).\n",logerror_node(nodex1->id),logerror_node(nodex2->id));
         }

       duplicate++;
      }
    else
      {
       prevnode1=segmentx.node1;
       prevnode2=segmentx.node2;
       prevway=segmentx.way;
       prevdist=DISTANCE(segmentx.distance);

       /* Mark the ways which are used */

       SetBit(segmentsx->usedway,segmentx.way);

       /* Set the distance but keep the other flags except for area */

       segmentx.distance=DISTANCE(DistanceX(nodex1,nodex2))|DISTFLAG(segmentx.distance);
       segmentx.distance&=~SEGMENT_AREA;

       /* Write the modified segment */

       WriteFileBuffered(fd,&segmentx,sizeof(SegmentX));

       good++;
      }

    total++;

    if(!(total%10000))
       printf_middle("Processing Segments: Segments=%"Pindex_t" Duplicates=%"Pindex_t,total,duplicate);
   }

 segmentsx->number=good;

 /* Close the files */

 segmentsx->fd=CloseFileBuffered(segmentsx->fd);
 CloseFileBuffered(fd);

 /* Unmap from memory / close the file */

#if !SLIM
 nodesx->data=UnmapFile(nodesx->data);
 waysx->data=UnmapFile(waysx->data);
#else
 nodesx->fd=SlimUnmapFile(nodesx->fd);
 waysx->fd=SlimUnmapFile(waysx->fd);
#endif

 /* Print the final message */

 printf_last("Processed Segments: Segments=%"Pindex_t" Duplicates=%"Pindex_t,total,duplicate);
}


/*++++++++++++++++++++++++++++++++++++++
  Index the segments by creating the firstnode index and filling in the segment next2 parameter.

  SegmentsX *segmentsx The set of segments to modify.

  NodesX *nodesx The set of nodes to use.

  WaysX *waysx The set of ways to use.
  ++++++++++++++++++++++++++++++++++++++*/

void IndexSegments(SegmentsX *segmentsx,NodesX *nodesx,WaysX *waysx)
{
 index_t index,start=0,i;
 SegmentX *segmentx_list=NULL;
#if SLIM
 index_t length=0;
#endif

 if(segmentsx->number==0)
    return;

 /* Print the start message */

 printf_first("Indexing Segments: Segments=0");

 /* Allocate the array of indexes */

 if(segmentsx->firstnode)
    free(segmentsx->firstnode);

 segmentsx->firstnode=(index_t*)malloc(nodesx->number*sizeof(index_t));

 logassert(segmentsx->firstnode,"Failed to allocate memory (try using slim mode?)"); /* Check malloc() worked */

 for(i=0;i<nodesx->number;i++)
    segmentsx->firstnode[i]=NO_SEGMENT;

 /* Map into memory / open the files */

#if !SLIM
 segmentsx->data=MapFileWriteable(segmentsx->filename_tmp);
#else
 segmentsx->fd=SlimMapFileWriteable(segmentsx->filename_tmp);

 segmentx_list=(SegmentX*)malloc(1024*sizeof(SegmentX));
#endif

 /* Read through the segments in reverse order (in chunks to help slim mode) */

 for(index=segmentsx->number-1;index!=NO_SEGMENT;index--)
   {
    SegmentX *segmentx;

    if((index%1024)==1023 || index==(segmentsx->number-1))
      {
       start=1024*(index/1024);

#if !SLIM
       segmentx_list=LookupSegmentX(segmentsx,start,1);
#else
       length=index-start+1;

       SlimFetch(segmentsx->fd,segmentx_list,length*sizeof(SegmentX),start*sizeof(SegmentX));
#endif
      }

    segmentx=segmentx_list+(index-start);

    if(nodesx->pdata)
      {
       segmentx->node1=nodesx->pdata[segmentx->node1];
       segmentx->node2=nodesx->pdata[segmentx->node2];
      }

    if(waysx->cdata)
       segmentx->way=waysx->cdata[segmentx->way];

    segmentx->next2=segmentsx->firstnode[segmentx->node2];

    segmentsx->firstnode[segmentx->node1]=index;
    segmentsx->firstnode[segmentx->node2]=index;

    if(!(index%10000))
       printf_middle("Indexing Segments: Segments=%"Pindex_t,segmentsx->number-index);

#if SLIM
    if(index==start)
       SlimReplace(segmentsx->fd,segmentx_list,length*sizeof(SegmentX),start*sizeof(SegmentX));
#endif
   }

 /* Unmap from memory / close the files */

#if !SLIM
 segmentsx->data=UnmapFile(segmentsx->data);
#else
 segmentsx->fd=SlimUnmapFile(segmentsx->fd);

 free(segmentx_list);
#endif

 /* Free the memory */

 if(nodesx->pdata)
   {
    free(nodesx->pdata);
    nodesx->pdata=NULL;
   }

 if(waysx->cdata)
   {
    free(waysx->cdata);
    waysx->cdata=NULL;
   }

 /* Print the final message */

 printf_last("Indexed Segments: Segments=%"Pindex_t,segmentsx->number);
}


/*++++++++++++++++++++++++++++++++++++++
  Prune the deleted segments while resorting the list.

  SegmentsX *segmentsx The set of segments to sort and modify.

  WaysX *waysx The set of ways to check.
  ++++++++++++++++++++++++++++++++++++++*/

void RemovePrunedSegments(SegmentsX *segmentsx,WaysX *waysx)
{
 int fd;
 index_t xnumber;

 if(segmentsx->number==0)
    return;

 /* Print the start message */

 printf_first("Sorting and Pruning Segments");

 /* Allocate the way usage bitmask */

 segmentsx->usedway=AllocBitMask(waysx->number);

 logassert(segmentsx->usedway,"Failed to allocate memory (try using slim mode?)"); /* Check AllocBitMask() worked */

 /* Re-open the file read-only and a new file writeable */

 segmentsx->fd=ReOpenFileBuffered(segmentsx->filename_tmp);

 DeleteFile(segmentsx->filename_tmp);

 fd=OpenFileBufferedNew(segmentsx->filename_tmp);

 /* Sort by node indexes */

 xnumber=segmentsx->number;

 sortsegmentsx=segmentsx;

 segmentsx->number=filesort_fixed(segmentsx->fd,fd,sizeof(SegmentX),(int (*)(void*,index_t))delete_pruned,
                                                                    (int (*)(const void*,const void*))sort_by_id,
                                                                    NULL);

 /* Close the files */

 segmentsx->fd=CloseFileBuffered(segmentsx->fd);
 CloseFileBuffered(fd);

 /* Print the final message */

 printf_last("Sorted and Pruned Segments: Segments=%"Pindex_t" Deleted=%"Pindex_t,xnumber,xnumber-segmentsx->number);
}


/*++++++++++++++++++++++++++++++++++++++
  Delete the pruned segments.

  int delete_pruned Return 1 if the value is to be kept, otherwise 0.

  SegmentX *segmentx The extended segment.

  index_t index The number of unsorted segments that have been read from the input file.
  ++++++++++++++++++++++++++++++++++++++*/

static int delete_pruned(SegmentX *segmentx,index_t index)
{
 if(IsPrunedSegmentX(segmentx))
    return(0);

 SetBit(sortsegmentsx->usedway,segmentx->way);

 return(1);
}


/*++++++++++++++++++++++++++++++++++++++
  Remove the duplicate super-segments.

  SegmentsX *segmentsx The set of super-segments to modify.

  WaysX *waysx The set of ways to use.
  ++++++++++++++++++++++++++++++++++++++*/

void DeduplicateSuperSegments(SegmentsX *segmentsx,WaysX *waysx)
{
 int fd;
 index_t xnumber;

 if(waysx->number==0)
    return;

 /* Print the start message */

 printf_first("Sorting and Deduplicating Super-Segments");

 /* Map into memory / open the file */

#if !SLIM
 waysx->data=MapFile(waysx->filename_tmp);
#else
 waysx->fd=SlimMapFile(waysx->filename_tmp);

 InvalidateWayXCache(waysx->cache);
#endif

 /* Re-open the file read-only and a new file writeable */

 segmentsx->fd=ReOpenFileBuffered(segmentsx->filename_tmp);

 DeleteFile(segmentsx->filename_tmp);

 fd=OpenFileBufferedNew(segmentsx->filename_tmp);

 /* Sort by node indexes */

 xnumber=segmentsx->number;

 sortsegmentsx=segmentsx;
 sortwaysx=waysx;

 segmentsx->number=filesort_fixed(segmentsx->fd,fd,sizeof(SegmentX),NULL,
                                                                    (int (*)(const void*,const void*))sort_by_id,
                                                                    (int (*)(void*,index_t))deduplicate_super);

 /* Close the files */

 segmentsx->fd=CloseFileBuffered(segmentsx->fd);
 CloseFileBuffered(fd);

 /* Unmap from memory / close the file */

#if !SLIM
 waysx->data=UnmapFile(waysx->data);
#else
 waysx->fd=SlimUnmapFile(waysx->fd);
#endif

 /* Print the final message */

 printf_last("Sorted and Deduplicated Super-Segments: Super-Segments=%"Pindex_t" Duplicate=%"Pindex_t,xnumber,xnumber-segmentsx->number);
}


/*++++++++++++++++++++++++++++++++++++++
  De-duplicate super-segments.

  int deduplicate_super Return 1 if the value is to be kept, otherwise 0.

  SegmentX *segmentx The extended super-segment.

  index_t index The number of sorted super-segments that have already been written to the output file.
  ++++++++++++++++++++++++++++++++++++++*/

static int deduplicate_super(SegmentX *segmentx,index_t index)
{
 static int nprev=0;
 static index_t prevnode1=NO_NODE,prevnode2=NO_NODE;
 static SegmentX prevsegx[MAX_SEG_PER_NODE];
 static Way prevway[MAX_SEG_PER_NODE];

 WayX *wayx=LookupWayX(sortwaysx,segmentx->way,1);
 int isduplicate=0;

 if(index==0 || segmentx->node1!=prevnode1 || segmentx->node2!=prevnode2)
   {
    nprev=1;
    prevnode1=segmentx->node1;
    prevnode2=segmentx->node2;
    prevsegx[0]=*segmentx;
    prevway[0] =wayx->way;
   }
 else
   {
    int offset;

    for(offset=0;offset<nprev;offset++)
      {
       if(DISTFLAG(segmentx->distance)==DISTFLAG(prevsegx[offset].distance))
          if(!WaysCompare(&prevway[offset],&wayx->way))
            {
             isduplicate=1;
             break;
            }
      }

    if(isduplicate)
      {
       nprev--;

       for(;offset<nprev;offset++)
         {
          prevsegx[offset]=prevsegx[offset+1];
          prevway[offset] =prevway[offset+1];
         }
      }
    else
      {
       logassert(nprev<MAX_SEG_PER_NODE,"Too many segments for one node (increase MAX_SEG_PER_NODE?)"); /* Only a limited amount of information stored. */

       prevsegx[nprev]=*segmentx;
       prevway[nprev] =wayx->way;

       nprev++;
      }
   }

 return(!isduplicate);
}


/*++++++++++++++++++++++++++++++++++++++
  Sort the segments geographically after updating the node indexes.

  SegmentsX *segmentsx The set of segments to modify.

  NodesX *nodesx The set of nodes to use.
  ++++++++++++++++++++++++++++++++++++++*/

void SortSegmentListGeographically(SegmentsX *segmentsx,NodesX *nodesx)
{
 int fd;

 if(segmentsx->number==0)
    return;

 /* Print the start message */

 printf_first("Sorting Segments Geographically");

 /* Re-open the file read-only and a new file writeable */

 segmentsx->fd=ReOpenFileBuffered(segmentsx->filename_tmp);

 DeleteFile(segmentsx->filename_tmp);

 fd=OpenFileBufferedNew(segmentsx->filename_tmp);

 /* Update the segments with geographically sorted node indexes and sort them */

 sortnodesx=nodesx;

 filesort_fixed(segmentsx->fd,fd,sizeof(SegmentX),(int (*)(void*,index_t))geographically_index,
                                                  (int (*)(const void*,const void*))sort_by_id,
                                                  NULL);
 /* Close the files */

 segmentsx->fd=CloseFileBuffered(segmentsx->fd);
 CloseFileBuffered(fd);

 /* Print the final message */

 printf_last("Sorted Segments Geographically: Segments=%"Pindex_t,segmentsx->number);
}


/*++++++++++++++++++++++++++++++++++++++
  Update the segment indexes.

  int geographically_index Return 1 if the value is to be kept, otherwise 0.

  SegmentX *segmentx The extended segment.

  index_t index The number of unsorted segments that have been read from the input file.
  ++++++++++++++++++++++++++++++++++++++*/

static int geographically_index(SegmentX *segmentx,index_t index)
{
 segmentx->node1=sortnodesx->gdata[segmentx->node1];
 segmentx->node2=sortnodesx->gdata[segmentx->node2];

 if(segmentx->node1>segmentx->node2)
   {
    index_t temp;

    temp=segmentx->node1;
    segmentx->node1=segmentx->node2;
    segmentx->node2=temp;

    if(segmentx->distance&(ONEWAY_2TO1|ONEWAY_1TO2))
       segmentx->distance^=ONEWAY_2TO1|ONEWAY_1TO2;
   }

 return(1);
}


/*++++++++++++++++++++++++++++++++++++++
  Save the segment list to a file.

  SegmentsX *segmentsx The set of segments to save.

  const char *filename The name of the file to save.
  ++++++++++++++++++++++++++++++++++++++*/

void SaveSegmentList(SegmentsX *segmentsx,const char *filename)
{
 index_t i;
 int fd;
 SegmentsFile segmentsfile={0};
 index_t super_number=0,normal_number=0;

 /* Print the start message */

 printf_first("Writing Segments: Segments=0");

 /* Re-open the file */

 segmentsx->fd=ReOpenFileBuffered(segmentsx->filename_tmp);

 /* Write out the segments data */

 fd=OpenFileBufferedNew(filename);

 SeekFileBuffered(fd,sizeof(SegmentsFile));

 for(i=0;i<segmentsx->number;i++)
   {
    SegmentX segmentx;
    Segment  segment={0};

    ReadFileBuffered(segmentsx->fd,&segmentx,sizeof(SegmentX));

    segment.node1   =segmentx.node1;
    segment.node2   =segmentx.node2;
    segment.next2   =segmentx.next2;
    segment.way     =segmentx.way;
    segment.distance=segmentx.distance;

    if(IsSuperSegment(&segment))
       super_number++;
    if(IsNormalSegment(&segment))
       normal_number++;

    WriteFileBuffered(fd,&segment,sizeof(Segment));

    if(!((i+1)%10000))
       printf_middle("Writing Segments: Segments=%"Pindex_t,i+1);
   }

 /* Write out the header structure */

 segmentsfile.number=segmentsx->number;
 segmentsfile.snumber=super_number;
 segmentsfile.nnumber=normal_number;

 SeekFileBuffered(fd,0);
 WriteFileBuffered(fd,&segmentsfile,sizeof(SegmentsFile));

 CloseFileBuffered(fd);

 /* Close the file */

 segmentsx->fd=CloseFileBuffered(segmentsx->fd);

 /* Print the final message */

 printf_last("Wrote Segments: Segments=%"Pindex_t,segmentsx->number);
}


/*++++++++++++++++++++++++++++++++++++++
  Calculate the distance between two nodes.

  distance_t DistanceX Returns the distance between the extended nodes.

  NodeX *nodex1 The starting node.

  NodeX *nodex2 The end node.
  ++++++++++++++++++++++++++++++++++++++*/

static distance_t DistanceX(NodeX *nodex1,NodeX *nodex2)
{
 double dlon = latlong_to_radians(nodex1->longitude) - latlong_to_radians(nodex2->longitude);
 double dlat = latlong_to_radians(nodex1->latitude)  - latlong_to_radians(nodex2->latitude);
 double lat1 = latlong_to_radians(nodex1->latitude);
 double lat2 = latlong_to_radians(nodex2->latitude);

 double a1,a2,a,sa,c,d;

 if(dlon==0 && dlat==0)
   return 0;

 a1 = sin (dlat / 2);
 a2 = sin (dlon / 2);
 a = (a1 * a1) + cos (lat1) * cos (lat2) * a2 * a2;
 sa = sqrt (a);
 if (sa <= 1.0)
   {c = 2 * asin (sa);}
 else
   {c = 2 * asin (1.0);}
 d = 6378.137 * c;

 return km_to_distance(d);
}

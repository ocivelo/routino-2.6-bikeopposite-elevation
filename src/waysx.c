/***************************************
 Extended Way data type functions.

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


#include <stdlib.h>
#include <string.h>

#include "types.h"
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
static WaysX *sortwaysx;
static SegmentsX *sortsegmentsx;

/* Local functions */

static int sort_by_id(WayX *a,WayX *b);
static int deduplicate_and_index_by_id(WayX *wayx,index_t index);

static int sort_by_name(char *a,char *b);

static int delete_unused(WayX *wayx,index_t index);
static int sort_by_name_and_prop_and_id(WayX *a,WayX *b);
static int deduplicate_and_index_by_compact_id(WayX *wayx,index_t index);


/*++++++++++++++++++++++++++++++++++++++
  Allocate a new way list (create a new file or open an existing one).

  WaysX *NewWayList Returns the way list.

  int append Set to 1 if the file is to be opened for appending.

  int readonly Set to 1 if the file is to be opened for reading.
  ++++++++++++++++++++++++++++++++++++++*/

WaysX *NewWayList(int append,int readonly)
{
 WaysX *waysx;

 waysx=(WaysX*)calloc(1,sizeof(WaysX));

 logassert(waysx,"Failed to allocate memory (try using slim mode?)"); /* Check calloc() worked */

 waysx->filename    =(char*)malloc(strlen(option_tmpdirname)+32);
 waysx->filename_tmp=(char*)malloc(strlen(option_tmpdirname)+32);

 sprintf(waysx->filename    ,"%s/waysx.parsed.mem",option_tmpdirname);
 sprintf(waysx->filename_tmp,"%s/waysx.%p.tmp"    ,option_tmpdirname,(void*)waysx);

 if(append || readonly)
    if(ExistsFile(waysx->filename))
      {
       FILESORT_VARINT waysize;
       int fd;

       fd=ReOpenFileBuffered(waysx->filename);

       while(!ReadFileBuffered(fd,&waysize,FILESORT_VARSIZE))
         {
          SkipFileBuffered(fd,waysize);

          waysx->number++;
         }

       CloseFileBuffered(fd);

       RenameFile(waysx->filename,waysx->filename_tmp);
      }

 if(append)
    waysx->fd=OpenFileBufferedAppend(waysx->filename_tmp);
 else if(!readonly)
    waysx->fd=OpenFileBufferedNew(waysx->filename_tmp);
 else
    waysx->fd=-1;

#if SLIM
 waysx->cache=NewWayXCache();
#endif


 waysx->nfilename_tmp=(char*)malloc(strlen(option_tmpdirname)+32);

 sprintf(waysx->nfilename_tmp,"%s/waynames.%p.tmp",option_tmpdirname,(void*)waysx);

 return(waysx);
}


/*++++++++++++++++++++++++++++++++++++++
  Free a way list.

  WaysX *waysx The set of ways to be freed.

  int keep If set then the results file is to be kept.
  ++++++++++++++++++++++++++++++++++++++*/

void FreeWayList(WaysX *waysx,int keep)
{
 if(keep)
    RenameFile(waysx->filename_tmp,waysx->filename);
 else
    DeleteFile(waysx->filename_tmp);

 free(waysx->filename);
 free(waysx->filename_tmp);

 if(waysx->idata)
    free(waysx->idata);

 if(waysx->odata)
    free(waysx->odata);

 if(waysx->cdata)
    free(waysx->cdata);

 DeleteFile(waysx->nfilename_tmp);

 free(waysx->nfilename_tmp);

#if SLIM
 DeleteWayXCache(waysx->cache);
#endif

 free(waysx);
}


/*++++++++++++++++++++++++++++++++++++++
  Append a single way to an unsorted way list.

  WaysX *waysx The set of ways to process.

  way_t id The ID of the way.

  Way *way The way data itself.

  node_t *nodes The list of nodes for this way.

  int nnodes The number of nodes for this way.

  const char *name The name or reference of the way.
  ++++++++++++++++++++++++++++++++++++++*/

void AppendWayList(WaysX *waysx,way_t id,Way *way,node_t *nodes,int nnodes,const char *name)
{
 WayX wayx;
 FILESORT_VARINT size;
 node_t nonode=NO_NODE_ID;

 wayx.id=id;
 wayx.way=*way;

 size=sizeof(WayX)+(nnodes+1)*sizeof(node_t)+strlen(name)+1;

 WriteFileBuffered(waysx->fd,&size,FILESORT_VARSIZE);
 WriteFileBuffered(waysx->fd,&wayx,sizeof(WayX));

 WriteFileBuffered(waysx->fd,nodes  ,nnodes*sizeof(node_t));
 WriteFileBuffered(waysx->fd,&nonode,       sizeof(node_t));

 WriteFileBuffered(waysx->fd,name,strlen(name)+1);

 waysx->number++;

 logassert(waysx->number!=0,"Too many ways (change index_t to 64-bits?)"); /* Zero marks the high-water mark for ways. */
}


/*++++++++++++++++++++++++++++++++++++++
  Finish appending ways and change the filename over.

  WaysX *waysx The ways that have been appended.
  ++++++++++++++++++++++++++++++++++++++*/

void FinishWayList(WaysX *waysx)
{
 if(waysx->fd!=-1)
    waysx->fd=CloseFileBuffered(waysx->fd);
}


/*++++++++++++++++++++++++++++++++++++++
  Find a particular way index.

  index_t IndexWayX Returns the index of the extended way with the specified id.

  WaysX *waysx The set of ways to process.

  way_t id The way id to look for.
  ++++++++++++++++++++++++++++++++++++++*/

index_t IndexWayX(WaysX *waysx,way_t id)
{
 index_t start=0;
 index_t end=waysx->number-1;
 index_t mid;

 if(waysx->number==0)           /* There are no ways */
    return(NO_WAY);

 if(id<waysx->idata[start])     /* Key is before start */
    return(NO_WAY);

 if(id>waysx->idata[end])       /* Key is after end */
    return(NO_WAY);

 /* Binary search - search key exact match only is required.
  *
  *  # <- start  |  Check mid and move start or end if it doesn't match
  *  #           |
  *  #           |  Since an exact match is wanted we can set end=mid-1
  *  # <- mid    |  or start=mid+1 because we know that mid doesn't match.
  *  #           |
  *  #           |  Eventually either end=start or end=start+1 and one of
  *  # <- end    |  start or end is the wanted one.
  */

 do
   {
    mid=(start+end)/2;            /* Choose mid point */

    if(waysx->idata[mid]<id)      /* Mid point is too low */
       start=mid+1;
    else if(waysx->idata[mid]>id) /* Mid point is too high */
       end=mid?(mid-1):mid;
    else                          /* Mid point is correct */
       return(mid);
   }
 while((end-start)>1);

 if(waysx->idata[start]==id)      /* Start is correct */
    return(start);

 if(waysx->idata[end]==id)        /* End is correct */
    return(end);

 return(NO_WAY);
}


/*++++++++++++++++++++++++++++++++++++++
  Sort the list of ways.

  WaysX *waysx The set of ways to process.
  ++++++++++++++++++++++++++++++++++++++*/

void SortWayList(WaysX *waysx)
{
 index_t xnumber;
 int fd;

 /* Print the start message */

 printf_first("Sorting Ways");

 /* Re-open the file read-only and a new file writeable */

 waysx->fd=ReOpenFileBuffered(waysx->filename_tmp);

 DeleteFile(waysx->filename_tmp);

 fd=OpenFileBufferedNew(waysx->filename_tmp);

 /* Allocate the array of indexes */

 waysx->idata=(way_t*)malloc(waysx->number*sizeof(way_t));

 logassert(waysx->idata,"Failed to allocate memory (try using slim mode?)"); /* Check malloc() worked */

 /* Sort the ways by ID and index them */

 sortwaysx=waysx;

 xnumber=waysx->number;

 waysx->number=filesort_vary(waysx->fd,fd,NULL,
                                          (int (*)(const void*,const void*))sort_by_id,
                                          (int (*)(void*,index_t))deduplicate_and_index_by_id);

 waysx->knumber=waysx->number;

 /* Close the files */

 waysx->fd=CloseFileBuffered(waysx->fd);
 CloseFileBuffered(fd);

 /* Print the final message */

 printf_last("Sorted Ways: Ways=%"Pindex_t" Duplicates=%"Pindex_t,xnumber,xnumber-waysx->number);
}


/*++++++++++++++++++++++++++++++++++++++
  Sort the ways into id order.

  int sort_by_id Returns the comparison of the id fields.

  WayX *a The first extended way.

  WayX *b The second extended way.
  ++++++++++++++++++++++++++++++++++++++*/

static int sort_by_id(WayX *a,WayX *b)
{
 way_t a_id=a->id;
 way_t b_id=b->id;

 if(a_id<b_id)
    return(-1);
 else if(a_id>b_id)
    return(1);
 else
    return(-FILESORT_PRESERVE_ORDER(a,b)); /* latest version first */
}


/*++++++++++++++++++++++++++++++++++++++
  Discard duplicate ways and create and index of ids.

  int deduplicate_and_index_by_id Return 1 if the value is to be kept, otherwise 0.

  WayX *wayx The extended way.

  index_t index The number of sorted ways that have already been written to the output file.
  ++++++++++++++++++++++++++++++++++++++*/

static int deduplicate_and_index_by_id(WayX *wayx,index_t index)
{
 static way_t previd=NO_WAY_ID;

 if(wayx->id!=previd)
   {
    previd=wayx->id;

    if(wayx->way.type==WAY_DELETED)
       return(0);
    else
      {
       sortwaysx->idata[index]=wayx->id;

       return(1);
      }
   }
 else
    return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  Split the ways into segments and way names.

  SegmentsX *SplitWays Returns the set of segments that have been created.

  WaysX *waysx The set of ways to process.

  NodesX *nodesx The set of nodes to use.

  int keep If set to 1 then keep the old data file otherwise delete it.
  ++++++++++++++++++++++++++++++++++++++*/

SegmentsX *SplitWays(WaysX *waysx,NodesX *nodesx,int keep)
{
 SegmentsX *segmentsx;
 index_t i;
 int fd,nfd;
 char *name=NULL;
 int namelen=0;

 /* Print the start message */

 printf_first("Splitting Ways: Ways=0 Segments=0");

 segmentsx=NewSegmentList();

 /* Re-open the file read-only and a new file writeable */

 waysx->fd=ReOpenFileBuffered(waysx->filename_tmp);

 if(keep)
    RenameFile(waysx->filename_tmp,waysx->filename);
 else
    DeleteFile(waysx->filename_tmp);

 fd=OpenFileBufferedNew(waysx->filename_tmp);

 nfd=OpenFileBufferedNew(waysx->nfilename_tmp);

 /* Loop through the ways and create the segments and way names */

 for(i=0;i<waysx->number;i++)
   {
    WayX wayx;
    FILESORT_VARINT size;
    node_t node,prevnode=NO_NODE_ID;
    index_t index,previndex=NO_NODE;

    ReadFileBuffered(waysx->fd,&size,FILESORT_VARSIZE);

    ReadFileBuffered(waysx->fd,&wayx,sizeof(WayX));

    waysx->allow|=wayx.way.allow;

    while(!ReadFileBuffered(waysx->fd,&node,sizeof(node_t)) && node!=NO_NODE_ID)
      {
       index=IndexNodeX(nodesx,node);

       if(prevnode==node)
         {
          logerror("Way %"Pway_t" contains node %"Pnode_t" that is connected to itself.\n",logerror_way(wayx.id),logerror_node(node));
         }
       else if(index==NO_NODE)
         {
          logerror("Way %"Pway_t" contains node %"Pnode_t" that does not exist in the Routino database.\n",logerror_way(wayx.id),logerror_node(node));
         }
       else if(previndex==NO_NODE)
          ;
       else
         {
          distance_t segment_flags=0;

          if(wayx.way.type&Highway_OneWay)
             segment_flags|=ONEWAY_1TO2;

          if(wayx.way.type&Highway_Area)
             segment_flags|=SEGMENT_AREA;

          AppendSegmentList(segmentsx,i,previndex,index,segment_flags);
         }

       prevnode=node;
       previndex=index;

       size-=sizeof(node_t);
      }

    size-=sizeof(node_t)+sizeof(WayX);

    if(namelen<size)
       name=(char*)realloc((void*)name,namelen=size);

    ReadFileBuffered(waysx->fd,name,size);

    WriteFileBuffered(fd,&wayx,sizeof(WayX));

    size+=sizeof(index_t);

    WriteFileBuffered(nfd,&size,FILESORT_VARSIZE);
    WriteFileBuffered(nfd,&i,sizeof(index_t));
    WriteFileBuffered(nfd,name,size-sizeof(index_t));

    if(!((i+1)%1000))
       printf_middle("Splitting Ways: Ways=%"Pindex_t" Segments=%"Pindex_t,i+1,segmentsx->number);
   }

 FinishSegmentList(segmentsx);

 if(name) free(name);

 /* Close the files */

 waysx->fd=CloseFileBuffered(waysx->fd);
 CloseFileBuffered(fd);

 CloseFileBuffered(nfd);

 /* Print the final message */

 printf_last("Split Ways: Ways=%"Pindex_t" Segments=%"Pindex_t,waysx->number,segmentsx->number);

 return(segmentsx);
}



/*++++++++++++++++++++++++++++++++++++++
  Sort the way names and assign the offsets to the ways.

  WaysX *waysx The set of ways to process.
  ++++++++++++++++++++++++++++++++++++++*/

void SortWayNames(WaysX *waysx)
{
 index_t i;
 int nfd;
 char *names[2]={NULL,NULL};
 int namelen[2]={0,0};
 int nnames=0;
 uint32_t lastlength=0;

 /* Print the start message */

 printf_first("Sorting Way Names");

 /* Re-open the file read-only and new file writeable */

 waysx->nfd=ReOpenFileBuffered(waysx->nfilename_tmp);

 DeleteFile(waysx->nfilename_tmp);

 nfd=OpenFileBufferedNew(waysx->nfilename_tmp);

 /* Sort the way names */

 waysx->nlength=0;

 filesort_vary(waysx->nfd,nfd,NULL,
                              (int (*)(const void*,const void*))sort_by_name,
                              NULL);

 /* Close the files */

 waysx->nfd=CloseFileBuffered(waysx->nfd);
 CloseFileBuffered(nfd);

 /* Print the final message */

 printf_last("Sorted Way Names: Ways=%"Pindex_t,waysx->number);


 /* Print the start message */

 printf_first("Updating Ways with Names: Ways=0 Names=0");

 /* Map into memory /  open the file */

#if !SLIM
 waysx->data=MapFileWriteable(waysx->filename_tmp);
#else
 waysx->fd=SlimMapFileWriteable(waysx->filename_tmp);
#endif

 /* Re-open the file read-only and new file writeable */

 waysx->nfd=ReOpenFileBuffered(waysx->nfilename_tmp);

 DeleteFile(waysx->nfilename_tmp);

 nfd=OpenFileBufferedNew(waysx->nfilename_tmp);

 /* Update the ways and de-duplicate the names */

 for(i=0;i<waysx->number;i++)
   {
    WayX *wayx;
    index_t index;
    FILESORT_VARINT size;

    ReadFileBuffered(waysx->nfd,&size,FILESORT_VARSIZE);

    if(namelen[nnames%2]<size)
       names[nnames%2]=(char*)realloc((void*)names[nnames%2],namelen[nnames%2]=size);

    ReadFileBuffered(waysx->nfd,&index,sizeof(index_t));
    ReadFileBuffered(waysx->nfd,names[nnames%2],size-sizeof(index_t));

    if(nnames==0 || strcmp(names[0],names[1]))
      {
       WriteFileBuffered(nfd,names[nnames%2],size-sizeof(index_t));

       lastlength=waysx->nlength;
       waysx->nlength+=size-sizeof(index_t);

       nnames++;
      }

    wayx=LookupWayX(waysx,index,1);

    wayx->way.name=lastlength;

    PutBackWayX(waysx,wayx);

    if(!((i+1)%1000))
       printf_middle("Updating Ways with Names: Ways=%"Pindex_t" Names=%"Pindex_t,i+1,nnames);
   }

 if(names[0]) free(names[0]);
 if(names[1]) free(names[1]);

 /* Close the files */

 waysx->nfd=CloseFileBuffered(waysx->nfd);
 CloseFileBuffered(nfd);

 /* Unmap from memory / close the files */

#if !SLIM
 waysx->data=UnmapFile(waysx->data);
#else
 waysx->fd=SlimUnmapFile(waysx->fd);
#endif

 /* Print the final message */

 printf_last("Updated Ways with Names: Ways=%"Pindex_t" Names=%"Pindex_t,waysx->number,nnames);
}


/*++++++++++++++++++++++++++++++++++++++
  Sort the ways into name order.

  int sort_by_name Returns the comparison of the name fields.

  char *a The first way name.

  char *b The second way name.
  ++++++++++++++++++++++++++++++++++++++*/

static int sort_by_name(char *a,char *b)
{
 int compare;
 char *a_name=a+sizeof(index_t);
 char *b_name=b+sizeof(index_t);

 compare=strcmp(a_name,b_name);

 if(compare)
    return(compare);
 else
    return(FILESORT_PRESERVE_ORDER(a,b));
}


/*++++++++++++++++++++++++++++++++++++++
  Compact the way list, removing duplicated ways and unused ways.

  WaysX *waysx The set of ways to process.

  SegmentsX *segmentsx The set of segments to check.
  ++++++++++++++++++++++++++++++++++++++*/

void CompactWayList(WaysX *waysx,SegmentsX *segmentsx)
{
 int fd;
 index_t cnumber;

 if(waysx->number==0)
    return;

 /* Print the start message */

 printf_first("Sorting Ways and Compacting");

 /* Allocate the array of indexes */

 waysx->cdata=(index_t*)malloc(waysx->number*sizeof(index_t));

 logassert(waysx->cdata,"Failed to allocate memory (try using slim mode?)"); /* Check malloc() worked */

 /* Re-open the file read-only and a new file writeable */

 waysx->fd=ReOpenFileBuffered(waysx->filename_tmp);

 DeleteFile(waysx->filename_tmp);

 fd=OpenFileBufferedNew(waysx->filename_tmp);

 /* Sort the ways to allow compacting according to the properties */

 sortwaysx=waysx;
 sortsegmentsx=segmentsx;

 cnumber=filesort_fixed(waysx->fd,fd,sizeof(WayX),(int (*)(void*,index_t))delete_unused,
                                                  (int (*)(const void*,const void*))sort_by_name_and_prop_and_id,
                                                  (int (*)(void*,index_t))deduplicate_and_index_by_compact_id);

 /* Close the files */

 waysx->fd=CloseFileBuffered(waysx->fd);
 CloseFileBuffered(fd);

 /* Free the data */

 free(segmentsx->usedway);
 segmentsx->usedway=NULL;

 /* Print the final message */

 printf_last("Sorted and Compacted Ways: Ways=%"Pindex_t" Unique=%"Pindex_t,waysx->number,cnumber);
 waysx->number=cnumber;
}


/*++++++++++++++++++++++++++++++++++++++
  Delete the ways that are no longer being used.

  int delete_unused Return 1 if the value is to be kept, otherwise 0.

  WayX *wayx The extended way.

  index_t index The number of unsorted ways that have been read from the input file.
  ++++++++++++++++++++++++++++++++++++++*/

static int delete_unused(WayX *wayx,index_t index)
{
 if(sortsegmentsx && !IsBitSet(sortsegmentsx->usedway,index))
   {
    sortwaysx->cdata[index]=NO_WAY;

    return(0);
   }
 else
   {
    wayx->id=index;

    return(1);
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Sort the ways into name, properties and id order.

  int sort_by_name_and_prop_and_id Returns the comparison of the name, properties and id fields.

  WayX *a The first extended Way.

  WayX *b The second extended Way.
  ++++++++++++++++++++++++++++++++++++++*/

static int sort_by_name_and_prop_and_id(WayX *a,WayX *b)
{
 int compare;
 index_t a_name=a->way.name;
 index_t b_name=b->way.name;

 if(a_name<b_name)
    return(-1);
 else if(a_name>b_name)
    return(1);

 compare=WaysCompare(&a->way,&b->way);

 if(compare)
    return(compare);

 return(sort_by_id(a,b));
}


/*++++++++++++++++++++++++++++++++++++++
  Create the index of compacted Way identifiers and ignore Ways with duplicated properties.

  int deduplicate_and_index_by_compact_id Return 1 if the value is to be kept, otherwise 0.

  WayX *wayx The extended way.

  index_t index The number of sorted ways that have already been written to the output file.
  ++++++++++++++++++++++++++++++++++++++*/

static int deduplicate_and_index_by_compact_id(WayX *wayx,index_t index)
{
 static Way lastway;

 if(index==0 || wayx->way.name!=lastway.name || WaysCompare(&lastway,&wayx->way))
   {
    lastway=wayx->way;

    sortwaysx->cdata[wayx->id]=index;

    wayx->id=index;

    return(1);
   }
 else
   {
    sortwaysx->cdata[wayx->id]=index-1;

    return(0);
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Save the way list to a file.

  WaysX *waysx The set of ways to save.

  const char *filename The name of the file to save.
  ++++++++++++++++++++++++++++++++++++++*/

void SaveWayList(WaysX *waysx,const char *filename)
{
 index_t i;
 int fd;
 index_t position=0;
 WayX wayx;
 WaysFile waysfile={0};
 highways_t   highways=0;
 transports_t allow=0;
 properties_t props=0;

 /* Print the start message */

 printf_first("Writing Ways: Ways=0");

 /* Re-open the files */

 waysx->fd=ReOpenFileBuffered(waysx->filename_tmp);
 waysx->nfd=ReOpenFileBuffered(waysx->nfilename_tmp);

 /* Write out the ways data */

 fd=OpenFileBufferedNew(filename);

 SeekFileBuffered(fd,sizeof(WaysFile));

 for(i=0;i<waysx->number;i++)
   {
    ReadFileBuffered(waysx->fd,&wayx,sizeof(WayX));

    highways|=HIGHWAYS(wayx.way.type);
    allow   |=wayx.way.allow;
    props   |=wayx.way.props;

    WriteFileBuffered(fd,&wayx.way,sizeof(Way));

    if(!((i+1)%1000))
       printf_middle("Writing Ways: Ways=%"Pindex_t,i+1);
   }

 /* Write out the ways names */

 SeekFileBuffered(fd,sizeof(WaysFile)+(off_t)waysx->number*sizeof(Way));

 while(position<waysx->nlength)
   {
    size_t len=1024;
    char temp[1024];

    if((waysx->nlength-position)<1024)
       len=waysx->nlength-position;

    ReadFileBuffered(waysx->nfd,temp,len);

    WriteFileBuffered(fd,temp,len);

    position+=len;
   }

 /* Close the files */

 waysx->fd=CloseFileBuffered(waysx->fd);
 waysx->nfd=CloseFileBuffered(waysx->nfd);

 /* Write out the header structure */

 waysfile.number =waysx->number;

 waysfile.highways=highways;
 waysfile.allow   =allow;
 waysfile.props   =props;

 SeekFileBuffered(fd,0);
 WriteFileBuffered(fd,&waysfile,sizeof(WaysFile));

 CloseFileBuffered(fd);

 /* Print the final message */

 printf_last("Wrote Ways: Ways=%"Pindex_t,waysx->number);
}

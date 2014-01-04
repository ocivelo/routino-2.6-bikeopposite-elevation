/***************************************
 Error log processing functions.

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


#include "typesx.h"
#include "nodesx.h"
#include "waysx.h"
#include "relationsx.h"

#include "errorlogx.h"
#include "errorlog.h"

#include "files.h"
#include "sorting.h"


/* Global variables */

/*+ The name of the error log file. +*/
extern char *errorlogfilename;

/*+ The name of the binary error log file. +*/
extern char *errorbinfilename;

/* Local variables */

/*+ Temporary file-local variables for use by the sort functions. +*/
static latlong_t lat_min,lat_max,lon_min,lon_max;

/* Local functions */

static void reindex_nodes(NodesX *nodesx);
static void reindex_ways(WaysX *waysx);
static void reindex_relations(RelationsX *relationsx);

static int lookup_lat_long_node(NodesX *nodesx,node_t node,latlong_t *latitude,latlong_t *longitude);
static int lookup_lat_long_way(WaysX *waysx,NodesX *nodesx,way_t way,latlong_t *latitude,latlong_t *longitude,index_t error);
static int lookup_lat_long_relation(RelationsX *relationsx,WaysX *waysx,NodesX *nodesx,relation_t relation,latlong_t *latitude,latlong_t *longitude,index_t error);

static int sort_by_lat_long(ErrorLogX *a,ErrorLogX *b);
static int measure_lat_long(ErrorLogX *errorlogx,index_t index);


/*++++++++++++++++++++++++++++++++++++++
  Allocate a new error log list (create a new file).

  ErrorLogsX *NewErrorLogList Returns a pointer to the error log list.
  ++++++++++++++++++++++++++++++++++++++*/

ErrorLogsX *NewErrorLogList(void)
{
 ErrorLogsX *errorlogsx;

 errorlogsx=(ErrorLogsX*)calloc(1,sizeof(ErrorLogsX));

 logassert(errorlogsx,"Failed to allocate memory (try using slim mode?)"); /* Check calloc() worked */

 return(errorlogsx);
}


/*++++++++++++++++++++++++++++++++++++++
  Free an error log list.

  ErrorLogsX *errorlogsx The set of error logs to be freed.
  ++++++++++++++++++++++++++++++++++++++*/

void FreeErrorLogList(ErrorLogsX *errorlogsx)
{
 free(errorlogsx);
}


/*++++++++++++++++++++++++++++++++++++++
  Process the binary error log.

  ErrorLogsX *errorlogsx The set of error logs to update.

  NodesX *nodesx The set of nodes.

  WaysX *waysx The set of ways.

  RelationsX *relationsx The set of relations.
  ++++++++++++++++++++++++++++++++++++++*/

void ProcessErrorLogs(ErrorLogsX *errorlogsx,NodesX *nodesx,WaysX *waysx,RelationsX *relationsx)
{
 int oldfd,newfd;
 uint32_t offset=0;
 int nerrorlogobjects=0;
 int finished;
 ErrorLogObject errorlogobjects[8];

 /* Re-index the nodes, ways and relations */

 printf_first("Re-indexing the Data: Nodes=0 Ways=0 Route-Relations=0 Turn-Relations=0");

 reindex_nodes(nodesx);

 printf_middle("Re-indexing the Data: Nodes=%"Pindex_t" Ways=0 Route-Relations=0 Turn-Relations=0",nodesx->number);

 reindex_ways(waysx);

 printf_middle("Re-indexing the Data: Nodes=%"Pindex_t" Ways=%"Pindex_t" Route-Relations=0 Turn-Relations=0",nodesx->number,waysx->number);

 reindex_relations(relationsx);

 printf_last("Re-indexed the Data: Nodes=%"Pindex_t" Ways=%"Pindex_t" Route-Relations=%"Pindex_t" Turn-Relations=%"Pindex_t,nodesx->number,waysx->number,relationsx->rrnumber,relationsx->trnumber);


 /* Print the start message */

 printf_first("Calculating Coordinates: Errors=0");

 /* Map into memory / open the files */

#if !SLIM
 nodesx->data=MapFile(nodesx->filename);
#else
 nodesx->fd=SlimMapFile(nodesx->filename);

 InvalidateNodeXCache(nodesx->cache);
#endif

 waysx->fd=ReOpenFileBuffered(waysx->filename);
 relationsx->rrfd=ReOpenFileBuffered(relationsx->rrfilename);
 relationsx->trfd=ReOpenFileBuffered(relationsx->trfilename);

 /* Open the binary log file read-only and a new file writeable */

 oldfd=ReOpenFileBuffered(errorbinfilename);

 DeleteFile(errorbinfilename);

 newfd=OpenFileBufferedNew(errorbinfilename);

 /* Loop through the file and merge the raw data into coordinates */

 errorlogsx->number=0;

 do
   {
    ErrorLogObject errorlogobject;

    finished=ReadFileBuffered(oldfd,&errorlogobject,sizeof(ErrorLogObject));

    if(finished)
       errorlogobject.offset=SizeFile(errorlogfilename);

    if(offset!=errorlogobject.offset)
      {
       ErrorLogX errorlogx;
       latlong_t errorlat=NO_LATLONG,errorlon=NO_LATLONG;

       /* Calculate suitable coordinates */

       if(nerrorlogobjects==1)
         {
          if(errorlogobjects[0].type=='N')
            {
             node_t node=(node_t)errorlogobjects[0].id;

             lookup_lat_long_node(nodesx,node,&errorlat,&errorlon);
            }
          else if(errorlogobjects[0].type=='W')
            {
             way_t way=(way_t)errorlogobjects[0].id;

             lookup_lat_long_way(waysx,nodesx,way,&errorlat,&errorlon,errorlogsx->number);
            }
          else if(errorlogobjects[0].type=='R')
            {
             relation_t relation=(relation_t)errorlogobjects[0].type;

             lookup_lat_long_relation(relationsx,waysx,nodesx,relation,&errorlat,&errorlon,errorlogsx->number);
            }
         }
       else
         {
          latlong_t latitude[8],longitude[8];
          int i;
          int ncoords=0,nnodes=0,nways=0,nrelations=0;

          for(i=0;i<nerrorlogobjects;i++)
            {
             if(errorlogobjects[i].type=='N')
               {
                node_t node=(node_t)errorlogobjects[i].id;

                if(lookup_lat_long_node(nodesx,node,&latitude[ncoords],&longitude[ncoords]))
                   ncoords++;

                nnodes++;
               }
             else if(errorlogobjects[i].type=='W')
                nways++;
             else if(errorlogobjects[i].type=='R')
                nrelations++;
            }

          if(nways==0 && nrelations==0) /* only nodes */
             ;
          else if(ncoords)      /* some good nodes, possibly ways and/or relations */
             ;
          else if(nways)        /* no good nodes, possibly some good ways */
            {
             for(i=0;i<nerrorlogobjects;i++)
                if(errorlogobjects[i].type=='W')
                  {
                   way_t way=(way_t)errorlogobjects[i].id;

                   if(lookup_lat_long_way(waysx,nodesx,way,&latitude[ncoords],&longitude[ncoords],errorlogsx->number))
                      ncoords++;
                  }
            }

          if(nrelations==0) /* only nodes and/or ways */
             ;
          else if(ncoords)  /* some good nodes and/or ways, possibly relations */
             ;
          else /* if(nrelations) */
            {
             for(i=0;i<nerrorlogobjects;i++)
                if(errorlogobjects[i].type=='R')
                  {
                   relation_t relation=(relation_t)errorlogobjects[i].id;

                   if(lookup_lat_long_relation(relationsx,waysx,nodesx,relation,&latitude[ncoords],&longitude[ncoords],errorlogsx->number))
                      ncoords++;
                  }
            }

          if(ncoords)
            {
             errorlat=0;
             errorlon=0;

             for(i=0;i<ncoords;i++)
               {
                errorlat+=latitude[i];
                errorlon+=longitude[i];
               }

             errorlat/=ncoords;
             errorlon/=ncoords;
            }
          else
            {
             errorlat=NO_LATLONG;
             errorlon=NO_LATLONG;
            }
         }

       /* Write to file */

       errorlogx.offset=offset;
       errorlogx.length=errorlogobject.offset-offset;

       errorlogx.latitude =errorlat;
       errorlogx.longitude=errorlon;

       WriteFileBuffered(newfd,&errorlogx,sizeof(ErrorLogX));

       errorlogsx->number++;

       offset=errorlogobject.offset;
       nerrorlogobjects=0;

       if(!(errorlogsx->number%10000))
          printf_middle("Calculating Coordinates: Errors=%"Pindex_t,errorlogsx->number);
      }

    /* Store for later */

    logassert(nerrorlogobjects<8,"Too many error log objects for one error message."); /* Only a limited amount of information stored. */

    errorlogobjects[nerrorlogobjects]=errorlogobject;

    nerrorlogobjects++;
   }
 while(!finished);

 /* Unmap from memory / close the files */

#if !SLIM
 nodesx->data=UnmapFile(nodesx->data);
#else
 nodesx->fd=SlimUnmapFile(nodesx->fd);
#endif

 waysx->fd=CloseFileBuffered(waysx->fd);
 relationsx->rrfd=CloseFileBuffered(relationsx->rrfd);
 relationsx->trfd=CloseFileBuffered(relationsx->trfd);

 CloseFileBuffered(oldfd);
 CloseFileBuffered(newfd);

 /* Print the final message */

 printf_last("Calculated Coordinates: Errors=%"Pindex_t,errorlogsx->number);
}


/*++++++++++++++++++++++++++++++++++++++
  Re-index the nodes that were kept.

  NodesX *nodesx The set of nodes to process (contains the filename and number of nodes).
  ++++++++++++++++++++++++++++++++++++++*/

static void reindex_nodes(NodesX *nodesx)
{
 int fd;
 index_t index=0;
 NodeX nodex;

 nodesx->number=nodesx->knumber;

 nodesx->idata=(node_t*)malloc(nodesx->number*sizeof(node_t));

 /* Get the node id for each node in the file. */

 fd=ReOpenFileBuffered(nodesx->filename);

 while(!ReadFileBuffered(fd,&nodex,sizeof(NodeX)))
   {
    nodesx->idata[index]=nodex.id;

    index++;
   }

 CloseFileBuffered(fd);
}


/*++++++++++++++++++++++++++++++++++++++
  Re-index the ways that were kept.

  WaysX *waysx The set of ways to process (contains the filename and number of ways).
  ++++++++++++++++++++++++++++++++++++++*/

static void reindex_ways(WaysX *waysx)
{
 FILESORT_VARINT waysize;
 int fd;
 off_t position=0;
 index_t index=0;

 waysx->number=waysx->knumber;

 waysx->idata=(way_t*)malloc(waysx->number*sizeof(way_t));
 waysx->odata=(off_t*)malloc(waysx->number*sizeof(off_t));

 /* Get the way id and the offset for each way in the file */

 fd=ReOpenFileBuffered(waysx->filename);

 while(!ReadFileBuffered(fd,&waysize,FILESORT_VARSIZE))
   {
    WayX wayx;

    ReadFileBuffered(fd,&wayx,sizeof(WayX));

    waysx->idata[index]=wayx.id;
    waysx->odata[index]=position+FILESORT_VARSIZE+sizeof(WayX);

    index++;

    SkipFileBuffered(fd,waysize-sizeof(WayX));

    position+=waysize+FILESORT_VARSIZE;
   }

 CloseFileBuffered(fd);
}


/*++++++++++++++++++++++++++++++++++++++
  Re-index the relations that were kept.

  RelationsX *relationsx The set of relations to process (contains the filenames and numbers of relations).
  ++++++++++++++++++++++++++++++++++++++*/

static void reindex_relations(RelationsX *relationsx)
{
 FILESORT_VARINT relationsize;
 int fd;
 off_t position=0;
 index_t index;
 TurnRelX turnrelx;

 /* Route relations */

 relationsx->rrnumber=relationsx->rrknumber;

 relationsx->rridata=(relation_t*)malloc(relationsx->rrnumber*sizeof(relation_t));
 relationsx->rrodata=(off_t*)malloc(relationsx->rrnumber*sizeof(off_t));

 /* Get the relation id and the offset for each relation in the file */

 fd=ReOpenFileBuffered(relationsx->rrfilename);

 index=0;

 while(!ReadFileBuffered(fd,&relationsize,FILESORT_VARSIZE))
   {
    RouteRelX routerelx;

    ReadFileBuffered(fd,&routerelx,sizeof(RouteRelX));

    relationsx->rridata[index]=routerelx.id;
    relationsx->rrodata[index]=position+FILESORT_VARSIZE+sizeof(RouteRelX);

    index++;

    SkipFileBuffered(fd,relationsize-sizeof(RouteRelX));

    position+=relationsize+FILESORT_VARSIZE;
   }

 CloseFileBuffered(fd);


 /* Turn relations */

 relationsx->trnumber=relationsx->trknumber;

 relationsx->tridata=(relation_t*)malloc(relationsx->trnumber*sizeof(relation_t));

 /* Get the relation id and the offset for each relation in the file */

 fd=ReOpenFileBuffered(relationsx->trfilename);

 index=0;

 while(!ReadFileBuffered(fd,&turnrelx,sizeof(TurnRelX)))
   {
    relationsx->tridata[index]=turnrelx.id;

    index++;
   }

 CloseFileBuffered(fd);
}


/*++++++++++++++++++++++++++++++++++++++
  Lookup a node's latitude and longitude.

  int lookup_lat_long_node Returns 1 if a node was found.

  NodesX *nodesx The set of nodes to use.

  node_t node The node number.

  latlong_t *latitude Returns the latitude.

  latlong_t *longitude Returns the longitude.
  ++++++++++++++++++++++++++++++++++++++*/

static int lookup_lat_long_node(NodesX *nodesx,node_t node,latlong_t *latitude,latlong_t *longitude)
{
 index_t index=IndexNodeX(nodesx,node);

 if(index==NO_NODE)
    return 0;
 else
   {
    NodeX *nodex=LookupNodeX(nodesx,index,1);

    *latitude =nodex->latitude;
    *longitude=nodex->longitude;

    return 1;
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Lookup a way's latitude and longitude.

  int lookup_lat_long_way Returns 1 if a way was found.

  WaysX *waysx The set of ways to use.

  NodesX *nodesx The set of nodes to use.

  way_t way The way number.

  latlong_t *latitude Returns the latitude.

  latlong_t *longitude Returns the longitude.

  index_t error The index of the error in the complete set of errors.
  ++++++++++++++++++++++++++++++++++++++*/

static int lookup_lat_long_way(WaysX *waysx,NodesX *nodesx,way_t way,latlong_t *latitude,latlong_t *longitude,index_t error)
{
 index_t index=IndexWayX(waysx,way);

 if(index==NO_WAY)
    return 0;
 else
   {
    int count=1;
    off_t offset=waysx->odata[index];
    node_t node1,node2,prevnode,node;
    latlong_t latitude1,longitude1,latitude2,longitude2;

    SeekFileBuffered(waysx->fd,offset);

    /* Choose a random pair of adjacent nodes */

    if(ReadFileBuffered(waysx->fd,&node1,sizeof(node_t)) || node1==NO_NODE_ID)
       return 0;

    if(ReadFileBuffered(waysx->fd,&node2,sizeof(node_t)) || node2==NO_NODE_ID)
       return lookup_lat_long_node(nodesx,node1,latitude,longitude);

    prevnode=node2;

    while(!ReadFileBuffered(waysx->fd,&node,sizeof(node_t)) && node!=NO_NODE_ID)
      {
       count++;

       if((error%count)==0)     /* A 1/count chance */
         {
          node1=prevnode;
          node2=node;
         }

       prevnode=node;
      }

    if(!lookup_lat_long_node(nodesx,node1,&latitude1,&longitude1))
       return lookup_lat_long_node(nodesx,node2,latitude,longitude);

    if(!lookup_lat_long_node(nodesx,node2,&latitude2,&longitude2))
       return lookup_lat_long_node(nodesx,node1,latitude,longitude);

    *latitude =(latitude1 +latitude2 )/2;
    *longitude=(longitude1+longitude2)/2;

    return 1;
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Lookup a relation's latitude and longitude.

  int lookup_lat_long_relation Returns 1 if a relation was found.

  RelationsX *relationsx The set of relations to use.

  WaysX *waysx The set of ways to use.

  NodesX *nodesx The set of nodes to use.

  relation_t relation The relation number.

  latlong_t *latitude Returns the latitude.

  latlong_t *longitude Returns the longitude.

  index_t error The index of the error in the complete set of errors.
  ++++++++++++++++++++++++++++++++++++++*/

static int lookup_lat_long_relation(RelationsX *relationsx,WaysX *waysx,NodesX *nodesx,relation_t relation,latlong_t *latitude,latlong_t *longitude,index_t error)
{
 index_t index=IndexRouteRelX(relationsx,relation);

 if(index==NO_RELATION)
   {
    index=IndexTurnRelX(relationsx,relation);

    if(index==NO_RELATION)
       return 0;
    else
      {
       TurnRelX turnrelx;

       SeekFileBuffered(relationsx->trfd,index*sizeof(TurnRelX));
       ReadFileBuffered(relationsx->trfd,&turnrelx,sizeof(TurnRelX));

       if(lookup_lat_long_node(nodesx,turnrelx.via,latitude,longitude))
          return 1;

       if(lookup_lat_long_way(waysx,nodesx,turnrelx.from,latitude,longitude,error))
          return 1;

       if(lookup_lat_long_way(waysx,nodesx,turnrelx.to,latitude,longitude,error))
          return 1;

       return 0;
      }
   }
 else
   {
    int count;
    off_t offset=relationsx->rrodata[index];
    node_t node=NO_NODE_ID,tempnode;
    way_t way=NO_WAY_ID,tempway;
    relation_t relation=NO_RELATION_ID,temprelation;

    SeekFileBuffered(relationsx->rrfd,offset);

    /* Choose a random node */

    count=0;

    while(!ReadFileBuffered(relationsx->rrfd,&tempnode,sizeof(node_t)) && tempnode!=NO_NODE_ID)
      {
       count++;

       if((error%count)==0)     /* A 1/count chance */
          node=tempnode;
      }

    if(lookup_lat_long_node(nodesx,node,latitude,longitude))
       return 1;

    /* Choose a random way */

    count=0;

    while(!ReadFileBuffered(relationsx->rrfd,&tempway,sizeof(way_t)) && tempway!=NO_WAY_ID)
      {
       count++;

       if((error%count)==0)     /* A 1/count chance */
          way=tempway;
      }

    if(lookup_lat_long_way(waysx,nodesx,way,latitude,longitude,error))
       return 1;

    /* Choose a random relation */

    count=0;

    while(!ReadFileBuffered(relationsx->rrfd,&temprelation,sizeof(relation_t)) && temprelation!=NO_RELATION_ID)
      {
       count++;

       if((error%count)==0)     /* A 1/count chance */
          relation=temprelation;
      }

    return lookup_lat_long_relation(relationsx,waysx,nodesx,relation,latitude,longitude,error);
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Sort the error logs geographically.

  ErrorLogsX *errorlogsx The set of error logs to sort.
  ++++++++++++++++++++++++++++++++++++++*/

void SortErrorLogsGeographically(ErrorLogsX *errorlogsx)
{
 int oldfd,newfd;
 ll_bin_t lat_min_bin,lat_max_bin,lon_min_bin,lon_max_bin;

 /* Print the start message */

 printf_first("Sorting Errors Geographically");

 /* Work out the range of data */

 lat_min=radians_to_latlong( 2);
 lat_max=radians_to_latlong(-2);
 lon_min=radians_to_latlong( 4);
 lon_max=radians_to_latlong(-4);

 /* Re-open the file read-only and a new file writeable */

 oldfd=ReOpenFileBuffered(errorbinfilename);

 DeleteFile(errorbinfilename);

 newfd=OpenFileBufferedNew(errorbinfilename);

 /* Sort errors geographically */

 filesort_fixed(oldfd,newfd,sizeof(ErrorLogX),NULL,
                                              (int (*)(const void*,const void*))sort_by_lat_long,
                                              (int (*)(void*,index_t))measure_lat_long);

 /* Close the files */

 CloseFileBuffered(oldfd);
 CloseFileBuffered(newfd);

 /* Work out the number of bins */

 lat_min_bin=latlong_to_bin(lat_min);
 lon_min_bin=latlong_to_bin(lon_min);
 lat_max_bin=latlong_to_bin(lat_max);
 lon_max_bin=latlong_to_bin(lon_max);

 errorlogsx->latzero=lat_min_bin;
 errorlogsx->lonzero=lon_min_bin;

 errorlogsx->latbins=(lat_max_bin-lat_min_bin)+1;
 errorlogsx->lonbins=(lon_max_bin-lon_min_bin)+1;

 /* Print the final message */

 printf_last("Sorted Errors Geographically: Errors=%"Pindex_t,errorlogsx->number);
}


/*++++++++++++++++++++++++++++++++++++++
  Sort the errors into latitude and longitude order (first by longitude bin
  number, then by latitude bin number and then by exact longitude and then by
  exact latitude).

  int sort_by_lat_long Returns the comparison of the latitude and longitude fields.

  ErrorLogX *a The first error location.

  ErrorLogX *b The second error location.
  ++++++++++++++++++++++++++++++++++++++*/

static int sort_by_lat_long(ErrorLogX *a,ErrorLogX *b)
{
 ll_bin_t a_lon=latlong_to_bin(a->longitude);
 ll_bin_t b_lon=latlong_to_bin(b->longitude);

 if(a_lon<b_lon)
    return(-1);
 else if(a_lon>b_lon)
    return(1);
 else
   {
    ll_bin_t a_lat=latlong_to_bin(a->latitude);
    ll_bin_t b_lat=latlong_to_bin(b->latitude);

    if(a_lat<b_lat)
       return(-1);
    else if(a_lat>b_lat)
       return(1);
    else
      {
       if(a->longitude<b->longitude)
          return(-1);
       else if(a->longitude>b->longitude)
          return(1);
       else
         {
          if(a->latitude<b->latitude)
             return(-1);
          else if(a->latitude>b->latitude)
             return(1);
         }

       return(FILESORT_PRESERVE_ORDER(a,b));
      }
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Measure the extent of the data.

  int measure_lat_long Return 1 if the value is to be kept, otherwise 0.

  ErrorLogX *errorlogx The error location.

  index_t index The number of sorted error locations that have already been written to the output file.
  ++++++++++++++++++++++++++++++++++++++*/

static int measure_lat_long(ErrorLogX *errorlogx,index_t index)
{
 if(errorlogx->latitude!=NO_LATLONG)
   {
    if(errorlogx->latitude<lat_min)
       lat_min=errorlogx->latitude;
    if(errorlogx->latitude>lat_max)
       lat_max=errorlogx->latitude;
    if(errorlogx->longitude<lon_min)
       lon_min=errorlogx->longitude;
    if(errorlogx->longitude>lon_max)
       lon_max=errorlogx->longitude;
   }

 return(1);
}


/*++++++++++++++++++++++++++++++++++++++
  Save the binary error log.

  ErrorLogsX *errorlogsx The set of error logs to write.

  char *filename The name of the final file to write.
  ++++++++++++++++++++++++++++++++++++++*/

void SaveErrorLogs(ErrorLogsX *errorlogsx,char *filename)
{
 ErrorLogsFile errorlogsfile;
 ErrorLogX errorlogx;
 int oldfd,newfd;
 ll_bin2_t latlonbin=0,maxlatlonbins;
 index_t *offsets;
 index_t number=0,number_geo=0,number_nongeo=0;
 off_t size;

 /* Print the start message */

 printf_first("Writing Errors: Geographical=0 Non-geographical=0");

 /* Allocate the memory for the geographical offsets array */

 offsets=(index_t*)malloc((errorlogsx->latbins*errorlogsx->lonbins+1)*sizeof(index_t));

 logassert(offsets,"Failed to allocate memory (try using slim mode?)"); /* Check malloc() worked */

 latlonbin=0;

 /* Re-open the file */

 oldfd=ReOpenFileBuffered(errorbinfilename);

 newfd=OpenFileBufferedNew(filename);

 /* Write out the geographical errors */

 SeekFileBuffered(newfd,sizeof(ErrorLogsFile)+(errorlogsx->latbins*errorlogsx->lonbins+1)*sizeof(index_t));

 while(!ReadFileBuffered(oldfd,&errorlogx,sizeof(ErrorLogX)))
   {
    ErrorLog errorlog={0};
    ll_bin_t latbin,lonbin;
    ll_bin2_t llbin;

    if(errorlogx.latitude==NO_LATLONG)
       continue;

    /* Create the ErrorLog */

    errorlog.latoffset=latlong_to_off(errorlogx.latitude);
    errorlog.lonoffset=latlong_to_off(errorlogx.longitude);

    errorlog.offset=errorlogx.offset;
    errorlog.length=errorlogx.length;

    /* Work out the offsets */

    latbin=latlong_to_bin(errorlogx.latitude )-errorlogsx->latzero;
    lonbin=latlong_to_bin(errorlogx.longitude)-errorlogsx->lonzero;
    llbin=lonbin*errorlogsx->latbins+latbin;

    for(;latlonbin<=llbin;latlonbin++)
       offsets[latlonbin]=number_geo;

    /* Write the data */

    WriteFileBuffered(newfd,&errorlog,sizeof(ErrorLog));

    number_geo++;
    number++;

    if(!(number%10000))
       printf_middle("Writing Errors: Geographical=%"Pindex_t" Non-geographical=%"Pindex_t,number_geo,number_nongeo);
   }

 /* Write out the non-geographical errors */

 SeekFileBuffered(oldfd,0);

 while(!ReadFileBuffered(oldfd,&errorlogx,sizeof(ErrorLogX)))
   {
    ErrorLog errorlog={0};

    if(errorlogx.latitude!=NO_LATLONG)
       continue;

    /* Create the ErrorLog */

    errorlog.latoffset=0;
    errorlog.lonoffset=0;

    errorlog.offset=errorlogx.offset;
    errorlog.length=errorlogx.length;

    /* Write the data */

    WriteFileBuffered(newfd,&errorlog,sizeof(ErrorLog));

    number_nongeo++;
    number++;

    if(!(number%10000))
       printf_middle("Writing Errors: Geographical=%"Pindex_t" Non-geographical=%"Pindex_t,number_geo,number_nongeo);
   }

 /* Close the input file */

 CloseFileBuffered(oldfd);

 DeleteFile(errorbinfilename);

 /* Append the text from the log file */

 size=SizeFile(errorlogfilename);

 oldfd=ReOpenFileBuffered(errorlogfilename);

 while(size)
   {
    int i;
    char buffer[4096];
    off_t chunksize=(size>sizeof(buffer)?sizeof(buffer):size);

    ReadFileBuffered(oldfd,buffer,chunksize);

    for(i=0;i<chunksize;i++)
       if(buffer[i]=='\n')
          buffer[i]=0;

    WriteFileBuffered(newfd,buffer,chunksize);

    size-=chunksize;
   }

 CloseFileBuffered(oldfd);

 /* Finish off the offset indexing and write them out */

 maxlatlonbins=errorlogsx->latbins*errorlogsx->lonbins;

 for(;latlonbin<=maxlatlonbins;latlonbin++)
    offsets[latlonbin]=number_geo;

 SeekFileBuffered(newfd,sizeof(ErrorLogsFile));
 WriteFileBuffered(newfd,offsets,(errorlogsx->latbins*errorlogsx->lonbins+1)*sizeof(index_t));

 free(offsets);

 /* Write out the header structure */

 errorlogsfile.number       =number;
 errorlogsfile.number_geo   =number_geo;
 errorlogsfile.number_nongeo=number_nongeo;

 errorlogsfile.latbins=errorlogsx->latbins;
 errorlogsfile.lonbins=errorlogsx->lonbins;

 errorlogsfile.latzero=errorlogsx->latzero;
 errorlogsfile.lonzero=errorlogsx->lonzero;

 SeekFileBuffered(newfd,0);
 WriteFileBuffered(newfd,&errorlogsfile,sizeof(ErrorLogsFile));

 CloseFileBuffered(newfd);

 /* Print the final message */

 printf_last("Wrote Errors: Geographical=%"Pindex_t" Non-geographical=%"Pindex_t,number_geo,number_nongeo);
}

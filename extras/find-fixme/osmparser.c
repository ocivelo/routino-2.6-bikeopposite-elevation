/***************************************
 OSM file parser (either JOSM or planet)

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
#include <stdint.h>

#include "types.h"
#include "typesx.h"

#include "nodesx.h"
#include "waysx.h"
#include "relationsx.h"

#include "osmparser.h"
#include "tagging.h"
#include "logging.h"


/* Macros */

/*+ Checks if a value in the XML is one of the allowed values for true. +*/
#define ISTRUE(xx)  (!strcmp(xx,"true") || !strcmp(xx,"yes") || !strcmp(xx,"1"))

/*+ Checks if a value in the XML is one of the allowed values for false. +*/
#define ISFALSE(xx) (!strcmp(xx,"false") || !strcmp(xx,"no") || !strcmp(xx,"0"))

/* Local variables */

static NodesX     *nodes;
static WaysX      *ways;
static RelationsX *relations;

static node_t *way_nodes=NULL;
static int     way_nnodes=0;

static node_t     *relation_nodes=NULL;
static int         relation_nnodes=0;
static way_t      *relation_ways=NULL;
static int         relation_nways=0;
static relation_t *relation_relations=NULL;
static int         relation_nrelations=0;


/*++++++++++++++++++++++++++++++++++++++
  Initialise the OSM parser by initialising the local variables.

  NodesX *OSMNodes The data structure of nodes to fill in.

  WaysX *OSMWays The data structure of ways to fill in.

  RelationsX *OSMRelations The data structure of relations to fill in.
  ++++++++++++++++++++++++++++++++++++++*/

void InitialiseParser(NodesX *OSMNodes,WaysX *OSMWays,RelationsX *OSMRelations)
{
 /* Copy the function parameters and initialise the variables */

 nodes=OSMNodes;
 ways=OSMWays;
 relations=OSMRelations;

 way_nodes=(node_t*)malloc(256*sizeof(node_t));

 relation_nodes    =(node_t    *)malloc(256*sizeof(node_t));
 relation_ways     =(way_t     *)malloc(256*sizeof(way_t));
 relation_relations=(relation_t*)malloc(256*sizeof(relation_t));
}


/*++++++++++++++++++++++++++++++++++++++
  Clean up the memory after parsing.
  ++++++++++++++++++++++++++++++++++++++*/

void CleanupParser(void)
{
 /* Free the variables */

 free(way_nodes);

 free(relation_nodes);
 free(relation_ways);
 free(relation_relations);
}


/*++++++++++++++++++++++++++++++++++++++
  Add node references to a way.

  int64_t node_id The node ID to add or zero to clear the list.
  ++++++++++++++++++++++++++++++++++++++*/

void AddWayRefs(int64_t node_id)
{
 if(node_id==0)
    way_nnodes=0;
 else
   {
    node_t id;

    if(way_nnodes && (way_nnodes%256)==0)
       way_nodes=(node_t*)realloc((void*)way_nodes,(way_nnodes+256)*sizeof(node_t));

    id=(node_t)node_id;
    logassert((int64_t)id==node_id,"Node ID too large (change node_t to 64-bits?)"); /* check node id can be stored in node_t data type. */

    way_nodes[way_nnodes++]=id;
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Add node, way or relation references to a relation.

  int64_t node_id The node ID to add or zero if it is not a node.

  int64_t way_id The way ID to add or zero if it is not a way.

  int64_t relation_id The relation ID to add or zero if it is not a relation.

  const char *role The role played by this referenced item or NULL.

  If all of node_id, way_id and relation_id are zero then the list is cleared.
  ++++++++++++++++++++++++++++++++++++++*/

void AddRelationRefs(int64_t node_id,int64_t way_id,int64_t relation_id,const char *role)
{
 if(node_id==0 && way_id==0 && relation_id==0)
   {
    relation_nnodes=0;
    relation_nways=0;
    relation_nrelations=0;
   }
 else if(node_id!=0)
   {
    node_t id;

    id=(node_t)node_id;
    logassert((int64_t)id==node_id,"Node ID too large (change node_t to 64-bits?)"); /* check node id can be stored in node_t data type. */

    if(relation_nnodes && (relation_nnodes%256)==0)
       relation_nodes=(node_t*)realloc((void*)relation_nodes,(relation_nnodes+256)*sizeof(node_t));

    relation_nodes[relation_nnodes++]=id;
   }
 else if(way_id!=0)
   {
    way_t id;

    id=(way_t)way_id;
    logassert((int64_t)id==way_id,"Way ID too large (change way_t to 64-bits?)"); /* check way id can be stored in way_t data type. */

    if(relation_nways && (relation_nways%256)==0)
       relation_ways=(way_t*)realloc((void*)relation_ways,(relation_nways+256)*sizeof(way_t));

    relation_ways[relation_nways++]=id;
   }
 else /* if(relation_id!=0) */
   {
    relation_t id;

    id=(relation_t)relation_id;
    logassert((int64_t)id==relation_id,"Relation ID too large (change relation_t to 64-bits?)"); /* check relation id can be stored in relation_t data type. */

    if(relation_nrelations && (relation_nrelations%256)==0)
       relation_relations=(relation_t*)realloc((void*)relation_relations,(relation_nrelations+256)*sizeof(relation_t));

    relation_relations[relation_nrelations++]=relation_id;
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Process the tags associated with a node.

  TagList *tags The list of node tags.

  int64_t node_id The id of the node.

  double latitude The latitude of the node.

  double longitude The longitude of the node.

  int mode The mode of operation to take (create, modify, delete).
  ++++++++++++++++++++++++++++++++++++++*/

void ProcessNodeTags(TagList *tags,int64_t node_id,double latitude,double longitude,int mode)
{
 node_t id;
 int i;

 /* Convert id */

 id=(node_t)node_id;
 logassert((int64_t)id==node_id,"Node ID too large (change node_t to 64-bits?)"); /* check node id can be stored in node_t data type. */

 /* Parse the tags */

 for(i=0;i<tags->ntags;i++)
   {
    char *k=tags->k[i];

    if(!strcmp(k,"fixme-finder:keep"))
      {
       DeleteTag(tags,"fixme-finder:keep");
       logerror("<node id='%"Pnode_t"'>%s</node>\n",logerror_node(id),StringifyTag(tags));
      }
   }

 /* Store the node */

 AppendNodeList(nodes,id,degrees_to_radians(latitude),degrees_to_radians(longitude),0,0);
}


/*++++++++++++++++++++++++++++++++++++++
  Process the tags associated with a way.

  TagList *tags The list of way tags.

  int64_t way_id The id of the way.

  int mode The mode of operation to take (create, modify, delete).
  ++++++++++++++++++++++++++++++++++++++*/

void ProcessWayTags(TagList *tags,int64_t way_id,int mode)
{
 Way way={0};
 way_t id;
 int i;

 /* Convert id */

 id=(way_t)way_id;
 logassert((int64_t)id==way_id,"Way ID too large (change way_t to 64-bits?)"); /* check way id can be stored in way_t data type. */

 /* Parse the tags */

 for(i=0;i<tags->ntags;i++)
   {
    char *k=tags->k[i];

    if(!strcmp(k,"fixme-finder:keep"))
      {
       DeleteTag(tags,"fixme-finder:keep");
       logerror("<way id='%"Pway_t"'>%s</way>\n",logerror_way(id),StringifyTag(tags));
      }
   }

 /* Store the way */

 AppendWayList(ways,id,&way,way_nodes,way_nnodes,"");
}


/*++++++++++++++++++++++++++++++++++++++
  Process the tags associated with a relation.

  TagList *tags The list of relation tags.

  int64_t relation_id The id of the relation.

  int mode The mode of operation to take (create, modify, delete).
  ++++++++++++++++++++++++++++++++++++++*/

void ProcessRelationTags(TagList *tags,int64_t relation_id,int mode)
{
 relation_t id;
 int i;

 /* Convert id */

 id=(relation_t)relation_id;
 logassert((int64_t)id==relation_id,"Relation ID too large (change relation_t to 64-bits?)"); /* check relation id can be stored in relation_t data type. */

 /* Parse the tags */

 for(i=0;i<tags->ntags;i++)
   {
    char *k=tags->k[i];

    if(!strcmp(k,"fixme-finder:keep"))
      {
       DeleteTag(tags,"fixme-finder:keep");
       logerror("<relation id='%"Prelation_t"'>%s</relation>\n",logerror_relation(id),StringifyTag(tags));
      }
   }

 /* Store the relation */

 AppendRouteRelationList(relations,id,0,
                         relation_nodes,relation_nnodes,
                         relation_ways,relation_nways,
                         relation_relations,relation_nrelations);
}

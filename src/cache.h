/***************************************
 Functions to maintain an in-RAM cache of on-disk data for slim mode.

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


#if SLIM

#ifndef CACHE_H
#define CACHE_H    /*+ To stop multiple inclusions. +*/

#include <unistd.h>
#include <stdlib.h>

#include "types.h"


/* Macros for constants */

#define CACHEWIDTH 2048         /*+ The width of the cache. +*/
#define CACHEDEPTH   16         /*+ The depth of the cache. +*/


/* Macro for structure forward declaration */

#define CACHE_STRUCTURE_FWD(type) typedef struct _##type##Cache type##Cache;


/* Macro for structure declaration */

/*+ A macro to create a cache structure. +*/
#define CACHE_STRUCTURE(type) \
                              \
struct _##type##Cache                                                     \
{                                                                         \
 int     first  [CACHEWIDTH];             /*+ The first entry to fill +*/ \
                                                                          \
 type    data   [CACHEWIDTH][CACHEDEPTH]; /*+ The array of type##s. +*/   \
 index_t indices[CACHEWIDTH][CACHEDEPTH]; /*+ The array of indexes. +*/   \
};


/* Macros for function prototypes */

#define CACHE_NEWCACHE_PROTO(type) static inline type##Cache *New##type##Cache(void);

#define CACHE_DELETECACHE_PROTO(type) static inline void Delete##type##Cache(type##Cache *cache);

#define CACHE_FETCHCACHE_PROTO(type) static inline type *FetchCached##type(type##Cache *cache,index_t index,int fd,off_t offset);

#define CACHE_REPLACECACHE_PROTO(type) static inline void ReplaceCached##type(type##Cache *cache,type *value,index_t index,int fd,off_t offset);

#define CACHE_INVALIDATECACHE_PROTO(type) static inline void Invalidate##type##Cache(type##Cache *cache);


/* Macros for function definitions */

/*+ A macro to create a function that creates a new cache data structure. +*/
#define CACHE_NEWCACHE(type) \
                             \
static inline type##Cache *New##type##Cache(void)       \
{                                                       \
 type##Cache *cache;                                    \
                                                        \
 cache=(type##Cache*)malloc(sizeof(type##Cache));       \
                                                        \
 Invalidate##type##Cache(cache);                        \
                                                        \
 return(cache);                                         \
}


/*+ A macro to create a function that deletes a cache data structure. +*/
#define CACHE_DELETECACHE(type) \
                                \
static inline void Delete##type##Cache(type##Cache *cache)      \
{                                                               \
 free(cache);                                                   \
}


/*+ A macro to create a function that fetches an item from a cache data structure or reads from file. +*/
#define CACHE_FETCHCACHE(type) \
                               \
static inline type *FetchCached##type(type##Cache *cache,index_t index,int fd,off_t offset) \
{                                                                                           \
 int row=index%CACHEWIDTH;                                                                  \
 int col;                                                                                   \
                                                                                            \
 for(col=0;col<CACHEDEPTH;col++)                                                            \
    if(cache->indices[row][col]==index)                                                     \
       return(&cache->data[row][col]);                                                      \
                                                                                            \
 col=cache->first[row];                                                                     \
                                                                                            \
 cache->first[row]=(cache->first[row]+1)%CACHEDEPTH;                                        \
                                                                                            \
 SlimFetch(fd,&cache->data[row][col],sizeof(type),offset+(off_t)index*sizeof(type));        \
                                                                                            \
 cache->indices[row][col]=index;                                                            \
                                                                                            \
 return(&cache->data[row][col]);                                                            \
}


/*+ A macro to create a function that replaces an item in a cache data structure and writes to file. +*/
#define CACHE_REPLACECACHE(type) \
                                 \
static inline void ReplaceCached##type(type##Cache *cache,type *value,index_t index,int fd,off_t offset) \
{                                                                                                        \
 int row=index%CACHEWIDTH;                                                                               \
 int col;                                                                                                \
                                                                                                         \
 for(col=0;col<CACHEDEPTH;col++)                                                                         \
    if(cache->indices[row][col]==index)                                                                  \
       break;                                                                                            \
                                                                                                         \
 if(col==CACHEDEPTH)                                                                                     \
   {                                                                                                     \
    col=cache->first[row];                                                                               \
                                                                                                         \
    cache->first[row]=(cache->first[row]+1)%CACHEDEPTH;                                                  \
   }                                                                                                     \
                                                                                                         \
 cache->indices[row][col]=index;                                                                         \
                                                                                                         \
 cache->data[row][col]=*value;                                                                           \
                                                                                                         \
 SlimReplace(fd,&cache->data[row][col],sizeof(type),offset+(off_t)index*sizeof(type));                   \
}


/*+ A macro to create a function that invalidates the contents of a cache data structure. +*/
#define CACHE_INVALIDATECACHE(type) \
                                    \
static inline void Invalidate##type##Cache(type##Cache *cache) \
{                                                              \
 int row,col;                                                  \
                                                               \
 for(row=0;row<CACHEWIDTH;row++)                               \
   {                                                           \
    cache->first[row]=0;                                       \
                                                               \
    for(col=0;col<CACHEDEPTH;col++)                            \
       cache->indices[row][col]=NO_NODE;                       \
   }                                                           \
}


/*+ Cache data structure forward declarations (for planetsplitter). +*/
CACHE_STRUCTURE_FWD(NodeX)
CACHE_STRUCTURE_FWD(SegmentX)
CACHE_STRUCTURE_FWD(WayX)

/*+ Cache data structure forward declarations (for router). +*/
CACHE_STRUCTURE_FWD(Node)
CACHE_STRUCTURE_FWD(Segment)
CACHE_STRUCTURE_FWD(Way)
CACHE_STRUCTURE_FWD(TurnRelation)


#endif /* CACHE_H */

#endif  /* SLIM */

/***************************************
 A header file for the results.

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


#ifndef RESULTS_H
#define RESULTS_H    /*+ To stop multiple inclusions. +*/

#include <stdint.h>

#include "types.h"


/* Constants */

/*+ A result is not currently queued. +*/
#define NOT_QUEUED (uint32_t)(0)


/* Data structures */

typedef struct _Result Result;

/*+ The result for a node. +*/
struct _Result
{
 index_t   node;                /*+ The node for which this result applies. +*/
 index_t   segment;             /*+ The segmemt used to get to the node for which this result applies. +*/

 Result   *prev;                /*+ The previous result following the best path to get to this node via the segment. +*/
 Result   *next;                /*+ The next result following the best path from this node that was reached via the segment. +*/

 score_t   score;               /*+ The best actual weighted distance or duration score from the start to the node. +*/
 score_t   sortby;              /*+ The best possible weighted distance or duration score from the start to the finish. +*/

 uint32_t  queued;              /*+ The position of this result in the queue. +*/

 Result   *hashnext;            /*+ The next result in the linked list for this hash bin. +*/
};

/*+ A list of results. +*/
typedef struct _Results
{
 uint32_t  nbins;               /*+ The number of bins in the has table. +*/
 uint32_t  mask;                /*+ A bit mask to select the bottom log2(nbins) bits. +*/

 uint32_t  number;              /*+ The total number of occupied results. +*/

 uint8_t   ncollisions;         /*+ The number of results allowed in each hash bin. +*/
 uint8_t  *count;               /*+ An array of nbins counters of results in each hash bin. +*/

 Result  **point;               /*+ An array of nbins linked lists of results for one hash bin. +*/

 uint32_t  ndata1;              /*+ The size of the first dimension of the 'data' array. +*/
 uint32_t  ndata2;              /*+ The size of the second dimension of the 'data' array. +*/

 uint32_t  nallocdata1;         /*+ The amount of allocated space in the first dimension of the 'data' array. +*/

 Result  **data;                /*+ An array of arrays containing the actual results, the first
                                    dimension is reallocated but the second dimension is not.
                                    Most importantly pointers into the real data don't change
                                    as more space is allocated (since realloc is not being used). +*/

 index_t start_node;            /*+ The start node. +*/
 index_t prev_segment;          /*+ The previous segment to get to the start node (if any). +*/

 index_t finish_node;           /*+ The finish node. +*/
 index_t last_segment;          /*+ The last segment (to arrive at the finish node). +*/
}
 Results;


/* Forward definition for opaque type */

typedef struct _Queue Queue;


/* Results functions in results.c */

Results *NewResultsList(uint8_t log2bins);
void ResetResultsList(Results *results);
void FreeResultsList(Results *results);

Result *InsertResult(Results *results,index_t node,index_t segment);

Result *FindResult(Results *results,index_t node,index_t segment);

Result *FirstResult(Results *results);
Result *NextResult(Results *results,Result *result);


/* Queue functions in queue.c */

Queue *NewQueueList(uint8_t log2bins);
void ResetQueueList(Queue *queue);
void FreeQueueList(Queue *queue);

void InsertInQueue(Queue *queue,Result *result,score_t score);
Result *PopFromQueue(Queue *queue);


#endif /* RESULTS_H */

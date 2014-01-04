/***************************************
 Queue data type functions.

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


#include <string.h>
#include <stdlib.h>

#include "results.h"


/*+ A queue of results. +*/
struct _Queue
{
 int      nincrement;           /*+ The amount to increment the queue when full. +*/
 int      nallocated;           /*+ The number of entries allocated. +*/
 int      noccupied;            /*+ The number of entries occupied. +*/

 Result **results;              /*+ The queue of pointers to results. +*/
};


/*++++++++++++++++++++++++++++++++++++++
  Allocate a new queue.

  Queue *NewQueueList Returns the queue.

  uint8_t log2bins The base 2 logarithm of the initial number of bins in the queue.
  ++++++++++++++++++++++++++++++++++++++*/

Queue *NewQueueList(uint8_t log2bins)
{
 Queue *queue;

 queue=(Queue*)malloc(sizeof(Queue));

 queue->nincrement=1<<log2bins;

 queue->nallocated=queue->nincrement;
 queue->noccupied=0;

 queue->results=(Result**)malloc(queue->nallocated*sizeof(Result*));

 return(queue);
}


/*++++++++++++++++++++++++++++++++++++++
  Re-use an existing queue.

  Queue *queue The queue to reset for re-use.
  ++++++++++++++++++++++++++++++++++++++*/

void ResetQueueList(Queue *queue)
{
 queue->noccupied=0;
}


/*++++++++++++++++++++++++++++++++++++++
  Free a queue.

  Queue *queue The queue to be freed.
  ++++++++++++++++++++++++++++++++++++++*/

void FreeQueueList(Queue *queue)
{
 free(queue->results);

 free(queue);
}


/*++++++++++++++++++++++++++++++++++++++
  Insert a new item into the queue in the right place.

  The data is stored in a "Binary Heap" http://en.wikipedia.org/wiki/Binary_heap
  and this operation is adding an item to the heap.

  Queue *queue The queue to insert the result into.

  Result *result The result to insert into the queue.

  score_t score The score to use for sorting the node.
  ++++++++++++++++++++++++++++++++++++++*/

void InsertInQueue(Queue *queue,Result *result,score_t score)
{
 int index;

 if(result->queued==NOT_QUEUED)
   {
    queue->noccupied++;
    index=queue->noccupied;

    if(queue->noccupied==queue->nallocated)
      {
       queue->nallocated=queue->nallocated+queue->nincrement;
       queue->results=(Result**)realloc((void*)queue->results,queue->nallocated*sizeof(Result*));
      }

    queue->results[index]=result;
    queue->results[index]->queued=index;
   }
 else
    index=result->queued;

 queue->results[index]->sortby=score;

 /* Bubble up the new value */

 while(index>1)
   {
    int newindex;
    Result *temp;

    newindex=index/2;

    if(queue->results[index]->sortby>=queue->results[newindex]->sortby)
       break;

    temp=queue->results[index];
    queue->results[index]=queue->results[newindex];
    queue->results[newindex]=temp;

    queue->results[index]->queued=index;
    queue->results[newindex]->queued=newindex;

    index=newindex;
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Pop an item from the front of the queue.

  The data is stored in a "Binary Heap" http://en.wikipedia.org/wiki/Binary_heap
  and this operation is deleting the root item from the heap.

  Result *PopFromQueue Returns the top item.

  Queue *queue The queue to remove the result from.
  ++++++++++++++++++++++++++++++++++++++*/

Result *PopFromQueue(Queue *queue)
{
 int index;
 Result *retval;

 if(queue->noccupied==0)
    return(NULL);

 retval=queue->results[1];
 retval->queued=NOT_QUEUED;

 index=1;

 queue->results[index]=queue->results[queue->noccupied];

 queue->noccupied--;

 /* Bubble down the newly promoted value */

 while((2*index)<queue->noccupied)
   {
    int newindex;
    Result *temp;

    newindex=2*index;

    if(queue->results[newindex]->sortby>queue->results[newindex+1]->sortby)
       newindex=newindex+1;

    if(queue->results[index]->sortby<=queue->results[newindex]->sortby)
       break;

    temp=queue->results[newindex];
    queue->results[newindex]=queue->results[index];
    queue->results[index]=temp;

    queue->results[index]->queued=index;
    queue->results[newindex]->queued=newindex;

    index=newindex;
   }

 if((2*index)==queue->noccupied)
   {
    int newindex;
    Result *temp;

    newindex=2*index;

    if(queue->results[index]->sortby<=queue->results[newindex]->sortby)
       ; /* break */
    else
      {
       temp=queue->results[newindex];
       queue->results[newindex]=queue->results[index];
       queue->results[index]=temp;

       queue->results[index]->queued=index;
       queue->results[newindex]->queued=newindex;
      }
   }

 return(retval);
}

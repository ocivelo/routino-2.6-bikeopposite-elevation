/***************************************
 Header file for logging function prototypes

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


#ifndef LOGGING_H
#define LOGGING_H    /*+ To stop multiple inclusions. +*/

#include <stdio.h>
#include <sys/time.h>

#include "typesx.h"


/* Data structures */

/*+ A structure containing a single object as written by the logerror_*() functions. +*/
typedef struct _ErrorLogObject
{
 char      type;             /*+ The type of the object. +*/

 uint64_t  id;               /*+ The id of the object. +*/

 uint32_t  offset;           /*+ The offset of the error message from the beginning of the text file. +*/
}
 ErrorLogObject;


/* Variables */

extern int option_loggable;
extern int option_logtime;


/* Runtime progress logging functions in logging.c */

void printf_program_start(void);
void printf_program_end(void);


#ifdef __GNUC__

void printf_first(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
void printf_middle(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
void printf_last(const char *format, ...) __attribute__ ((format (printf, 1, 2)));

void fprintf_first(FILE *file,const char *format, ...) __attribute__ ((format (printf, 2, 3)));
void fprintf_middle(FILE *file,const char *format, ...) __attribute__ ((format (printf, 2, 3)));
void fprintf_last(FILE *file,const char *format, ...) __attribute__ ((format (printf, 2, 3)));

#else

void printf_first(const char *format, ...);
void printf_middle(const char *format, ...);
void printf_last(const char *format, ...);

void fprintf_first(FILE *file,const char *format, ...);
void fprintf_middle(FILE *file,const char *format, ...);
void fprintf_last(FILE *file,const char *format, ...);

#endif

void fprintf_elapsed_time(FILE *file,struct timeval *start);


/* Error logging functions in logerror.c */

void open_errorlog(const char *filename,int append,int bin);
void close_errorlog(void);

#ifdef __GNUC__

void logerror(const char *format, ...) __attribute__ ((format (printf, 1, 2)));

#else

void logerror(const char *format, ...);

#endif

node_t     logerror_node    (node_t     id);
way_t      logerror_way     (way_t      id);
relation_t logerror_relation(relation_t id);


/* Runtime fatal error assertion in logging.c */

#define logassert(xx,yy) do { if(!(xx)) _logassert(yy,__FILE__,__LINE__); } while(0)

void _logassert(const char *message,const char *file,int line);


#endif /* LOGGING_H */

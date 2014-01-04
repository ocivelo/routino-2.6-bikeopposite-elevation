/***************************************
 Header file for file function prototypes

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


#ifndef FILES_H
#define FILES_H    /*+ To stop multiple inclusions. +*/

/* If your system does not have the pread() and pwrite() system calls then you
 * will need to change this line to the value 0 so that seek() and
 * read()/write() are used instead of pread()/pwrite(). */
#define HAVE_PREAD_PWRITE 1


#include <unistd.h>
#include <sys/types.h>

#include "logging.h"


/* Functions in files.c */

char *FileName(const char *dirname,const char *prefix, const char *name);

void *MapFile(const char *filename);
void *MapFileWriteable(const char *filename);

void *UnmapFile(const void *address);

int SlimMapFile(const char *filename);
int SlimMapFileWriteable(const char *filename);

int SlimUnmapFile(int fd);

int OpenFileBufferedNew(const char *filename);
int OpenFileBufferedAppend(const char *filename);

int ReOpenFileBuffered(const char *filename);

int WriteFileBuffered(int fd,const void *address,size_t length);
int ReadFileBuffered(int fd,void *address,size_t length);

int SeekFileBuffered(int fd,off_t position);
int SkipFileBuffered(int fd,off_t skip);

int CloseFileBuffered(int fd);

int OpenFile(const char *filename);

void CloseFile(int fd);

off_t SizeFile(const char *filename);
int ExistsFile(const char *filename);

int DeleteFile(const char *filename);

int RenameFile(const char *oldfilename,const char *newfilename);

/* Functions in files.h */

static inline int SlimReplace(int fd,const void *address,size_t length,off_t position);
static inline int SlimFetch(int fd,void *address,size_t length,off_t position);


/* Inline the frequently called functions */

/*++++++++++++++++++++++++++++++++++++++
  Write data to a file that has been opened for slim mode access.

  int SlimReplace Returns 0 if OK or something else in case of an error.

  int fd The file descriptor to write to.

  const void *address The address of the data to be written.

  size_t length The length of data to write.

  off_t position The position in the file to seek to.
  ++++++++++++++++++++++++++++++++++++++*/

static inline int SlimReplace(int fd,const void *address,size_t length,off_t position)
{
 /* Seek and write the data */

#if HAVE_PREAD_PWRITE

 if(pwrite(fd,address,length,position)!=(ssize_t)length)
    return(-1);

#else

 if(lseek(fd,position,SEEK_SET)!=position)
    return(-1);

 if(write(fd,address,length)!=length)
    return(-1);

#endif

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  Read data from a file that has been opened for slim mode access.

  int SlimFetch Returns 0 if OK or something else in case of an error.

  int fd The file descriptor to read from.

  void *address The address the data is to be read into.

  size_t length The length of data to read.

  off_t position The position in the file to seek to.
  ++++++++++++++++++++++++++++++++++++++*/

static inline int SlimFetch(int fd,void *address,size_t length,off_t position)
{
 /* Seek and read the data */

#if HAVE_PREAD_PWRITE

 if(pread(fd,address,length,position)!=(ssize_t)length)
    return(-1);

#else

 if(lseek(fd,position,SEEK_SET)!=position)
    return(-1);

 if(read(fd,address,length)!=length)
    return(-1);

#endif

 return(0);
}


#endif /* FILES_H */

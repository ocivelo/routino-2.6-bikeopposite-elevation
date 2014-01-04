/***************************************
 Fixme file dumper.

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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>

#include "types.h"
#include "errorlog.h"

#include "files.h"
#include "xmlparse.h"


/* Local functions */

static void OutputErrorLog(ErrorLogs *errorlogs,double latmin,double latmax,double lonmin,double lonmax);

static void print_errorlog_visualiser(ErrorLogs *errorlogs,index_t item);

static char *RFC822Date(time_t t);

static void print_usage(int detail,const char *argerr,const char *err);


/*++++++++++++++++++++++++++++++++++++++
  The main program for the fixme dumper.
  ++++++++++++++++++++++++++++++++++++++*/

int main(int argc,char** argv)
{
 ErrorLogs*OSMErrorLogs;
 int       arg;
 char     *dirname=NULL,*prefix=NULL;
 char     *errorlogs_filename;
 int       option_statistics=0;
 int       option_visualiser=0,coordcount=0;
 double    latmin=0,latmax=0,lonmin=0,lonmax=0;
 char     *option_data=NULL;
 int       option_dump_visualiser=0;

 /* Parse the command line arguments */

 for(arg=1;arg<argc;arg++)
   {
    if(!strcmp(argv[arg],"--help"))
       print_usage(1,NULL,NULL);
    else if(!strncmp(argv[arg],"--dir=",6))
       dirname=&argv[arg][6];
    else if(!strcmp(argv[arg],"--statistics"))
       option_statistics=1;
    else if(!strcmp(argv[arg],"--visualiser"))
       option_visualiser=1;
    else if(!strcmp(argv[arg],"--dump-visualiser"))
       option_dump_visualiser=1;
    else if(!strncmp(argv[arg],"--latmin",8) && argv[arg][8]=='=')
      {latmin=degrees_to_radians(atof(&argv[arg][9]));coordcount++;}
    else if(!strncmp(argv[arg],"--latmax",8) && argv[arg][8]=='=')
      {latmax=degrees_to_radians(atof(&argv[arg][9]));coordcount++;}
    else if(!strncmp(argv[arg],"--lonmin",8) && argv[arg][8]=='=')
      {lonmin=degrees_to_radians(atof(&argv[arg][9]));coordcount++;}
    else if(!strncmp(argv[arg],"--lonmax",8) && argv[arg][8]=='=')
      {lonmax=degrees_to_radians(atof(&argv[arg][9]));coordcount++;}
    else if(!strncmp(argv[arg],"--data",6) && argv[arg][6]=='=')
       option_data=&argv[arg][7];
    else if(!strncmp(argv[arg],"--fixme=",8))
       ;
    else
       print_usage(0,argv[arg],NULL);
   }

 if((option_statistics + option_visualiser + option_dump_visualiser)!=1)
    print_usage(0,NULL,"Must choose --visualiser, --statistics or --dump-visualiser.");

 /* Load in the data - Note: No error checking because Load*List() will call exit() in case of an error. */

 OSMErrorLogs=LoadErrorLogs(errorlogs_filename=FileName(dirname,prefix,"fixme.mem"));

 /* Write out the visualiser data */

 if(option_visualiser)
   {
    if(coordcount!=4)
       print_usage(0,NULL,"The --visualiser option must have --latmin, --latmax, --lonmin, --lonmax.\n");

    if(!option_data)
       print_usage(0,NULL,"The --visualiser option must have --data.\n");

    if(!strcmp(option_data,"fixmes"))
       OutputErrorLog(OSMErrorLogs,latmin,latmax,lonmin,lonmax);
    else
       print_usage(0,option_data,NULL);
   }

 /* Print out statistics */

 if(option_statistics)
   {
    struct stat buf;

    /* Examine the files */

    printf("Files\n");
    printf("-----\n");
    printf("\n");

    stat(errorlogs_filename,&buf);

    printf("'%s%sfixme.mem' - %9"PRIu64" Bytes\n",prefix?prefix:"",prefix?"-":"",(uint64_t)buf.st_size);
    printf("%s\n",RFC822Date(buf.st_mtime));
    printf("\n");

    printf("\n");
    printf("Error Logs\n");
    printf("----------\n");
    printf("\n");

    printf("Number(total)           =%9"Pindex_t"\n",OSMErrorLogs->file.number);
    printf("Number(geographical)    =%9"Pindex_t"\n",OSMErrorLogs->file.number_geo);
    printf("Number(non-geographical)=%9"Pindex_t"\n",OSMErrorLogs->file.number_nongeo);

    printf("\n");
    stat(errorlogs_filename,&buf);
#if !SLIM
    printf("Total strings=%9lu Bytes\n",(unsigned long)buf.st_size-(unsigned long)(OSMErrorLogs->strings-(char*)OSMErrorLogs->data));
#else
    printf("Total strings=%9lu Bytes\n",(unsigned long)buf.st_size-(unsigned long)OSMErrorLogs->stringsoffset);
#endif
   }

 /* Print out internal data (in HTML format for the visualiser) */

 if(option_dump_visualiser)
   {
    index_t item;

    if(!option_data)
       print_usage(0,NULL,"The --dump-visualiser option must have --data.\n");

    for(arg=1;arg<argc;arg++)
       if(!strncmp(argv[arg],"--data=fixme",12))
         {
          item=atoi(&argv[arg][12]);

          if(item<OSMErrorLogs->file.number)
             print_errorlog_visualiser(OSMErrorLogs,item);
          else
             printf("Invalid fixme number; minimum=0, maximum=%"Pindex_t".\n",OSMErrorLogs->file.number-1);
         }
   }

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  Output the data for error logs within the region.

  ErrorLogs *errorlogs The set of error logs to use.

  double latmin The minimum latitude.

  double latmax The maximum latitude.

  double lonmin The minimum longitude.

  double lonmax The maximum longitude.
  ++++++++++++++++++++++++++++++++++++++*/

static void OutputErrorLog(ErrorLogs *errorlogs,double latmin,double latmax,double lonmin,double lonmax)
{
 ll_bin_t latminbin=latlong_to_bin(radians_to_latlong(latmin))-errorlogs->file.latzero;
 ll_bin_t latmaxbin=latlong_to_bin(radians_to_latlong(latmax))-errorlogs->file.latzero;
 ll_bin_t lonminbin=latlong_to_bin(radians_to_latlong(lonmin))-errorlogs->file.lonzero;
 ll_bin_t lonmaxbin=latlong_to_bin(radians_to_latlong(lonmax))-errorlogs->file.lonzero;
 ll_bin_t latb,lonb;
 index_t i,index1,index2;

 /* Loop through all of the error logs. */

 for(latb=latminbin;latb<=latmaxbin;latb++)
    for(lonb=lonminbin;lonb<=lonmaxbin;lonb++)
      {
       ll_bin2_t llbin=lonb*errorlogs->file.latbins+latb;

       if(llbin<0 || llbin>(errorlogs->file.latbins*errorlogs->file.lonbins))
          continue;

       index1=LookupErrorLogOffset(errorlogs,llbin);
       index2=LookupErrorLogOffset(errorlogs,llbin+1);

       if(index2>errorlogs->file.number_geo)
          index2=errorlogs->file.number_geo;

       for(i=index1;i<index2;i++)
         {
          ErrorLog *errorlogp=LookupErrorLog(errorlogs,i,1);

          double lat=latlong_to_radians(bin_to_latlong(errorlogs->file.latzero+latb)+off_to_latlong(errorlogp->latoffset));
          double lon=latlong_to_radians(bin_to_latlong(errorlogs->file.lonzero+lonb)+off_to_latlong(errorlogp->lonoffset));

          if(lat>latmin && lat<latmax && lon>lonmin && lon<lonmax)
             printf("fixme%"Pindex_t" %.6f %.6f\n",i,radians_to_degrees(lat),radians_to_degrees(lon));
         }
      }
}


/*++++++++++++++++++++++++++++++++++++++
  Print out an error log entry from the database (in visualiser format).

  ErrorLogs *errorlogs The set of error logs to use.

  index_t item The error log index to print.
  ++++++++++++++++++++++++++++++++++++++*/

static void print_errorlog_visualiser(ErrorLogs *errorlogs,index_t item)
{
 char *string=LookupErrorLogString(errorlogs,item);

 printf("%s\n",ParseXML_Encode_Safe_XML(string));
}


/*+ Conversion from time_t to date string (day of week). +*/
static const char* const weekdays[7]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

/*+ Conversion from time_t to date string (month of year). +*/
static const char* const months[12]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};


/*++++++++++++++++++++++++++++++++++++++
  Convert the time into an RFC 822 compliant date.

  char *RFC822Date Returns a pointer to a fixed string containing the date.

  time_t t The time.
  ++++++++++++++++++++++++++++++++++++++*/

static char *RFC822Date(time_t t)
{
 static char value[32];
 char weekday[4];
 char month[4];
 struct tm *tim;

 tim=gmtime(&t);

 strcpy(weekday,weekdays[tim->tm_wday]);
 strcpy(month,months[tim->tm_mon]);

 /* Sun, 06 Nov 1994 08:49:37 GMT    ; RFC 822, updated by RFC 1123 */

 sprintf(value,"%3s, %02d %3s %4d %02d:%02d:%02d %s",
         weekday,
         tim->tm_mday,
         month,
         tim->tm_year+1900,
         tim->tm_hour,
         tim->tm_min,
         tim->tm_sec,
         "GMT"
         );

 return(value);
}


/*++++++++++++++++++++++++++++++++++++++
  Print out the usage information.

  int detail The level of detail to use - 0 = low, 1 = high.

  const char *argerr The argument that gave the error (if there is one).

  const char *err Other error message (if there is one).
  ++++++++++++++++++++++++++++++++++++++*/

static void print_usage(int detail,const char *argerr,const char *err)
{
 fprintf(stderr,
         "Usage: fixme-dumper [--help]\n"
         "                    [--dir=<dirname>]\n"
         "                    [--statistics]\n"
         "                    [--visualiser --latmin=<latmin> --latmax=<latmax>\n"
         "                                  --lonmin=<lonmin> --lonmax=<lonmax>\n"
         "                                  --data=<data-type>]\n"
         "                    [--dump--visualiser [--data=fixme<number>]]\n");

 if(argerr)
    fprintf(stderr,
            "\n"
            "Error with command line parameter: %s\n",argerr);

 if(err)
    fprintf(stderr,
            "\n"
            "Error: %s\n",err);

 if(detail)
    fprintf(stderr,
            "\n"
            "--help                    Prints this information.\n"
            "\n"
            "--dir=<dirname>           The directory containing the fixme database.\n"
            "\n"
            "--statistics              Print statistics about the fixme database.\n"
            "\n"
            "--visualiser              Extract selected data from the fixme database:\n"
            "  --latmin=<latmin>       * the minimum latitude (degrees N).\n"
            "  --latmax=<latmax>       * the maximum latitude (degrees N).\n"
            "  --lonmin=<lonmin>       * the minimum longitude (degrees E).\n"
            "  --lonmax=<lonmax>       * the maximum longitude (degrees E).\n"
            "  --data=<data-type>      * the type of data to select.\n"
            "\n"
            "  <data-type> can be selected from:\n"
            "      fixmes              = fixme tags extracted from the data.\n"
            "\n"
            "--dump-visualiser         Dump selected contents of the database in HTML.\n"
            "  --data=fixme<number>    * the fixme with the selected index.\n");

 exit(!detail);
}

#!/usr/bin/perl
#
# Routino data visualiser CGI
#
# Part of the Routino routing software.
#
# This file Copyright 2008-2013 Andrew M. Bishop
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

# Use the directory paths script
require "paths.pl";

# Use the perl CGI module
use CGI ':cgi';


# Create the query and get the parameters

$query=new CGI;

@rawparams=$query->param;

# Legal CGI parameters with regexp validity check

%legalparams=(
              "latmin"     => "[-0-9.]+",
              "latmax"     => "[-0-9.]+",
              "lonmin"     => "[-0-9.]+",
              "lonmax"     => "[-0-9.]+",
              "data"       => "fixmes",
              "dump"       => "fixme[0-9]+",
              "statistics" => "yes"
             );

# Validate the CGI parameters, ignore invalid ones

foreach my $key (@rawparams)
  {
   foreach my $test (keys (%legalparams))
     {
      if($key =~ m%^$test$%)
        {
         my $value=$query->param($key);

         if($value =~ m%^$legalparams{$test}$%)
           {
            $cgiparams{$key}=$value;
            last;
           }
        }
     }
  }

# Data, dump or statistics?

$data      =$cgiparams{"data"};
$dump      =$cgiparams{"dump"};
$statistics=$cgiparams{"statistics"};

if(!defined $data && !defined $dump && !defined $statistics)
  {
   print header(-status => '500 Invalid CGI parameters');
   exit;
  }

if(defined $statistics)
  {
   # Print the output

   print header('text/plain');

   # Set the parameters

   $params.=" --statistics";
  }
elsif(defined $data)
  {
   # Parameters to limit range selected

   $limits=0.5;

   # Check the parameters

   $latmin=$cgiparams{"latmin"};
   $latmax=$cgiparams{"latmax"};
   $lonmin=$cgiparams{"lonmin"};
   $lonmax=$cgiparams{"lonmax"};

   if($latmin eq "" || $latmax eq "" || $lonmin eq "" || $lonmax eq "" || $data eq "")
     {
      print header(-status => '500 Invalid CGI parameters');
      exit;
     }

   if(($latmax-$latmin)>$limits || ($lonmax-$lonmin)>$limits)
     {
      print header(-status => '500 Selected area too large');
      exit;
     }

   # Print the output

   print header('text/plain');

   print "$latmin $lonmin $latmax $lonmax\n";

   # Set the parameters

   $params.=" --visualiser --data=$data";
   $params.=" --latmin=$latmin --latmax=$latmax --lonmin=$lonmin --lonmax=$lonmax";
  }
else
  {
   # Print the output

   print header('text/plain');

   # Set the parameters

   $params.=" --dump-visualiser --data=$dump";
  }

# Run the filedumper

$params.=" --dir=$data_dir" if($data_dir);
$params.=" --prefix=$data_prefix" if($data_prefix);

system "$bin_dir/$fixme_dumper_exe $params 2>&1";

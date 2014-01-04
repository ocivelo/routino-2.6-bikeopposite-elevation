//
// Routino (extras) fixme web page Javascript
//
// Part of the Routino routing software.
//
// This file Copyright 2008-2013 Andrew M. Bishop
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//


////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Initialisation /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// Process the URL query string and extract the arguments

var legal={"^lon"  : "^[-0-9.]+$",
           "^lat"  : "^[-0-9.]+$",
           "^zoom" : "^[0-9]+$"};

var args={};

if(location.search.length>1)
  {
   var query,queries;

   query=location.search.replace(/^\?/,"");
   query=query.replace(/;/g,'&');
   queries=query.split('&');

   for(var i=0;i<queries.length;i++)
     {
      queries[i].match(/^([^=]+)(=(.*))?$/);

      k=RegExp.$1;
      v=unescape(RegExp.$3);

      for(var l in legal)
        {
         if(k.match(RegExp(l)) && v.match(RegExp(legal[l])))
            args[k]=v;
        }
     }
  }


////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// Map handling /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

var map;
var layerMap=[], layerHighlights, layerVectors, layerBoxes;
var epsg4326, epsg900913;

var box;
var select;

// 
// Initialise the 'map' object
//

function map_init()             // called from fixme.html
{
 lon =args["lon"];
 lat =args["lat"];
 zoom=args["zoom"];

 // Map URLs and limits are in mapprops.js.

 //
 // Create the map
 //

 epsg4326=new OpenLayers.Projection("EPSG:4326");
 epsg900913=new OpenLayers.Projection("EPSG:900913");

 map = new OpenLayers.Map ("map",
                           {
                            controls:[
                                      new OpenLayers.Control.Navigation(),
                                      new OpenLayers.Control.PanZoomBar(),
                                      new OpenLayers.Control.ScaleLine(),
                                      new OpenLayers.Control.LayerSwitcher()
                                      ],

                            projection: epsg900913,
                            displayProjection: epsg4326,

                            minZoomLevel: mapprops.zoomout,
                            numZoomLevels: mapprops.zoomin-mapprops.zoomout+1,
                            maxResolution: 156543.03390625 / Math.pow(2,mapprops.zoomout),

                            // These two lines are not needed with OpenLayers 2.12
                            units: "m",
                            maxExtent:        new OpenLayers.Bounds(-20037508.34, -20037508.34, 20037508.34, 20037508.34),

                            restrictedExtent: new OpenLayers.Bounds(mapprops.westedge,mapprops.southedge,mapprops.eastedge,mapprops.northedge).transform(epsg4326,epsg900913)
                           });

 // Add map tile layers

 for(var l=0;l < mapprops.mapdata.length;l++)
   {
    layerMap[l] = new OpenLayers.Layer.TMS(mapprops.mapdata[l].label,
                                           mapprops.mapdata[l].tileurl,
                                           {
                                            getURL: limitedUrl,
                                            displayOutsideMaxExtent: true,
                                            buffer: 1
                                           });
    map.addLayer(layerMap[l]);
   }

 // Update the attribution if the layer changes

 map.events.register("changelayer",layerMap,change_attribution_event);

 function change_attribution_event(event)
 {
  for(var l=0;l < mapprops.mapdata.length;l++)
     if(this[l] == event.layer)
        change_attribution(l);
 }

 function change_attribution(l)
 {
  var data_url =mapprops.mapdata[l].attribution.data_url;
  var data_text=mapprops.mapdata[l].attribution.data_text;
  var tile_url =mapprops.mapdata[l].attribution.tile_url;
  var tile_text=mapprops.mapdata[l].attribution.tile_text;

  document.getElementById("attribution_data").innerHTML="<a href=\"" + data_url + "\" target=\"data_attribution\">" + data_text + "</a>";
  document.getElementById("attribution_tile").innerHTML="<a href=\"" + tile_url + "\" target=\"tile_attribution\">" + tile_text + "</a>";
 }

 change_attribution(0);

 // Get a URL for the tile (mostly copied from OpenLayers/Layer/XYZ.js).

 function limitedUrl(bounds)
 {
  var res = map.getResolution();

  var x = Math.round((bounds.left - this.maxExtent.left) / (res * this.tileSize.w));
  var y = Math.round((this.maxExtent.top - bounds.top) / (res * this.tileSize.h));
  var z = map.getZoom() + map.minZoomLevel;

  var limit = Math.pow(2, z);
  x = ((x % limit) + limit) % limit;

  var xyz = {'x': x, 'y': y, 'z': z};
  var url = this.url;

  if (OpenLayers.Util.isArray(url))
    {
     var s = '' + xyz.x + xyz.y + xyz.z;
     url = this.selectUrl(s, url);
    }
        
  return OpenLayers.String.format(url, xyz);
 }

 // Add two vectors layers (one for highlights that display behind the vectors)
 
 layerHighlights = new OpenLayers.Layer.Vector("Highlights",{displayInLayerSwitcher: false});
 map.addLayer(layerHighlights);

 layerVectors = new OpenLayers.Layer.Vector("Markers",{displayInLayerSwitcher: false});
 map.addLayer(layerVectors);

 // Handle feature selection and popup

 select = new OpenLayers.Control.SelectFeature(layerVectors,
                                               {onSelect: selectFeature, onUnselect: unselectFeature});

 map.addControl(select);
 select.activate();

 createPopup();

 // Add a boxes layer

 layerBoxes = new OpenLayers.Layer.Boxes("Boundary",{displayInLayerSwitcher: false});
 map.addLayer(layerBoxes);

 box=null;

 // Set the map centre to the limited range specified

 map.setCenter(map.restrictedExtent.getCenterLonLat(), map.getZoomForExtent(map.restrictedExtent,true));
 map.maxResolution = map.getResolution();

 // Move the map

 if(lon != undefined && lat != undefined && zoom != undefined)
   {
    if(lon<mapprops.westedge) lon=mapprops.westedge;
    if(lon>mapprops.eastedge) lon=mapprops.eastedge;

    if(lat<mapprops.southedge) lat=mapprops.southedge;
    if(lat>mapprops.northedge) lat=mapprops.northedge;

    if(zoom<mapprops.zoomout) zoom=mapprops.zoomout;
    if(zoom>mapprops.zoomin)  zoom=mapprops.zoomin;

    var lonlat = new OpenLayers.LonLat(lon,lat);
    lonlat.transform(epsg4326,epsg900913);

    map.moveTo(lonlat,zoom-map.minZoomLevel);
   }

 // Unhide editing URL if variable set

 if(mapprops.editurl != undefined && mapprops.editurl != "")
   {
    edit_url=document.getElementById("edit_url");

    edit_url.style.display="";
    edit_url.href=mapprops.editurl;
   }
}


//
// Format a number in printf("%.5f") format.
//

function format5f(number)
{
 var newnumber=Math.floor(number*100000+0.5);
 var delta=0;

 if(newnumber>=0 && newnumber<100000) delta= 100000;
 if(newnumber<0 && newnumber>-100000) delta=-100000;

 var string=String(newnumber+delta);

 var intpart =string.substring(0,string.length-5);
 var fracpart=string.substring(string.length-5,string.length);

 if(delta>0) intpart="0";
 if(delta<0) intpart="-0";

 return(intpart + "." + fracpart);
}


//
// Build a set of URL arguments for the map location
//

function buildMapArguments()
{
 var lonlat = map.getCenter().clone();
 lonlat.transform(epsg900913,epsg4326);

 var zoom = map.getZoom() + map.minZoomLevel;

 return "lat=" + format5f(lonlat.lat) + ";lon=" + format5f(lonlat.lon) + ";zoom=" + zoom;
}


//
// Update a URL
//

function updateURL(element)     // called from fixme.html
{
 if(element.id == "permalink_url")
    element.href=location.pathname + "?" + buildMapArguments();

 if(element.id == "edit_url")
    element.href=mapprops.editurl + "?" + buildMapArguments();
}


////////////////////////////////////////////////////////////////////////////////
///////////////////////// Popup and selection handling /////////////////////////
////////////////////////////////////////////////////////////////////////////////

var popup=null;

//
// Create a popup - not using OpenLayers because want it fixed on screen not fixed on map.
//

function createPopup()
{
 popup=document.createElement('div');

 popup.className = "popup";

 popup.innerHTML = "<span></span>";

 popup.style.display = "none";

 popup.style.position = "fixed";
 popup.style.top = "-4000px";
 popup.style.left = "-4000px";
 popup.style.zIndex = "100";

 popup.style.padding = "5px";

 popup.style.opacity=0.85;
 popup.style.backgroundColor="#C0C0C0";
 popup.style.border="4px solid #404040";

 document.body.appendChild(popup);
}


//
// Draw a popup - not using OpenLayers because want it fixed on screen not fixed on map.
//

function drawPopup(html)
{
 if(html==null)
   {
    popup.style.display="none";
    return;
   }

 if(popup.style.display=="none")
   {
    var map_div=document.getElementById("map");

    popup.style.left  =map_div.offsetParent.offsetLeft+map_div.offsetLeft+60 + "px";
    popup.style.top   =                                map_div.offsetTop +30 + "px";
    popup.style.width =map_div.clientWidth-100 + "px";

    popup.style.display="";
   }

 popup.innerHTML=html;
}


//
// Select a feature
//

function selectFeature(feature)
{
 if(feature.attributes.dump)
    OpenLayers.Request.GET({url: "fixme.cgi?dump=" + feature.attributes.dump, success: runDumpSuccess});

 layerHighlights.destroyFeatures();

 highlight_style = new OpenLayers.Style({},{strokeColor: "#F0F000",strokeWidth: 8,
                                            fillColor: "#F0F000",pointRadius: 4});

 highlight = new OpenLayers.Feature.Vector(feature.geometry.clone(),{},highlight_style);

 layerHighlights.addFeatures([highlight]);
}


//
// Un-select a feature
//

function unselectFeature(feature)
{
 layerHighlights.destroyFeatures();

 drawPopup(null);
}


//
// Display the dump data
//

function runDumpSuccess(response)
{
 var string=response.responseText;

 if(mapprops.editurl != undefined && mapprops.editurl != "")
   {
    var types=["node", "way", "relation"];

    for(var t in types)
      {
       var type=types[t];

       var regexp=RegExp(type + " id=&apos;[0-9]+&apos;");

       var match=string.match(regexp);

       if(match != null)
         {
          match=String(match);

          var id=match.slice(10+type.length,match.length-6);

          string=string.replace(regexp,type + " id=&apos;<a href='" + mapprops.browseurl + "/" + type + "/" + id + "' target='" + type + id + "'>" + id + "</a>&apos;");
         }
      }
   }

 drawPopup(string.split("&gt;&lt;").join("&gt;<br>&lt;").split("<br>&lt;tag").join("<br>&nbsp;&nbsp;&lt;tag"));
}


////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Server handling ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//
// Display the status
//

function displayStatus(type,subtype,content)
{
 var child=document.getElementById("result_status").firstChild;

 do
   {
    if(child.id != undefined)
       child.style.display="none";

    child=child.nextSibling;
   }
 while(child != undefined);

 var chosen_status=document.getElementById("result_status_" + type);

 chosen_status.style.display="";

 if(subtype != null)
   {
    var format_status=document.getElementById("result_status_" + subtype).innerHTML;

    chosen_status.innerHTML=format_status.replace('#',String(content));
   }
}


//
// Display data statistics
//

function displayStatistics()
{
 // Use AJAX to get the statistics

 OpenLayers.Request.GET({url: "fixme.cgi?statistics=yes", success: runStatisticsSuccess});
}


//
// Success in running data statistics generation.
//

function runStatisticsSuccess(response)
{
 document.getElementById("statistics_data").innerHTML="<pre>" + response.responseText + "</pre>";
 document.getElementById("statistics_link").style.display="none";
}


//
// Get the requested data
//

function displayData(datatype)  // called from fixme.html
{
 // Delete the old data

 unselectFeature();

 select.deactivate();

 layerVectors.destroyFeatures();
 layerHighlights.destroyFeatures();

 if(box != null)
    layerBoxes.removeMarker(box);
 box=null;

 // Print the status

 displayStatus("no_data");

 // Return if just here to clear the data

 if(datatype == "")
    return;

 // Get the new data

 var mapbounds=map.getExtent().clone();
 mapbounds.transform(epsg900913,epsg4326);

 var url="fixme.cgi";

 url=url + "?lonmin=" + mapbounds.left;
 url=url + ";latmin=" + mapbounds.bottom;
 url=url + ";lonmax=" + mapbounds.right;
 url=url + ";latmax=" + mapbounds.top;
 url=url + ";data=" + datatype;

 // Use AJAX to get the data

 OpenLayers.Request.GET({url: url, success: runFixmeSuccess, falure: runFailure});
}


//
// Success in getting the error log data
//

function runFixmeSuccess(response)
{
 var lines=response.responseText.split('\n');

 var style = new OpenLayers.Style({},{stroke: false,
                                      pointRadius: 3,fillColor: "#FF0000",
                                      cursor: "pointer"});

 var features=[];

 for(var line=0;line<lines.length;line++)
   {
    var words=lines[line].split(' ');

    if(line == 0)
      {
       var lat1=words[0];
       var lon1=words[1];
       var lat2=words[2];
       var lon2=words[3];

       var bounds = new OpenLayers.Bounds(lon1,lat1,lon2,lat2).transform(epsg4326,epsg900913);

       box = new OpenLayers.Marker.Box(bounds);

       layerBoxes.addMarker(box);
      }
    else if(words[0] != "")
      {
       var dump=words[0];
       var lat=words[1];
       var lon=words[2];

       var lonlat = new OpenLayers.LonLat(lon,lat).transform(epsg4326,epsg900913);

       var point = new OpenLayers.Geometry.Point(lonlat.lon,lonlat.lat);

       features.push(new OpenLayers.Feature.Vector(point,{dump: dump},style));
      }
   }

 select.activate();

 layerVectors.addFeatures(features);

 displayStatus("data","fixme",lines.length-2);
}


//
// Failure in getting data.
//

function runFailure(response)
{
 displayStatus("failed");
}

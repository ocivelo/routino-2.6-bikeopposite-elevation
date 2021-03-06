//
// Routino data visualiser web page Javascript
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


//
// Data types
//

var data_types=[
                "junctions",
                "super",
                "oneway",
                "highway",
                "transport",
                "barrier",
                "turns",
                "speed",
                "weight",
                "height",
                "width",
                "length",
                "property",
                "errorlogs"
               ];


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

function map_init()             // called from visualiser.html
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

function updateURL(element)     // called from visualiser.html
{
 if(element.id == "permalink_url")
    element.href=location.pathname + "?" + buildMapArguments();

 if(element.id == "router_url")
    element.href="router.html" + "?" + buildMapArguments();

 if(element.id == "edit_url")
    element.href=mapprops.editurl + "?" + buildMapArguments();

 if(element.id.match(/^lang_([a-zA-Z-]+)_url$/))
    element.href="visualiser.html" + "." + RegExp.$1 + "?" + buildMapArguments();
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
    OpenLayers.Request.GET({url: "visualiser.cgi?dump=" + feature.attributes.dump, success: runDumpSuccess});

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
    var Types=["Node", "Way", "Relation"];

    for(var t in types)
      {
       var Type=Types[t];
       var type=types[t];

       var regexp=RegExp(Type + " [0-9]+");

       while((match=string.match(regexp)) != null)
         {
          match=String(match);

          var id=match.slice(1+type.length,match.length);

          string=string.replace(regexp,Type + " <a href='" + mapprops.browseurl + "/" + type + "/" + id + "' target='" + type + id + "'>" + id + "</a>");
         }
      }
   }

 drawPopup(string.split("\n").join("<br>"));
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

 OpenLayers.Request.GET({url: "statistics.cgi", success: runStatisticsSuccess});
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

function displayData(datatype)  // called from visualiser.html
{
 for(var data in data_types)
    hideshow_hide(data_types[data]);

 if(datatype != "")
    hideshow_show(datatype);

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

 var url="visualiser.cgi";

 url=url + "?lonmin=" + mapbounds.left;
 url=url + ";latmin=" + mapbounds.bottom;
 url=url + ";lonmax=" + mapbounds.right;
 url=url + ";latmax=" + mapbounds.top;
 url=url + ";data=" + datatype;

 // Use AJAX to get the data

 switch(datatype)
   {
   case 'junctions':
    OpenLayers.Request.GET({url: url, success: runJunctionsSuccess, failure: runFailure});
    break;
   case 'super':
    OpenLayers.Request.GET({url: url, success: runSuperSuccess, failure: runFailure});
    break;
   case 'oneway':
    OpenLayers.Request.GET({url: url, success: runOnewaySuccess, failure: runFailure});
    break;
   case 'highway':
    var highways=document.forms["highways"].elements["highway"];
    for(var h in highways)
       if(highways[h].checked)
          highway=highways[h].value;
    url+="-" + highway;
    OpenLayers.Request.GET({url: url, success: runHighwaySuccess, failure: runFailure});
    break;
   case 'transport':
    var transports=document.forms["transports"].elements["transport"];
    for(var t in transports)
       if(transports[t].checked)
          transport=transports[t].value;
    url+="-" + transport;
    OpenLayers.Request.GET({url: url, success: runTransportSuccess, failure: runFailure});
    break;
   case 'barrier':
    var transports=document.forms["barriers"].elements["barrier"];
    for(var t in transports)
       if(transports[t].checked)
          transport=transports[t].value;
    url+="-" + transport;
    OpenLayers.Request.GET({url: url, success: runBarrierSuccess, failure: runFailure});
    break;
   case 'turns':
    OpenLayers.Request.GET({url: url, success: runTurnsSuccess, failure: runFailure});
    break;
   case 'speed':
   case 'weight':
   case 'height':
   case 'width':
   case 'length':
    OpenLayers.Request.GET({url: url, success: runLimitSuccess, failure: runFailure});
    break;
   case 'property':
    var properties=document.forms["properties"].elements["property"];
    for(var p in properties)
       if(properties[p].checked)
          property=properties[p].value;
    url+="-" + property;
    OpenLayers.Request.GET({url: url, success: runPropertySuccess, failure: runFailure});
    break;
   case 'errorlogs':
    OpenLayers.Request.GET({url: url, success: runErrorlogSuccess, failure: runFailure});
    break;
   }
}


//
// Success in getting the junctions.
//

function runJunctionsSuccess(response)
{
 var lines=response.responseText.split('\n');

 var junction_colours={
                       0: "#FFFFFF",
                       1: "#FF0000",
                       2: "#FFFF00",
                       3: "#00FF00",
                       4: "#8B4513",
                       5: "#00BFFF",
                       6: "#FF69B4",
                       7: "#000000",
                       8: "#000000",
                       9: "#000000"
                      };

 var styles={};

 for(var colour in junction_colours)
    styles[colour]=new OpenLayers.Style({},{stroke: false,
                                            pointRadius: 2,fillColor: junction_colours[colour],
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
       var count=words[3];

       var lonlat= new OpenLayers.LonLat(lon,lat).transform(epsg4326,epsg900913);

       var point = new OpenLayers.Geometry.Point(lonlat.lon,lonlat.lat);

       features.push(new OpenLayers.Feature.Vector(point,{dump: dump},styles[count]));
      }
   }

 select.activate();

 layerVectors.addFeatures(features);

 displayStatus("data","junctions",lines.length-2);
}


//
// Success in getting the super-node and super-segments
//

function runSuperSuccess(response)
{
 var lines=response.responseText.split('\n');

 var node_style = new OpenLayers.Style({},{stroke: false,
                                           pointRadius: 4,fillColor: "#FF0000",
                                           cursor: "pointer"});

 var segment_style = new OpenLayers.Style({},{fill: false,
                                              strokeWidth: 2,strokeColor: "#FF0000",
                                              cursor: "pointer"});

 var features=[];

 var nodepoint;

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

       var lonlat= new OpenLayers.LonLat(lon,lat).transform(epsg4326,epsg900913);

       var point = new OpenLayers.Geometry.Point(lonlat.lon,lonlat.lat);

       if(dump.charAt(0) == "n")
         {
          nodepoint=point;

          features.push(new OpenLayers.Feature.Vector(point,{dump: dump},node_style));
         }
       else
         {
          var segment = new OpenLayers.Geometry.LineString([nodepoint,point]);

          features.push(new OpenLayers.Feature.Vector(segment,{dump: dump},segment_style));
         }
      }
   }

 select.activate();

 layerVectors.addFeatures(features);

 displayStatus("data","super",lines.length-2);
}


//
// Success in getting the oneway data
//

function runOnewaySuccess(response)
{
 var hex={0: "00", 1: "11",  2: "22",  3: "33",  4: "44",  5: "55",  6: "66",  7: "77",
          8: "88", 9: "99", 10: "AA", 11: "BB", 12: "CC", 13: "DD", 14: "EE", 15: "FF"};

 var lines=response.responseText.split('\n');

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
       var lat1=words[1];
       var lon1=words[2];
       var lat2=words[3];
       var lon2=words[4];

       var lonlat1= new OpenLayers.LonLat(lon1,lat1).transform(epsg4326,epsg900913);
       var lonlat2= new OpenLayers.LonLat(lon2,lat2).transform(epsg4326,epsg900913);

     //var point1 = new OpenLayers.Geometry.Point(lonlat1.lon,lonlat1.lat);
       var point2 = new OpenLayers.Geometry.Point(lonlat2.lon,lonlat2.lat);

       var dlat = lonlat2.lat-lonlat1.lat;
       var dlon = lonlat2.lon-lonlat1.lon;
       var dist = Math.sqrt(dlat*dlat+dlon*dlon)/10;
       var ang  = Math.atan2(dlat,dlon);

       var point3 = new OpenLayers.Geometry.Point(lonlat1.lon+dlat/dist,lonlat1.lat-dlon/dist);
       var point4 = new OpenLayers.Geometry.Point(lonlat1.lon-dlat/dist,lonlat1.lat+dlon/dist);

       var segment = new OpenLayers.Geometry.LineString([point2,point3,point4,point2]);

       var r=Math.round(7.5+7.9*Math.cos(ang));
       var g=Math.round(7.5+7.9*Math.cos(ang+2.0943951));
       var b=Math.round(7.5+7.9*Math.cos(ang-2.0943951));
       var colour = "#" + hex[r] + hex[g] + hex[b];

       var style=new OpenLayers.Style({},{strokeWidth: 2,strokeColor: colour, cursor: "pointer"});

       features.push(new OpenLayers.Feature.Vector(segment,{dump: dump},style));
      }
   }

 select.activate();

 layerVectors.addFeatures(features);

 displayStatus("data","oneway",lines.length-2);
}


//
// Success in getting the highway data
//

function runHighwaySuccess(response)
{
 var lines=response.responseText.split('\n');

 var style = new OpenLayers.Style({},{fill: false,
                                      strokeWidth: 2,strokeColor: "#FF0000",
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
       var lat1=words[1];
       var lon1=words[2];
       var lat2=words[3];
       var lon2=words[4];

       var lonlat1= new OpenLayers.LonLat(lon1,lat1).transform(epsg4326,epsg900913);
       var lonlat2= new OpenLayers.LonLat(lon2,lat2).transform(epsg4326,epsg900913);

       var point1 = new OpenLayers.Geometry.Point(lonlat1.lon,lonlat1.lat);
       var point2 = new OpenLayers.Geometry.Point(lonlat2.lon,lonlat2.lat);

       var segment = new OpenLayers.Geometry.LineString([point1,point2]);

       features.push(new OpenLayers.Feature.Vector(segment,{dump: dump},style));
      }
   }

 select.activate();

 layerVectors.addFeatures(features);

 displayStatus("data","highway",lines.length-2);
}


//
// Success in getting the transport data
//

function runTransportSuccess(response)
{
 var lines=response.responseText.split('\n');

 var style = new OpenLayers.Style({},{fill: false,
                                      strokeWidth: 2,strokeColor: "#FF0000",
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
       var lat1=words[1];
       var lon1=words[2];
       var lat2=words[3];
       var lon2=words[4];

       var lonlat1= new OpenLayers.LonLat(lon1,lat1).transform(epsg4326,epsg900913);
       var lonlat2= new OpenLayers.LonLat(lon2,lat2).transform(epsg4326,epsg900913);

       var point1 = new OpenLayers.Geometry.Point(lonlat1.lon,lonlat1.lat);
       var point2 = new OpenLayers.Geometry.Point(lonlat2.lon,lonlat2.lat);

       var segment = new OpenLayers.Geometry.LineString([point1,point2]);

       features.push(new OpenLayers.Feature.Vector(segment,{dump: dump},style));
      }
   }

 select.activate();

 layerVectors.addFeatures(features);

 displayStatus("data","transport",lines.length-2);
}


//
// Success in getting the barrier data
//

function runBarrierSuccess(response)
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

       var lonlat= new OpenLayers.LonLat(lon,lat).transform(epsg4326,epsg900913);

       var point = new OpenLayers.Geometry.Point(lonlat.lon,lonlat.lat);

       features.push(new OpenLayers.Feature.Vector(point,{dump: dump},style));
      }
   }

 select.activate();

 layerVectors.addFeatures(features);

 displayStatus("data","barrier",lines.length-2);
}


//
// Success in getting the turn restrictions data
//

function runTurnsSuccess(response)
{
 var lines=response.responseText.split('\n');

 var style = new OpenLayers.Style({},{fill: false,
                                      strokeWidth: 2,strokeColor: "#FF0000",
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
       var lat1=words[1];
       var lon1=words[2];
       var lat2=words[3];
       var lon2=words[4];
       var lat3=words[5];
       var lon3=words[6];

       var lonlat1= new OpenLayers.LonLat(lon1,lat1).transform(epsg4326,epsg900913);
       var lonlat2= new OpenLayers.LonLat(lon2,lat2).transform(epsg4326,epsg900913);
       var lonlat3= new OpenLayers.LonLat(lon3,lat3).transform(epsg4326,epsg900913);

       var point1 = new OpenLayers.Geometry.Point(lonlat1.lon,lonlat1.lat);
       var point2 = new OpenLayers.Geometry.Point(lonlat2.lon,lonlat2.lat);
       var point3 = new OpenLayers.Geometry.Point(lonlat3.lon,lonlat3.lat);

       var segments = new OpenLayers.Geometry.LineString([point1,point2,point3]);

       features.push(new OpenLayers.Feature.Vector(segments,{dump: dump},style));
      }
   }

 select.activate();

 layerVectors.addFeatures(features);

 displayStatus("data","turns",lines.length-2);
}


//
// Success in getting the speed/weight/height/width/length limits
//

function runLimitSuccess(response)
{
 var lines=response.responseText.split('\n');

 var node_style = new OpenLayers.Style({},{stroke: false,
                                           pointRadius: 3,fillColor: "#FF0000",
                                           cursor: "pointer"});

 var segment_style = new OpenLayers.Style({},{fill: false,
                                              strokeWidth: 2,strokeColor: "#FF0000",
                                              cursor: "pointer"});

 var features=[];

 var nodepoint;
 var nodelonlat;

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
       var number=words[3];

       var lonlat= new OpenLayers.LonLat(lon,lat).transform(epsg4326,epsg900913);

       if(number == undefined)
         {
          var point = new OpenLayers.Geometry.Point(lonlat.lon,lonlat.lat);

          nodelonlat=lonlat;
          nodepoint = point;

          features.push(new OpenLayers.Feature.Vector(point,{dump: dump},node_style));
         }
       else
         {
          var point = new OpenLayers.Geometry.Point(lonlat.lon,lonlat.lat);

          var segment = new OpenLayers.Geometry.LineString([nodepoint,point]);

          features.push(new OpenLayers.Feature.Vector(segment,{dump: dump},segment_style));

          var dlat = lonlat.lat-nodelonlat.lat;
          var dlon = lonlat.lon-nodelonlat.lon;
          var dist = Math.sqrt(dlat*dlat+dlon*dlon)/120;

          var point = new OpenLayers.Geometry.Point(nodelonlat.lon+dlon/dist,nodelonlat.lat+dlat/dist);

          features.push(new OpenLayers.Feature.Vector(point,{dump: dump},
                                                      new OpenLayers.Style({},{externalGraphic: 'icons/limit-' + number + '.png',
                                                                               graphicYOffset: -9,
                                                                               graphicWidth: 19,
                                                                               graphicHeight: 19})));
         }
      }
   }

 select.activate();

 layerVectors.addFeatures(features);

 displayStatus("data","limit",lines.length-2);
}


//
// Success in getting the property data
//

function runPropertySuccess(response)
{
 var lines=response.responseText.split('\n');

 var style = new OpenLayers.Style({},{fill: false,
                                      strokeWidth: 2,strokeColor: "#FF0000",
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
       var lat1=words[1];
       var lon1=words[2];
       var lat2=words[3];
       var lon2=words[4];

       var lonlat1= new OpenLayers.LonLat(lon1,lat1).transform(epsg4326,epsg900913);
       var lonlat2= new OpenLayers.LonLat(lon2,lat2).transform(epsg4326,epsg900913);

       var point1 = new OpenLayers.Geometry.Point(lonlat1.lon,lonlat1.lat);
       var point2 = new OpenLayers.Geometry.Point(lonlat2.lon,lonlat2.lat);

       var segment = new OpenLayers.Geometry.LineString([point1,point2]);

       features.push(new OpenLayers.Feature.Vector(segment,{dump: dump},style));
      }
   }

 select.activate();

 layerVectors.addFeatures(features);

 displayStatus("data","property",lines.length-2);
}


//
// Success in getting the error log data
//

function runErrorlogSuccess(response)
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

 displayStatus("data","errorlogs",lines.length-2);
}


//
// Failure in getting data.
//

function runFailure(response)
{
 displayStatus("failed");
}

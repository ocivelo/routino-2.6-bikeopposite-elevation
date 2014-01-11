routino-2.6-bikeopposite-elevation
==================================

routino take into account opposite_lane for bikes and elevation inspired from Pavel Zbytovský
Routino : OpenStreetMap Routing Software
                  ========================================


   Routino is an application for finding a route between two points using
   the dataset of topographical information collected by
   http://www.OpenStreetMap.org.

   more information in file readme.txt
   
   Status
   ------
   Version 2.6 of Routino was released on 6th July 2013.
   
   Copyright
   ---------
   Routino is copyright Andrew M. Bishop 2008-2013.
                 ========================================
                 
history of modifications 
------------------------
   The first modif. is to take into account openstreetmap way tag : cycleway = opposite_lane 
   the bikes are autorized to take a oneway street in the two directions. 
   
   The second modif. is to use quickest route research in order to avoid big elevation ways for bikes. 
   first, it uses the tag : incline to fix % of elevation of a way; if not defined, it uses NASA srtm data to 
   fix elevation of each segment. the srtm part is inspired from Pavel Zbytovský 
      (https://github.com/zbycz/routino/commit/30e8eb086aab1f3724da2f2319caf9f7efdc5e46) 
   The duration calculated for bikes is just an indication and has no purpose to be accurate. 
   corrections have been made on Pavel algorithm to make it work correctly . 
   
   Why the choice to create a new repository ?
     because I m new in using Github, so I didn't know how to contribute on Pavel own repo.
     because of the use of the ultimate version of routino available (2.6) 
     because the first modifications (opposite_lane and incline tags) were made before knowing of Pavel contribution.
     because It's not using modifications on profile parameters (ie hills and Lengh) made by Pavel.
     because There is some not english language stuff in Pavel repository that I don't understand. 
     
   change 07/01/2014 
      correction in taking into account elevation on supersegments
      output speed calculated by Duration (real speed used to calculate duration of the journey on a segment )
   change 11/01/2014 
      Adding french version for language translations.xml, router.html.fr and .en, and change on router.js to change language
   

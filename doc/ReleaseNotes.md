# v01-09-04

* 2022-03-18 Thomas Madlener ([PR#10](https://github.com/iLCSoft/CED/pull/10))
  - Make sure to link against GLUT libraries

* 2022-03-18 Thomas Madlener ([PR#9](https://github.com/iLCSoft/CED/pull/9))
  - Migrate CI to github actions.

# v01-09-03

* 2017-07-26 Shaojun Lu ([PR#5](https://github.com/iLCSoft/CED/pull/5))
  - Fix RPATH for CVMFS (ilc.desy.de) installation
   - Do not add the system directory "/usr/lib64" into the executable files "glced" RPATH list, it will be able to find the right "libstdc++.so.6" from CERN CVMFS compiler with "LD_LIBRARY_PATH" setup.
   - It cannot have influence on sim/reco.

# CED Release notes


 
## v01-09-02 

   M /CED/trunk/src/server/glced.cc
 - replace << std::cout w/ std::endl ( bug for c++11)



## v01-09-01 

     - fixed wrappper function ced_hit(...,type,...) to call 
       ced_hit_ID(...., type & 0xFF,type >> CED_LAYER_SHIFT ,....)
       for backward compatibility with old code


## v01-09 

    - turned off C++ name mangling in client library libCED.so/dylib
      to facilitate calling ced from python


## v01-08 


New features (A. Miyamoto):

    - Add "Auto shot" flag in Tools menu of glced.
      When set, glced write a screen image to a file when new event is received from
      CED client. The file is written in a directory where glced is executed with
      a name, glced-N.tga, where N is a sequence number.  If an environment variable,
      CED_IMAGEFILE is set, output file name is ${CED_IMAGEFILE}-N.tga.  Note that
      a file could be written at any place by including a directory in ${CED_IMAGEFILE}.

      A menu, Tools->Auto scale, can be used to select the scale of output image.

    - A CMake variable, CED_NOT_INCLUDE_OPENGL_LINKER_PATH.  If set,
      OpenGL linker path is not included in rpath of glced. Inclusion of
      the OpenGL path caused a problem at run time when gcc used for OpenGL
      is older than those used to build glced.

    - Add 2 more CED_HIT_TYPE, CED_HIT_BOX and CED_HIT_VXD. Then will be used
      to display pixel hits of FPCCD.

New feature (O. Volynets):

    - Added shortcut ctrl+s for fast saving of screenshots in original size (i.e. without scaling)


## v01-07 


Fixes/Changes:

    - Updated the manual to the latest CED version

    - Deleted the old pictures
    
    - New pictures are stored in doc/manual/img/
    
    - Added undo to help frame 
    
    - Fixed typo in help frame text

    - Fixed a bug with the screenshot function where the axis are drawn at a wrong possition

    - Improve the identification of hits/detector components when 
      pick object while  detector picking is enabled

    - Fixed an issue with the popup menu which causes a visible menu 
      when right clicking at the same position.

    - The distance calculation between two objects is 
      now also affected by selecting objects via double click

    - Instead of highlighting the picked point now the whole detector 
      component is highlighted (only visable when detector picking 
      is enabled and picking marker enabled).
    
    - When selecting a detector component by doubleclick this component 
      is now also active to modify only this component via shortkeys.   

    - Fixed menu detector transparency checkboxes


## v01-06 

NOTE: Old config files (created by CED by save settings) are not longer readable, sorry!

New features (H.Hoelbe, DESY):
    - split reset ced into two parts: reset everything (shortcut R), reset view (shortcut r)
    - menu and popup menu expands now to left/top if right/button is not enough space.
    - update the manual (not finished yet)
    - Undo function is build in. (shortkey: 'u')
    - Replaced the GLUT popup menu by a selfmade main menu drawn at the top of the window and a selfmade context based popup menu. 
      (The popup menu entrys depend on with object you click)
    - It is now possible to pick detector components
    - Marker which highlights the last picked object 
        - Toggle this option in menu->graphics->picking marker. (Default option is off)

    Menus:
        - Popup/Main menu entrys now shown a little triangle at the right when they are have subitems
        - Add a menu option with allows to change the text size
        - Main menu now show current status of
            - axis visibility
            - layers
            - detector
            - transparency
            - projections
            - cuts
            - graphics settings
            - font size
            - fps
            - help
            - additionaly the screenshot function show the pixel x pixel size of the image
            - New available cut values for angle and z-cut added to popup and main menu
            - Selecting a detector component by righclick now allows to change its transparency (>,<), cutangle (m,M) and z-cut value (z,Z)
            - Selecting the background allows to change transparency, cutangle, z-cut value for all components starting at the individual values of each component
            - Save/Load now saves the individual values for each detector component
            - popup menu now allow to change detector transparence and phi-cut exclusive for one specific layer
         - Layer menu now contains visible/not visible flag
         - popup menu now contains data and detector layers too
    - Add some of the normals to the draw detector function to improve lighting, still under construction!
    - Layer description text is now shown in popup menu


Small changes (H. Hoelbe, DESY):
    - X,Y,Z-Axis now always get drawn at the point (0,0,0) (not obligatory at the center of the detector).
      I think this change will bring more overview when moving the detector and rotate it after this.
    
    - shortcut help frame now changes color depending on background color

    - It is now possible to decrease transparency by using the '<'-key to 0. (before it stops at an value of 0.05)
      (transparency of 0.0 is special because at this value only the outline of the detector will drawn)
    
    - In the DST viewer, a legend get drawn. Now the color of this legend will switch from white to black
      when the background color is to light. (white letters on white background was not readable)
    
    - Detector outline lines will now always set to no transparency. (default value in ced_config.h)
    
    - arrow keys moves the detector:
          arrow key left: z-axis,
          right arrow: -z-axis,
          up-arraw: y-axis,  (new)
          down-arrow: -y-axis (new)


Bugfixes (H. Hoelbe, DESY):
    - layer visibility  is now shown correctly in the menus
    - layer description is now shown in the menus
    - undo function is moved from shortkey 'u' to x or ctrl+z
    - undo function now does not cause in strange behavior when pressing after startup
    - save light settings
    - screenshot with light
    - load default settings
    - toggle z and phi projection
    - fixed picking problem (calculate the distance for each method in the same way)
    - axis are now drawn always in the middle of the detector (enquired by frank)
    - subsubmenu and submenu are now able to draw horizontal lines
    - data and detector layer menus (main and popup) now shows the layer description
    - workaround for mac - draw bogus GLUT menu
    - Fixed bug in the picking line feature
    - Shortcut frame: now works correct for diverent font sizes
    - Popup menu now supports subitems
    - Popup menu for detector components now shows phi-cut submenu
    - Removed compiler warnings
    - Reset CED (shortkey: 'R'): Now also reset light and the visibly of the axis
    - Save function now also store the position, picking marker and detector picking.
    - Fixed an issue with the popup menu.





## v01-05 


    - Z-cutting now only affects the detector components, not the hits,tracks etc
      this change has been made to be consistency with phi-cutting

    - Fisheye options are now in the ced_config file 

    - Improve the look of geocylinder objects

    (- Front view and front projection now shows the detector from the different side)

    - Add ced_config.h: collects all options which can be set before build CED



## v01-04-01 


    - fixed memory leak
    - changes method of adding pickable points to lines from splitting lines to add more points to.
    - add new flag client_connected to avoid crashes after picking when client ist disconnected 




## v01-04 


    - New features (H. Hoelbe, DESY):
        * Add screenshot function
            - Select screenshot from popup menu, resolution options depending on the size of your glced window. 
              This will save a screenshot under /tmp/glced.tga. 
              To convert the screenshot into a compressed format Use for example "convert /tmp/glced.tga screenshot.png".
        * New help frame
            - New concept: not a extra window. Insteat a orthogonal projection inside of the original ced window
              this should fix: focus problems, crashes, and its transparence now
            - Detector and data layer now also have a visibility field which is shown
            - Help frame now change its size in hight so that always all elements fit into it
    
    - Changes/Bugfixes
        * Removed grafical artifact in detector components who have differ amount of inner and outer edges
        * Outer line of detector are now smooth and are drawn at least to remove a grafical artifact
        * Add fading into the background color for bigger distance as a menu feature.
        * New order of popup menu items
        * Add some new shortcut keys
        * Add 4 extra slots to save user settings
        * fixed typo S for F and added brackets for ((a && b) || (c && d))  if condition
        * renamed MAX_LAYER to CED_MAX_LAYER in order to avoid name clashes with other tools (e.g. calice event display )




## v01-03 


   - New features (H. Hoelbe, DESY):
        * Save/Load settings option (saves zoom, background color, view position, etc in ~/.glced)
        * Detector components have now layers and layer description
        * Increased the number of available layers 
        * Add side view projection (press 'S' to toggle)
        * Add front view projection (press 'F' to toggle)
        * Add perspectivical option (objects far away apairs smaller)
        * Its possible to toggle the visible of the axes (select it from the menu)
        * Show frames per secounds (select it from the menu) 
        * Enhanced fisheye view by adding zoom levels when view mode toggles,
          or when switches to side, front or reset view

        * Add a new optional draw style for the detector, called "New view" includes: 
            * Detector cuts: longitudinal (press and hold 'z' or 'Z') 
              and transversal cuts (select it from the menu) 
            * Its possible to change the value of the transparence (menu)
            * in 3d-view also added a line model for better identification
            * Inner edges can be different from outeredges, and the inner cylinder can be 
              rotated with an other angle as the outer cylinder shape
            * Its possible to set a flag in GEO_Tube objects, so that they only 
              get drawn in new view, see marlinCED.c for an example


   * Fixes/Intern stuff (H. Hoelbe, DESY):
        * Fixed mouse-wheel functionality for Ubuntu
        * Changed CED from a c to a c++ project
        * Change order of drawing items 
        * Removed zoom level reset from side and front view
        * Update deepfilter and blend function


    - added OpenGL library paths to the rpath list for preventing LD_LIBRARY_PATH
        (set by geant4 env init script) to overwrite the opengl system libraries used
        in the linking of glced with the ones from Mesa (installed in afs), which might
        cause a dramatic performance penalty



## v01-02 



    New features (H. Hoelbe, DESY): 
        - Remote access to CED: 
            * Per default CED rejects connects from remote hosts, to
              enable remote access, start CED with option --trust <hostname>. 

            * Example: To allow remote access from host xyz: 
                   glced --trust xyz

        - Use user defined port:
            * To start ced several times on the same host, it is necessary 
              that each instance of CED use a different port. To do that, 
              you are able to set the CED port by set the enviroment 
              variable CED_PORT.

            * Example: Start CED listen on port 8888:
                    export CED_PORT=8888
                    glced

            * Hint: To connect on this CED with Marlin you must set
              the enviroment variable CED_PORT (and for remote access 
              also CED_HOST) before starting Marlin.

    Fixes/Intern stuff
        - Move (Middle mouse button): 
               * Now works correct with rotate
               * Adjusted the move speed, to get the same speed for
                 each zoom level and window size.

        - rotate3d:
              * Helper function: rotates a vector in the 3d room

        - ced_geobox_r_ID:
              * do the same as ced_geobox_r (plus lcio_id) but use
                ced_line_ID, this ensure working picking, fisheye view and
                layerdescription. All other geobox functions ced_geobox_r,
                ced_geobox_ID, ced_geobox, now can use this new function
                instead of having own code.

    Simplified CMakeLists ( added dependency to ILCUTIL package )



## v01-01 


    new features: (H. Hoelbe, DESY)
    - Add popup menu, to change background color, toggle layer visibly etc.
      Rightclick to open/close the menu
    - Add help frame, press 'h' to open/close this frame.
      This frame show shortcuts and the layer descriptions.
    - Viewers are able to add their layer descriptions to CED. This 
      descriptions are shown in the popup menu and the help frame. To 
      add layer descriptions use MarlinCED:set_layer_description or
      MarlinCED:add_layer_description. 
    - Added test_ced_mhits (draws 1.000.000 random hits)

    changes/bug fixes: (H. Hoelbe, DESY)
    -  Make CED faster by adapt the frequence when the socket get read.
    -  Removed compiler warnings
    -  Zoom in/out are additional added to +/-
    -  More possible background colors
    -  User defined background color are now also accessible by pressing 'b' 
       or using the popup menu
    -  removed old makefiles


## v01-00  

     new features: (H. Hoelbe, DESY)
     -  enabled picking (double-click on objects)
     -  zoom (mouse-wheel)
     -  backgroud color option (e.g. glced -bgcolor black)
     -  new ced_write_text function

      merged from calice branch:
       - filled boxes
       - fixed perspective
       - b-key toggles background color 
         (light-blue, black, gray shades, white)
	
     bug fix: 
        -  CED no longer crashes when drawing more 
           than ~350.000 hits (H. Hoelbe)


## v00-07   

    * added new Fisheye view functionality 
    * added server client communication 
    * started to add picking functionality
    (S. Martin-Haugh)        


## v00-06   

    * added new functionality to draw (S. Darasz):
        - rotated and translated geo_cylinders
        - transparent and wireframe ellipsoids
        - cones and boxes
        - code to display a color map legend
    * cmake improvements:
        - added CED_LIBRARY_DIRS
        - added 32 bit compatibility build option
        - made cmake 2.6 compatible
        - bug fix for building ced server on mac


## v00-05 

      * restructered build process: 
        default is to build true client library libCED.so 
        - optionally also build CED server glced (needs glut/OpenGL)
          with cmake switch -D CED_SERVER=ON 

      * install only ced_cli.h 


## v00-04-01 

    - made headers dual use (C/C++)
 

## v00-04 

     -  improved CMakeLists.txt: (F.Gaede)
	    create true client library libCED.so 
        
     -  corrected implementation of crosses for displaying hits (S.Aplin)
	 -  added layer functionality to CED_GeoBoxR
	 -  added a unique id variable (int) to CED_Hit, which is printed to
		screen whenever the 'C' key is pressed to center on a hit


 ## v00-03 

    - minor fixes
    - improved test_ced.c with more examples
    - improved MacOSX support (cmake)
 

 ## v00-02 

     - new build scripts for CMake (J.Engels, DESY)
     - added ced_geobox for displaying a box with offset in x,y 
       (author A.Bulgheroni, INFN) 
	 - ported to MAC (thanks to E. Corrin)  (need to build with cmake)  
     F.Gaede, DESY:
     - added world_size command line parameter to glced
     - added help message for "glced -help [ -h, -?] "
     - replaced fixed size window geometry with geometry comand-line option
     - added simple example program test_ced (only built with cmake)
       

 ## v00-01   

     - initial release in marlinreco cvs

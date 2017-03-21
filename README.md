# CED (C Event Display)
[![Build Status](https://travis-ci.org/iLCSoft/CED.svg?branch=master)](https://travis-ci.org/iLCSoft/CED)

CED is a server client application for OpenGL drawing

CED is distributed under the [GPLv3 License](http://www.gnu.org/licenses/gpl-3.0.en.html)

[![License](https://www.gnu.org/graphics/gplv3-127x51.png)](https://www.gnu.org/licenses/gpl-3.0.en.html)

## Build instructions

For building CED from source you need to have CMake and CMakeModules for
ilcsoft installed in your system. ( cmake homepage: www.cmake.org )
once cmake is in your $PATH, follow this steps:

	mkdir build
	cd build
	cmake -DCMAKE_MODULE_PATH=/path/to/ilcsoft_CMakeModules [-DCED_SERVER=ON] ..
	make install

**NOTE**: Only the CED client library is built per default!!

If you also need the display server (glced), you have to
explicitly set the cmake option CED_SERVER to ON and you
need GLUT installed on your system ( http://freeglut.sourceforge.net )

If cmake cannot find GLUT you can use the variables
```CMAKE_LIBRARY_PATH``` and ```CMAKE_INCLUDE_PATH``` to find libraries
and header files in non-standard paths, e.g.:

	export CMAKE_LIBRARY_PATH=/path/to/glut/lib
	export CMAKE_INCLUDE_PATH=/path/to/glut/include
    cmake ..
    make install


## Users Manual
The users manual can be found at [./doc/manual.pdf](./doc/manual.pdf)


## glced command line options

	glced -h  # show help
	glced     # run event display server


## Controls for CED display server (glced window):

The window has to be active and the mouse placed inside it.


### Keyboard shortcuts:

		ESC .......... quit
		CTRL+s ....... save screenshot
		keys [0-9] ... toggle visibility layers
		key 'r' ...... redispaly in some initial projection
		key 'f' ...... front projection
		key 's' ...... side projection
		key 'c' ...... move center of rotation to the nearest hit within current mouse position
		key 'v' ...... fish-eye view
		key 'b' ...... toggle background color
		key 'h' ...... display help menu



### Mouse interaction:

		L button + drag:    rotate
		R button + drag:    pop-up menu
		M button + drag:    shift
		M wheel  + up  :    zoom-in
		M wheel  + down:    zoom-out




## Using the CED client library with Marlin:

Somewhere at the beginning of file ******Processor.cc

		#include <ced_cli.h>
		
		//   Visibility layers types: should be included as:
		#define PHOTON_LAYER (0<<CED_LAYER_SHIFT)
		#define PHOTON_LAYER (1<<CED_LAYER_SHIFT)
		#define NHADR_LAYER  (2<<CED_LAYER_SHIFT)
		#define CHADR_LAYER  (3<<CED_LAYER_SHIFT)
		#define TPC_LAYER    (4<<CED_LAYER_SHIFT)
		#define ECAL_LAYER   (5<<CED_LAYER_SHIFT)
		#define HCAL_LAYER   (6<<CED_LAYER_SHIFT)
		     ...........................
		#define SOME_LAYER   (19<<CED_LAYER_SHIFT)


Once per task in first init: 

	  void *********Processor::init() { 
	  //    Initialize event display   
	    ced_client_init("localhost",7286);
	    ced_register_elements();


In the first called ```Processor::processEvent( LCEvent * evt )```
initialize the geometry:

		//-----------------------------------------------------------------------
		  ced_new_event();  // Reset drawing buffer and START drawing collection
		  //          The Simplest geometry has Z axis as beam axis
		  static CED_GeoCylinder geoCylinders[] = {       // for TESLA Detector Geometry
		    {    50.0,  6,  0.0, 5658.5, -5658.5, 0xff      }, // beam tube
		    {   380.0, 24,  0.0, 2658.5, -2658.5, 0xff      }, // inner TPC
		    {  1840.0,  8, 22.5, 2700.0, -2700.0, 0x7f7f1f  }, // inner ECAL
		    {  2045.7,  8, 22.5, 2700.0, -2700.0, 0x7f7f1f  }, // outer ECAL
		    {  2045.7,  8, 22.5, 101.00,  2820.0, 0x7f7f1f  }, // endcap ECAL
		    {  2045.7,  8, 22.5, 101.00, -3022.0, 0x7f7f1f  }, // endcap ECAL
		    {  3000.0, 16,  0.0, 2658.5, -2658.5, 0xcf00    }, // outer HCAL
		    {  3000.0,  8, 22.5, 702.25,  2826.0, 0xcf00    }, // endcap HCAL
		    {  3000.0,  8, 22.5, 702.25, -4230.5, 0xcf00    }, // endcap HCAL
		// radius,poligon order,angle degree,1/2 length,shift in z, color
		  }; 
		
		  /*
		  static CED_GeoCylinder geoCylinders[] = {    // for Prototype
		    {    180.0,  4,  45.0, 110.0, 0.0, 0xff }, // ECAL
		    {    500.0,  4,  45.0, 250.0, 220., 0xff } // HCAL
		  };
		  */
		
		  ced_geocylinders(sizeof(geoCylinders)/sizeof(CED_GeoCylinder),geoCylinders);
		//------------------------------------------------------------------

One can put here as many cylinders as one want.

In any Processor:

		  void ********Processor::processEvent( LCEvent * evt ) { 
		
		//  hit/point types are predefined as
		//    CED_HIT_POINT=0,
		//    CED_HIT_CROSS=1,
		//    CED_HIT_STAR=2,
		
		//in loop:   one can call either hit drawing or line drawing with functions:
		
		// void ced_hit(float x,float y,float z,unsigned type,unsigned size,unsigned color);
		// void ced_line(float x0,float y0,float z0,
		//	      float x1,float y1,float z1,
		//	      unsigned type,unsigned width,unsigned color);


to define colors is better use gimp - it gives that crasy numbers easily

		// Possible type combinations:
		
		// real call
		     for (int i=0; i<nn,i++){ // along hit/point collection or so
		       kcol = 0xf2290e;
		//  ECAL_LAYER-- mean hit will be switch on/off by keyboad key number 5 and 
		//       will draw as CED_POINT i.e. . 
		 	 ced_hit(cc_red[kk][nn].x,cc_red[kk][nn].y,cc_red[kk][nn].z,ECAL_LAYER,3,kcol);
		
		       kcol = 0x25cb17;
		//  HCAL_LAYER|2 -- mean hit will be switch on/off by keyboad key number 6 and 
		//  will draw as CED_HIT_STAR i.e. * 
			 ced_hit(cc_blue[kk][nn].x,cc_blue[kk][nn].y,cc_blue[kk][nn].z,HCAL_LAYER|2,3,kcol);
		
		//or:
		         ced_line(x1,y1,z1,x2,y2,z2, CHADR_LAYER , 1, 0x7af774);
		}
		
		// after loop   
		   ced_send_event();    // Does not clean up screen
		
		
		// at the end of event 
		  ced_draw_event();    // Make clean up screen before drawing


## License and Copyright
Copyright (C) 2005-2017, CED Authors

CED is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License long with this program.  If not, see <http://www.gnu.org/licenses/>.

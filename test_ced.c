

#include <ced_cli.h>
#include <math.h>

double length = 100. ;

/** Simpl client test/example program. 
 *  Draw a white box with edge lengths 100 mm centered at the origin
 *  and two boxes with 50 mm lengths with lower left (pink) and upper right corner (blue)
 *  respectively at the origin.
 *  And finally an enclosing octagonal barrel.
 * 
 *  @author gaede
 */

int main(){

  ced_client_init("localhost",7286); // FIXME: this should be a parameter

  ced_register_elements();
  
  // ----- white box -------
  // geocylinder actually draws a 'polygonal cylinder' along z axis
  // with outer(!) radius, n-edges, rotation in degrees, half-z, z-offset, color 
  ced_geocylinder( M_SQRT2*length , 4, 45. , length , -length , 0xffffff ) ;
    

  //  ----- pink box -------
  double sizes[3] = { length , length ,  length } ;
  double center[3] = { length/2. , length/2. ,  length/2. } ;
  //   size: lengths along x,y,z (not half lengths !) , center: shift vector
  ced_geobox( sizes,  center,  0xff00ff );

  //  ----- blue box -------
  double center_1[3] = { -length/2. , -length/2. ,  -length/2. } ;
  ced_geobox( sizes,  center_1 ,  0x00ffff );


  //  ----- yellow octagonal barrel -------
  ced_geocylinder( M_SQRT2*length , 8 , 45. , 2.*length , -2.*length , 0xffff00 ) ;



  //--------------------------------------------------------------------

  ced_send_event();    // Does not clean up screen
  
  ced_draw_event();    // Make clean up screen before drawing

 return 0 ;
}

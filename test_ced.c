

#include <ced_cli.h>
#include <math.h>


/** Simpl client test/example program. 
 *  Draw a white box with edge lengths 100 mm centered at the origin. 
 *  @author gaede
 */

int main(){

  ced_client_init("localhost",7286); // FIXME: this should be a parameter

  ced_register_elements();
  
  double l = 100. ;
  // geocylinder actually draws a 'polygonal cylinder' along z axis
  // with outer(!) radius, n-edges, rotation in degrees, half-z, z-offset, color 
  ced_geocylinder( sqrt(2.)*l , 4, 45. , l , -l , 0xffffff ) ;
    
  ced_send_event();    // Does not clean up screen
  
  ced_draw_event();    // Make clean up screen before drawing

 return 0 ;
}

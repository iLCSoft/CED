#include <ced_cli.h>
#include <math.h>

double length = 100;

/** Simple test programme (C)
 * 
 *  Check the ced_geobox and ced_geobox_r
 * 
 *  @author SD
 * 	@date 22.08.08
 */

int main(){

	/*
	 * Initialisation */
  	ced_client_init("localhost",7286);
  	ced_register_elements();
  
  	/*
  	 * White cylinder
   	 * outer(!) radius, n-edges, rotation in degrees, half-z, z-offset, color */
  	ced_geocylinder(M_SQRT2*length, 100, 45, length, -length, 0xffffff);
    

  	/*
   	 * Pink box
   	 * size: lengths along x,y,z (not half lengths !) , center: shift vector */
  	double sizes[3] = {length, length, length};
  	double center[3] = {0.0, 0.0, 0.0} ;
  	ced_geobox( sizes,  center,  0xff00ff );
  	
  	/*
   	 * Red rotated box
   	 * size: lengths along x,y,z (not half lengths !) , center: shift vector */
  	double sizes_r[3] = {2*length, 2*length, 2*length};
  	double center_r[3] = {0.0, 0.0, 0.0};
  	double theta_r1 = 20;
  	double theta_r2 = 20;
  	double theta_r3 = 20;
  	double rotation_r[3] = {theta_r1, theta_r2, theta_r3};
  	
    //float RGBAcolor[4] = {1, 0.3, 0.4, 0.4} ;
  	
  	ced_geobox_r( sizes_r,  center_r,  rotation_r, 0x990000, 7);
  	
  	/*
   	 * Another, yet white, rotated box
   	 * size: lengths along x,y,z (not half lengths !) , center: shift vector */
//   	unsigned int i = 0;
//   	for (i; i<360; i = i+4){
//  		double sizes_w[3] = {2*length, 2*length, 2*length};
//  		double center_w[3] = {0.0, 0.0, 0.0};
//  		double theta_w1 = 0+i;
//  		double theta_w2 = 0+i;
//  		double theta_w3 = 0+i;
//  		double rotation_w[3] = {theta_w1, theta_w2, theta_w3};
//  		int color = 0x000000;
//  		//int color = 0x660000 + 100*i;
//	  	ced_geobox_r( sizes_w,  center_w,  rotation_w, color);
//   	 }


  	/*
  	 * Yellow octagonal barrel */
  	ced_geocylinder( M_SQRT2*length , 8 , 45. , 2.*length , -2.*length , 0xffff00 ) ;
  	/** cone */
  	//double center_c[3] = {10, 10, 10};
  	//double rotation_c[3] = {0,0,0};
  	//ced_cone_r(10.0, 100, center_c, rotation_c, 1, RGBAcolor);


	double sizes_eli[3] = {50, 50, 150};
	double center_eli[3] = {0, 0, 0};
	double rotation_eli[3] = {0, 0, 0};
	//ced_cluellipse_r(sizes_eli[0], sizes_eli[2], center_eli,  rotation_eli, 1, RGBAcolor);
	ced_ellipsoid_r(sizes_eli, center_eli,  rotation_eli, 1, 0x99999999);
  	ced_send_event();    // Does not clean up screen
  	ced_draw_event();    // Make clean up screen before drawing

 return 0;
}

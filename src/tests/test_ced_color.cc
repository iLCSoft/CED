#include <ced_cli.h>
#include <math.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>

double length = 5;

/** Simple test programme (C)
 * 
 *  Check the ced_geobox and ced_geobox_r
 * 
 *  @author SD
 * 	@date 22.08.08
 */

//SM-H: Declare ced_do_draw_event as extern to avoid warning
//extern void ced_do_draw_event();
int main(){

	/*
	 * Initialisation */
        char *p;
        p = getenv("CED_PORT");
        if(p != NULL){
            printf("Try to use user defined port %s.\n", p);
            ced_client_init("localhost",atoi(p));
        }else{
            printf("CED_PORT undefined. Using CED_PORT=7286.\n");
            ced_client_init("localhost",7286);
        }

  	ced_register_elements();
  
  	/*
  	 * White cylinder
   	 * outer(!) radius, n-edges, rotation in degrees, half-z, z-offset, color */
//  	ced_geocylinder(M_SQRT2*length, 100, 45, length, -length, 0xffffff);
    

  	/*
   	 * Pink box
   	 * size: lengths along x,y,z (not half lengths !) , center: shift vector */
  	double sizes[3] = {length, length, length};
  	double center[3] = {0.0, 0.0, 0.0} ;
        double dx=10.0;
        center[0]=-180.0;
//        double th=90.0;
//        double dth=10.0;
#if 0        
        double pi=acos(-1.0);
        for(int i=0;i<36;i++) {
          center[0]+=dx;
          th+=dth;
          unsigned int ired=(unsigned int)(255*sin(th/180.0*pi));
          unsigned int iblue=(unsigned int)(255*sin((th+120.0)/180.0*pi));
          unsigned int igreen=(unsigned int)(255*sin((th+240.0)/180.0*pi));
          unsigned int icolor=(256*256*ired+256*iblue+igreen)%(256*256*256+256*256+256);
   
  	  ced_geobox( sizes,  center,  icolor );
          std::cout << " center[0]=" << center[0] << "  " ;
          std::cout << std::hex << icolor << std::endl; 
        }  	
#endif
        unsigned int coldat[24]={
           0x000044, 0x000088, 0x0000bb, 0x0000ff, 
           0x0044ff, 0x0088ff, 0x00bbff, 0x00ffff,
           0x00ffbb, 0x00ff88, 0x00ff44, 0x00ff00,
           0x44ff00, 0x88ff00, 0xbbff00, 0xffff00, 
           0xffbb00, 0xff8800, 0xff4400, 0xff0000,
           0xff4444, 0xff8888, 0xffbbbb, 0xffffff};

        for(int i=0;i<24;i++) {
          center[0] += dx;
          unsigned int icolor=coldat[i];
  	  ced_geobox( sizes,  center,  icolor );
          std::cout << " center[0]=" << center[0] ;
          std::cout << std::hex << icolor << std::endl; 
        }

#if 0
        for(int j=0;j<3;j++) {
           int index0[3]={2,0,1};
           int index1[3]={0,1,2};
           int index2[3]={1,2,0};
           for(int i=0;i<10;i++) {
              center[0]+=dx;
              unsigned int ic[3]={0,0,0};
              double th=dx*i;
              unsigned int ic1=(int)fabs(255.0*cos(th/180.0*pi));
              unsigned int ic2=(int)fabs(255.0*sin(th/180.0*pi));
              ic[index0[j]]=ic1;
              ic[index1[j]]=ic2;
              ic[index2[j]]=0;
              unsigned int icolor=(ic[0]*0xff00 + ic[1]*0xff + ic[2]);
  	      ced_geobox( sizes,  center,  icolor );
              std::cout << " center[0]=" << center[0] << " th=" << th << "  ic1=" << ic1 << " ic2=" << ic2 << "   ";
              std::cout << std::hex << icolor << std::endl; 
            }   
        }
#endif

  	/*
   	 * Red rotated box
   	 * size: lengths along x,y,z (not half lengths !) , center: shift vector */
/*
  	double sizes_r[3] = {2*length, 2*length, 2*length};
  	double center_r[3] = {0.0, 0.0, 0.0};
  	double theta_r1 = 20;
  	double theta_r2 = 20;
  	double theta_r3 = 20;
  	double rotation_r[3] = {theta_r1, theta_r2, theta_r3};
*/
  	
    //float RGBAcolor[4] = {1, 0.3, 0.4, 0.4} ;
  	
//  	ced_geobox_r( sizes_r,  center_r,  rotation_r, 0x990000, 7);
  	
//        center_r[0]=100.0;  center_r[1]=100.0; center_r[2]=200.0;
//        sizes_r[0]=length; 
//        ced_geobox_r_solid( sizes_r, center_r, rotation_r, 0x995500, 7);
  	/*
   	 * Another, yet white, rotated box
   	 * size: lengths along x,y,z (not half lengths !) , center: shift vector */
  	//ced_do_draw_event();    // Make clean up screen before drawing
  	ced_draw_event();    // Make clean up screen before drawing

 return 0;
}

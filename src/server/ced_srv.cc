/* "C" event display.
 * Server side elements definitions.
 *
 * Alexey Zhelezov, DESY/ITEP, 2005 */
#include<iostream>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <X11/Xlib.h>
#include <GL/glut.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <ced_cli.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

//hauke
//#include "glced.h"
int graphic[3];
double cut_angle;

static int mouse_x, mouse_y; 

#include <ced.h>
#define PORT        0x1234

/** This defines what is visible */
unsigned ced_visible_layers=0x00000FFF;
int SELECTED_ID = -1;

//hauke
int SELECTED_X=0;
int SELECTED_Y=0;

extern double fisheye_alpha;
#define IS_VISIBLE(x) ((1<<((x>>8)&0xff))&ced_visible_layers)

/*
 * To support mouse operations with objects, we need
 * object screen coordinates.
 */
static GLdouble modelM[16];
static GLdouble projM[16];
static GLint    viewport[4];

typedef struct {
  unsigned int ID;
  int x; // in window
  int y;
  int max_dxy; // after this distance, ignore this object
  CED_Point p; // object real coordinates (can't use pointer...)
} CED_ObjMap;

static CED_ObjMap *omap=0;
static unsigned omap_count=0;
static unsigned omap_alloced=0;

//SM-H: Takes a given point, and returns the fisheye transformed version. Based on transform given in
//'Event display: Can We See What We Want to See', 
//H. Drevermann, D. Kuhn, B.S. Nilsson, 1995
//TODO: More elegant (and eficient) implementation possible. 
//See http://www.opengl.org/discussion_boards/ubbthreads.php?ubb=showflat&Number=262910
CED_Point fisheye_transform(const float x, const float y, const float z, const double scale_factor) {
  CED_Point p_final;
  if(scale_factor < 1e-10) {
    //If fisheye_alpha < observable value, do nothing
    p_final.x = x;
    p_final.y = y;
    p_final.z = z;
  }
  else {
    float rho = sqrt(x*x + y*y);
    rho = rho/(1.0+scale_factor*rho);
    float r = sqrt(rho*rho+z*z);
    float cos_theta = z/r;
    float theta = acos(cos_theta);
    float phi = atan2(y,x); 
    p_final.x = r*cos(phi)*sin(theta);
    p_final.y = r*sin(phi)*sin(theta);
    p_final.z = z/(1.0 + fisheye_alpha*abs(z));
  }
  return p_final;
}
//SM-H: The same as above, but just applied to r or z rather than a whole cartesian co-ordinate system
//CED co-ordinates only defined up to float precision
inline float single_fisheye_transform(float c, const double scale_factor) {
  return c/(1.0+scale_factor*fabs(c)); 
}


void drawPartialLineCylinder(double length, double R /*radius*/, double iR /*inner radius*/, int edges, double angle_cut_off, double angle_cut_off_left, bool outer_face=1, bool inner_face=1){
#define PI 3.14159265358979323846f 
    double phi=360.0/edges;
    int i,j;
    double x, xl;

    glPushMatrix();

    glTranslatef(0, 0, length/2);

    
    //draw the two ends
    for(j=0;j<2;j++){
        if(j==0){glTranslatef(0, 0, -length/2);}
        else if(j==1){glTranslatef(0, 0, length);}

        phi=(360.0/edges);

        i=(int) angle_cut_off_left/phi+1;  
        phi=(360.0/edges)*(i+edges);
        xl = cos(2*PI/edges/2)/cos((360- (phi-360/edges/2)+angle_cut_off_left)*2*PI/360);

        if(inner_face){
        glBegin(GL_LINE);
        glVertex2d(iR*sin(360.0/edges*(i)*2*PI/360.0),iR*cos(360.0/edges*(i)*2*PI/360.0));
        glVertex2d(iR*xl*sin((angle_cut_off_left)*2*PI/360),iR*xl*cos((angle_cut_off_left)*2*PI/360));
        glEnd();
        }
        
        if(outer_face){
        glBegin(GL_LINE);
        glVertex2d(R*xl*sin((angle_cut_off_left)*2*PI/360.0),R*xl*cos(angle_cut_off_left*2*PI/360.0));
        glVertex2d(R*sin(360.0/edges*(i)*2*PI/360.0),R*cos(360.0/edges*(i)*2*PI/360.0));
        glEnd();
        }

        glBegin(GL_LINE_STRIP );
        glVertex2d(R*sin(360.0/edges*(i)*2*PI/360.0),R*cos(360.0/edges*(i)*2*PI/360.0));

        i=i+1; 


        for(;i<edges+1;i++){
            phi=360.0/edges*i;
            if(360.0-phi <= angle_cut_off){
                x = cos(2*PI/edges/2)/cos((360- (phi-360/edges/2)-angle_cut_off)*2*PI/360);

                if(outer_face){
                glVertex2d(R*x*sin((360-angle_cut_off)*2*PI/360), R*x*cos((360-angle_cut_off)*2*PI/360));
                glVertex2d(R*x*sin((360-angle_cut_off)*2*PI/360), R*x*cos((360-angle_cut_off)*2*PI/360));
                }
                break;
            }else{
                if(outer_face){
                glVertex2d(R*sin(phi*2*PI/360), R*cos(phi*2*PI/360));
                if(i != 0){
                    glVertex2d(R*sin(phi*2*PI/360), R*cos(phi*2*PI/360));
                }
                }
            }
        }
        glEnd();



        glBegin(GL_LINE_STRIP);
        i=(int) angle_cut_off_left/phi+1;  
        i=i+1; 

        glVertex2d(iR*sin(360.0/edges*(i)*2*PI/360.0),iR*cos(360.0/edges*(i)*2*PI/360.0));


        for(;i<edges+1;i++){
            phi=360.0/edges*i;
            if(360.0-phi <= angle_cut_off){
                x = cos(2*PI/edges/2)/cos((360- (phi-360/edges/2)-angle_cut_off)*2*PI/360);

                if(inner_face){
                glVertex2d(iR*sin(360/edges*(i-1)*2*PI/360), iR*cos(360/edges*(i-1)*2*PI/360));
                glVertex2d(iR*x*sin((360-angle_cut_off)*2*PI/360), iR*x*cos((360-angle_cut_off)*2*PI/360));
                }
                break;
            }else{
                //glVertex2d(R*sin(phi*2*PI/360), R*cos(phi*2*PI/360));
                if(inner_face){
                if(i != 0){
                    glVertex2d(iR*sin(360/edges*(i-1)*2*PI/360), iR*cos(360/edges*(i-1)*2*PI/360));
                    glVertex2d(iR*sin(phi*2*PI/360), iR*cos(phi*2*PI/360));
                }
                }
            }
        }
        glEnd();

    }

   glTranslatef(0, 0, -length/2);

    //close 2 to cuts, if cutting

    if(angle_cut_off > 0.0 || angle_cut_off_left > 0.0){
        if(outer_face){

        glBegin(GL_LINES);
        glVertex3d(R*x*sin((360-angle_cut_off)*2*PI/360), R*x*cos((360-angle_cut_off)*2*PI/360), -length/2);
        glVertex3d(R*x*sin((360-angle_cut_off)*2*PI/360), R*x*cos((360-angle_cut_off)*2*PI/360), length/2);
        glEnd();

        glBegin(GL_LINES);
        glVertex3d(iR*x*sin((360-angle_cut_off)*2*PI/360), iR*x*cos((360-angle_cut_off)*2*PI/360), length/2);
        glVertex3d(R*x*sin((360-angle_cut_off)*2*PI/360), R*x*cos((360-angle_cut_off)*2*PI/360), length/2);
        glEnd();

        glBegin(GL_LINES);
        glVertex3d(R*x*sin((360-angle_cut_off)*2*PI/360), R*x*cos((360-angle_cut_off)*2*PI/360), -length/2);
        glVertex3d(iR*x*sin((360-angle_cut_off)*2*PI/360), iR*x*cos((360-angle_cut_off)*2*PI/360), -length/2);
        glEnd();

        }
        if(inner_face){
        glBegin(GL_LINES);
        glVertex3d(iR*x*sin((360-angle_cut_off)*2*PI/360), iR*x*cos((360-angle_cut_off)*2*PI/360), length/2);
        glVertex3d(iR*x*sin((360-angle_cut_off)*2*PI/360), iR*x*cos((360-angle_cut_off)*2*PI/360), -length/2);
        glEnd();

        glBegin(GL_LINES);
        glVertex3d(iR*x*sin((360-angle_cut_off)*2*PI/360), iR*x*cos((360-angle_cut_off)*2*PI/360), length/2);
        glVertex3d(R*x*sin((360-angle_cut_off)*2*PI/360), R*x*cos((360-angle_cut_off)*2*PI/360), length/2);
        glEnd();

        glBegin(GL_LINES);
        glVertex3d(R*x*sin((360-angle_cut_off)*2*PI/360), R*x*cos((360-angle_cut_off)*2*PI/360), -length/2);
        glVertex3d(iR*x*sin((360-angle_cut_off)*2*PI/360), iR*x*cos((360-angle_cut_off)*2*PI/360), -length/2);
        glEnd();
        }

        if(outer_face){

        glBegin(GL_LINES);
        glVertex3d(R*xl*sin((angle_cut_off_left)*2*PI/360), R*xl*cos((angle_cut_off_left)*2*PI/360), -length/2);
        glVertex3d(R*xl*sin((angle_cut_off_left)*2*PI/360), R*xl*cos((angle_cut_off_left)*2*PI/360), length/2);
        glEnd();

        glBegin(GL_LINES);
        glVertex3d(iR*xl*sin((angle_cut_off_left)*2*PI/360),iR*xl*cos((angle_cut_off_left)*2*PI/360), length/2);
        glVertex3d(R*xl*sin((angle_cut_off_left)*2*PI/360), R*xl*cos((angle_cut_off_left)*2*PI/360), length/2);
        glEnd();
       
        glBegin(GL_LINES);
        glVertex3d(iR*xl*sin((angle_cut_off_left)*2*PI/360),iR*xl*cos((angle_cut_off_left)*2*PI/360), -length/2);
        glVertex3d(R*xl*sin((angle_cut_off_left)*2*PI/360), R*xl*cos((angle_cut_off_left)*2*PI/360), -length/2);
        glEnd();

        }
        if(inner_face){
        glBegin(GL_LINES);
        glVertex3d(iR*xl*sin((angle_cut_off_left)*2*PI/360),iR*xl*cos((angle_cut_off_left)*2*PI/360), length/2);
        glVertex3d(iR*xl*sin((angle_cut_off_left)*2*PI/360),iR*xl*cos((angle_cut_off_left)*2*PI/360), -length/2);

        glBegin(GL_LINES);
        glVertex3d(iR*xl*sin((angle_cut_off_left)*2*PI/360),iR*xl*cos((angle_cut_off_left)*2*PI/360), length/2);
        glVertex3d(R*xl*sin((angle_cut_off_left)*2*PI/360), R*xl*cos((angle_cut_off_left)*2*PI/360), length/2);
        glEnd();

        glBegin(GL_LINES);
        glVertex3d(iR*xl*sin((angle_cut_off_left)*2*PI/360),iR*xl*cos((angle_cut_off_left)*2*PI/360), -length/2);
        glVertex3d(R*xl*sin((angle_cut_off_left)*2*PI/360), R*xl*cos((angle_cut_off_left)*2*PI/360), -length/2);
        glEnd();

        }
    }


    //draw the cylinder

    if(edges < 20){
    phi=(360.0/edges);
    i=(int) angle_cut_off_left/phi+1;  
    //outer
    if(outer_face){
    glBegin(GL_LINE_LOOP);
    glVertex3d(R*xl*sin((angle_cut_off_left)*2*PI/360), R*xl*cos((angle_cut_off_left)*2*PI/360), -length/2);
    glVertex3d(R*xl*sin((angle_cut_off_left)*2*PI/360), R*xl*cos((angle_cut_off_left)*2*PI/360), length/2);
    glVertex3d(R*sin(360.0/edges*(i)*2*PI/360.0),R*cos(360.0/edges*(i)*2*PI/360.0), length/2);
    glVertex3d(R*sin(360.0/edges*(i)*2*PI/360.0),R*cos(360.0/edges*(i)*2*PI/360.0), -length/2);
    glEnd();
    }


    if(inner_face){
    if(iR > 0){
    //inner
    glBegin(GL_LINE_LOOP);
    glVertex3d(iR*sin(360.0/edges*(i)*2*PI/360.0),iR*cos(360.0/edges*(i)*2*PI/360.0), -length/2);
    glVertex3d(iR*sin(360.0/edges*(i)*2*PI/360.0),iR*cos(360.0/edges*(i)*2*PI/360.0), length/2);
    glVertex3d(iR*xl*sin((angle_cut_off_left)*2*PI/360),iR*xl*cos((angle_cut_off_left)*2*PI/360), length/2);
    glVertex3d(iR*xl*sin((angle_cut_off_left)*2*PI/360),iR*xl*cos((angle_cut_off_left)*2*PI/360), -length/2);
    glEnd();
    }
    }


    i=(int) angle_cut_off_left/phi+2;  


    for(;i<edges+1;i++){
        phi=360.0/edges*i;
        double phi2=360.0/edges*(i-1);

        if(360.0-phi <= angle_cut_off){
            double x = cos(2*PI/edges/2)/cos((360- (phi-360/edges/2)-angle_cut_off)*2*PI/360);

            if(outer_face){
            //outer
            glBegin(GL_LINE_LOOP);
            glVertex3d(R*sin(phi2*2*PI/360), R*cos(phi2*2*PI/360),-length/2);
            glVertex3d(R*sin(phi2*2*PI/360), R*cos(phi2*2*PI/360),length/2);
            glVertex3d(R*x*sin((360-angle_cut_off)*2*PI/360), R*x*cos((360-angle_cut_off)*2*PI/360), length/2);
            glVertex3d(R*x*sin((360-angle_cut_off)*2*PI/360), R*x*cos((360-angle_cut_off)*2*PI/360), -length/2);
            glEnd();
            }

            if(iR > 0){
            if(inner_face){
            //inner:
            glBegin(GL_LINE_LOOP);
            glVertex3d(iR*sin(phi2*2*PI/360), iR*cos(phi2*2*PI/360),-length/2);
            glVertex3d(iR*sin(phi2*2*PI/360), iR*cos(phi2*2*PI/360),length/2);
            glVertex3d(iR*x*sin((360-angle_cut_off)*2*PI/360), iR*x*cos((360-angle_cut_off)*2*PI/360), length/2);
            glVertex3d(iR*x*sin((360-angle_cut_off)*2*PI/360), iR*x*cos((360-angle_cut_off)*2*PI/360), -length/2);
            glEnd();
            }
            }
            break;
        }else{
            if(outer_face){
            //outer
            glBegin(GL_LINE_LOOP);
            glVertex3d(R*sin(phi2*2*PI/360), R*cos(phi2*2*PI/360),-length/2);
            glVertex3d(R*sin(phi2*2*PI/360), R*cos(phi2*2*PI/360),length/2);
            glVertex3d(R*sin(phi*2*PI/360), R*cos(phi*2*PI/360), length/2);
            glVertex3d(R*sin(phi*2*PI/360), R*cos(phi*2*PI/360), -length/2);
            glEnd();
            }

            if(iR > 0){
            if(inner_face){
            //inner
            glBegin(GL_LINE_LOOP);
            glVertex3d(iR*sin(phi2*2*PI/360), iR*cos(phi2*2*PI/360),-length/2);
            glVertex3d(iR*sin(phi2*2*PI/360), iR*cos(phi2*2*PI/360),length/2);
            glVertex3d(iR*sin(phi*2*PI/360), iR*cos(phi*2*PI/360), length/2);
            glVertex3d(iR*sin(phi*2*PI/360), iR*cos(phi*2*PI/360), -length/2);
            glEnd();
            }
            }
        }
    }
    //glEnd();
    }
glPopMatrix();

}


void drawPartialCylinder_backup(double length, double R /*radius*/, double iR /*inner radius*/, int o_edges, int i_edges, double angle_cut_off, double angle_cut_off_left, double rotate_i){
//std::cout << "outer edges: " << o_edges << "  inner edges:" << i_edges << "  cut right: " << angle_cut_off << "  cut left: " << angle_cut_off_left << " rotate inner cylinder: " << rotate_i << std::endl;
return;
/*
#define PI 3.14159265358979323846f 
    double phi=360.0/edges;
    int i,j;
    double x, xl;
    

    glPushMatrix();

    glTranslatef(0, 0, length/2);

    
    //draw the two ends
    for(j=0;j<2;j++){
        if(j==0){glTranslatef(0, 0, -length/2);}
        else if(j==1){glTranslatef(0, 0, length);}


//        glBegin(GL_LINE_STRIP);
        //glBegin(GL_TRIANGLE_STRIP );
        //glVertex2d(0,iR);



        //i=(int) angle_cut_off_left/phi + edges + 1;  
        //i+=edges;
        phi=(360.0/edges);

        i=(int) angle_cut_off_left/phi+1;  
        phi=(360.0/edges)*(i+edges);
        xl = cos(2*PI/edges/2)/cos((360- (phi-360/edges/2)+angle_cut_off_left)*2*PI/360);

        glBegin(GL_TRIANGLES);
        glVertex2d(R*xl*sin((angle_cut_off_left)*2*PI/360), R*xl*cos((angle_cut_off_left)*2*PI/360));
        glVertex2d(iR*sin(360.0/edges*(i)*2*PI/360.0),iR*cos(360.0/edges*(i)*2*PI/360.0));
        glVertex2d(iR*xl*sin((angle_cut_off_left)*2*PI/360),iR*xl*cos((angle_cut_off_left)*2*PI/360));
        glEnd();
        
        

        glBegin(GL_TRIANGLES);
        glVertex2d(R*xl*sin((angle_cut_off_left)*2*PI/360.0),R*xl*cos(angle_cut_off_left*2*PI/360.0));
        glVertex2d(R*sin(360.0/edges*(i)*2*PI/360.0),R*cos(360.0/edges*(i)*2*PI/360.0));
        glVertex2d(iR*sin(360.0/edges*(i)*2*PI/360.0),iR*cos(360.0/edges*(i)*2*PI/360.0));
        glEnd();


        //phi=(360.0/edges)*i;
        //i=(int) angle_cut_off_left/phi+1;  


        glBegin(GL_TRIANGLE_STRIP );
        glVertex2d(R*sin(360.0/edges*(i)*2*PI/360.0),R*cos(360.0/edges*(i)*2*PI/360.0));
        glVertex2d(iR*sin(360.0/edges*(i)*2*PI/360.0),iR*cos(360.0/edges*(i)*2*PI/360.0));

        i=i+1; 


        for(;i<edges+1;i++){
            phi=360.0/edges*i;
            if(360.0-phi <= angle_cut_off){
                x = cos(2*PI/edges/2)/cos((360- (phi-360/edges/2)-angle_cut_off)*2*PI/360);
                glVertex2d(R*x*sin((360-angle_cut_off)*2*PI/360), R*x*cos((360-angle_cut_off)*2*PI/360));
                glVertex2d(iR*sin(360/edges*(i-1)*2*PI/360), iR*cos(360/edges*(i-1)*2*PI/360));
                glVertex2d(iR*x*sin((360-angle_cut_off)*2*PI/360), iR*x*cos((360-angle_cut_off)*2*PI/360));
                glVertex2d(R*x*sin((360-angle_cut_off)*2*PI/360), R*x*cos((360-angle_cut_off)*2*PI/360));
                break;
            }else{
                glVertex2d(R*sin(phi*2*PI/360), R*cos(phi*2*PI/360));
                if(i != 0){
                    glVertex2d(iR*sin(360/edges*(i-1)*2*PI/360), iR*cos(360/edges*(i-1)*2*PI/360));
                    glVertex2d(iR*sin(phi*2*PI/360), iR*cos(phi*2*PI/360));
                    glVertex2d(R*sin(phi*2*PI/360), R*cos(phi*2*PI/360));
                }
            }
        }
        glEnd();
    }

   glTranslatef(0, 0, -length/2);
    //close 2 to cuts, if cutting

    if(angle_cut_off > 0.0 || angle_cut_off_left > 0.0){
        glBegin(GL_QUADS);
        glVertex3d(R*x*sin((360-angle_cut_off)*2*PI/360), R*x*cos((360-angle_cut_off)*2*PI/360), -length/2);
        glVertex3d(R*x*sin((360-angle_cut_off)*2*PI/360), R*x*cos((360-angle_cut_off)*2*PI/360), length/2);
        glVertex3d(iR*x*sin((360-angle_cut_off)*2*PI/360), iR*x*cos((360-angle_cut_off)*2*PI/360), length/2);
        glVertex3d(iR*x*sin((360-angle_cut_off)*2*PI/360), iR*x*cos((360-angle_cut_off)*2*PI/360), -length/2);
        glEnd();

        glBegin(GL_QUADS);
        //phi=0.0;

        glVertex3d(R*xl*sin((angle_cut_off_left)*2*PI/360), R*xl*cos((angle_cut_off_left)*2*PI/360), -length/2);
        glVertex3d(R*xl*sin((angle_cut_off_left)*2*PI/360), R*xl*cos((angle_cut_off_left)*2*PI/360), length/2);
        glVertex3d(iR*xl*sin((angle_cut_off_left)*2*PI/360),iR*xl*cos((angle_cut_off_left)*2*PI/360), length/2);
        glVertex3d(iR*xl*sin((angle_cut_off_left)*2*PI/360),iR*xl*cos((angle_cut_off_left)*2*PI/360), -length/2);
        glEnd();
    }

    //draw the cylinder

    glEnd();
    phi=(360.0/edges);
    i=(int) angle_cut_off_left/phi+1;  
    //outer
    glBegin(GL_QUADS);
    glVertex3d(R*xl*sin((angle_cut_off_left)*2*PI/360), R*xl*cos((angle_cut_off_left)*2*PI/360), -length/2);
    glVertex3d(R*xl*sin((angle_cut_off_left)*2*PI/360), R*xl*cos((angle_cut_off_left)*2*PI/360), length/2);

    glVertex3d(R*sin(360.0/edges*(i)*2*PI/360.0),R*cos(360.0/edges*(i)*2*PI/360.0), length/2);
    glVertex3d(R*sin(360.0/edges*(i)*2*PI/360.0),R*cos(360.0/edges*(i)*2*PI/360.0), -length/2);
    glEnd();


    //inner
    glBegin(GL_QUADS);
    glVertex3d(iR*sin(360.0/edges*(i)*2*PI/360.0),iR*cos(360.0/edges*(i)*2*PI/360.0), -length/2);
    glVertex3d(iR*sin(360.0/edges*(i)*2*PI/360.0),iR*cos(360.0/edges*(i)*2*PI/360.0), length/2);
    glVertex3d(iR*xl*sin((angle_cut_off_left)*2*PI/360),iR*xl*cos((angle_cut_off_left)*2*PI/360), length/2);
    glVertex3d(iR*xl*sin((angle_cut_off_left)*2*PI/360),iR*xl*cos((angle_cut_off_left)*2*PI/360), -length/2);
    glEnd();


    i=(int) angle_cut_off_left/phi+2;  


    for(;i<edges+1;i++){
        phi=360.0/edges*i;
        double phi2=360.0/edges*(i-1);

        if(360.0-phi <= angle_cut_off){
            double x = cos(2*PI/edges/2)/cos((360- (phi-360/edges/2)-angle_cut_off)*2*PI/360);

            //outer
            //glBegin(GL_LINE_LOOP);
            glBegin(GL_QUADS);
            glVertex3d(R*sin(phi2*2*PI/360), R*cos(phi2*2*PI/360),-length/2);
            glVertex3d(R*sin(phi2*2*PI/360), R*cos(phi2*2*PI/360),length/2);
            glVertex3d(R*x*sin((360-angle_cut_off)*2*PI/360), R*x*cos((360-angle_cut_off)*2*PI/360), length/2);
            glVertex3d(R*x*sin((360-angle_cut_off)*2*PI/360), R*x*cos((360-angle_cut_off)*2*PI/360), -length/2);
            glEnd();

            //inner:
            glBegin(GL_QUADS);
            glVertex3d(iR*sin(phi2*2*PI/360), iR*cos(phi2*2*PI/360),-length/2);
            glVertex3d(iR*sin(phi2*2*PI/360), iR*cos(phi2*2*PI/360),length/2);
            glVertex3d(iR*x*sin((360-angle_cut_off)*2*PI/360), iR*x*cos((360-angle_cut_off)*2*PI/360), length/2);
            glVertex3d(iR*x*sin((360-angle_cut_off)*2*PI/360), iR*x*cos((360-angle_cut_off)*2*PI/360), -length/2);
            glEnd();
            break;
        }else{
            //outer
            //glBegin(GL_LINE_LOOP);
            glBegin(GL_QUADS);
            glVertex3d(R*sin(phi2*2*PI/360), R*cos(phi2*2*PI/360),-length/2);
            glVertex3d(R*sin(phi2*2*PI/360), R*cos(phi2*2*PI/360),length/2);
            glVertex3d(R*sin(phi*2*PI/360), R*cos(phi*2*PI/360), length/2);
            glVertex3d(R*sin(phi*2*PI/360), R*cos(phi*2*PI/360), -length/2);
            glEnd();

            //inner
            glBegin(GL_QUADS);
            glVertex3d(iR*sin(phi2*2*PI/360), iR*cos(phi2*2*PI/360),-length/2);
            glVertex3d(iR*sin(phi2*2*PI/360), iR*cos(phi2*2*PI/360),length/2);
            glVertex3d(iR*sin(phi*2*PI/360), iR*cos(phi*2*PI/360), length/2);
            glVertex3d(iR*sin(phi*2*PI/360), iR*cos(phi*2*PI/360), -length/2);
            glEnd();
        }
    }
    //glEnd();
glPopMatrix();

*/
}
/* Draw a Cylinder
*/
void drawPartialCylinder(double length, double R /*radius*/, double iR /*inner radius*/, int edges, double angle_cut_off, double angle_cut_off_left, bool outer_face=1, bool inner_face=1){
//std::cout << "outer edges: " << o_edges << "  inner edges:" << i_edges << "  cut right: " << angle_cut_off << "  cut left: " << angle_cut_off_left << " rotate inner cylinder: " << rotate_i << std::endl;
//std::cout << "outer_face " << outer_face << " inner_face: " << inner_face << std::endl;
#define PI 3.14159265358979323846f 
    double phi=360.0/edges;
    int i,j;
    double x, xl;
    

    glPushMatrix();

    glTranslatef(0, 0, length/2);
    //draw the two ends
    for(j=0;j<2;j++){
        if(j==0){glTranslatef(0, 0, -length/2);}
        else if(j==1){glTranslatef(0, 0, length);}


//        glBegin(GL_LINE_STRIP);
        //glBegin(GL_TRIANGLE_STRIP );
        //glVertex2d(0,iR);



        //i=(int) angle_cut_off_left/phi + edges + 1;  
        //i+=edges;
        phi=(360.0/edges);

        i=(int) angle_cut_off_left/phi+1;  
        phi=(360.0/edges)*(i+edges);
        xl = cos(2*PI/edges/2)/cos((360- (phi-360/edges/2)+angle_cut_off_left)*2*PI/360);

        glBegin(GL_TRIANGLES);
        glVertex2d(R*xl*sin((angle_cut_off_left)*2*PI/360), R*xl*cos((angle_cut_off_left)*2*PI/360));
        glVertex2d(iR*sin(360.0/edges*(i)*2*PI/360.0),iR*cos(360.0/edges*(i)*2*PI/360.0));
        glVertex2d(iR*xl*sin((angle_cut_off_left)*2*PI/360),iR*xl*cos((angle_cut_off_left)*2*PI/360));
        glEnd();
        
        

        glBegin(GL_TRIANGLES);
        glVertex2d(R*xl*sin((angle_cut_off_left)*2*PI/360.0),R*xl*cos(angle_cut_off_left*2*PI/360.0));
        glVertex2d(R*sin(360.0/edges*(i)*2*PI/360.0),R*cos(360.0/edges*(i)*2*PI/360.0));
        glVertex2d(iR*sin(360.0/edges*(i)*2*PI/360.0),iR*cos(360.0/edges*(i)*2*PI/360.0));
        glEnd();


        //phi=(360.0/edges)*i;
        //i=(int) angle_cut_off_left/phi+1;  


        glBegin(GL_TRIANGLE_STRIP );
        glVertex2d(R*sin(360.0/edges*(i)*2*PI/360.0),R*cos(360.0/edges*(i)*2*PI/360.0));
        glVertex2d(iR*sin(360.0/edges*(i)*2*PI/360.0),iR*cos(360.0/edges*(i)*2*PI/360.0));

        i=i+1; 


        for(;i<edges+1;i++){
            phi=360.0/edges*i;
            if(360.0-phi <= angle_cut_off){
                x = cos(2*PI/edges/2)/cos((360- (phi-360/edges/2)-angle_cut_off)*2*PI/360);
                glVertex2d(R*x*sin((360-angle_cut_off)*2*PI/360), R*x*cos((360-angle_cut_off)*2*PI/360));
                glVertex2d(iR*sin(360/edges*(i-1)*2*PI/360), iR*cos(360/edges*(i-1)*2*PI/360));
                glVertex2d(iR*x*sin((360-angle_cut_off)*2*PI/360), iR*x*cos((360-angle_cut_off)*2*PI/360));
                glVertex2d(R*x*sin((360-angle_cut_off)*2*PI/360), R*x*cos((360-angle_cut_off)*2*PI/360));
                break;
            }else{
                glVertex2d(R*sin(phi*2*PI/360), R*cos(phi*2*PI/360));
                if(i != 0){
                    glVertex2d(iR*sin(360/edges*(i-1)*2*PI/360), iR*cos(360/edges*(i-1)*2*PI/360));
                    glVertex2d(iR*sin(phi*2*PI/360), iR*cos(phi*2*PI/360));
                    glVertex2d(R*sin(phi*2*PI/360), R*cos(phi*2*PI/360));
                }
            }
        }
        glEnd();
    }

   glTranslatef(0, 0, -length/2);
    //close 2 to cuts, if cutting

    if(angle_cut_off > 0.0 || angle_cut_off_left > 0.0){
        glBegin(GL_QUADS);
        glVertex3d(R*x*sin((360-angle_cut_off)*2*PI/360), R*x*cos((360-angle_cut_off)*2*PI/360), -length/2);
        glVertex3d(R*x*sin((360-angle_cut_off)*2*PI/360), R*x*cos((360-angle_cut_off)*2*PI/360), length/2);
        glVertex3d(iR*x*sin((360-angle_cut_off)*2*PI/360), iR*x*cos((360-angle_cut_off)*2*PI/360), length/2);
        glVertex3d(iR*x*sin((360-angle_cut_off)*2*PI/360), iR*x*cos((360-angle_cut_off)*2*PI/360), -length/2);
        glEnd();

        glBegin(GL_QUADS);
        //phi=0.0;

        glVertex3d(R*xl*sin((angle_cut_off_left)*2*PI/360), R*xl*cos((angle_cut_off_left)*2*PI/360), -length/2);
        glVertex3d(R*xl*sin((angle_cut_off_left)*2*PI/360), R*xl*cos((angle_cut_off_left)*2*PI/360), length/2);
        glVertex3d(iR*xl*sin((angle_cut_off_left)*2*PI/360),iR*xl*cos((angle_cut_off_left)*2*PI/360), length/2);
        glVertex3d(iR*xl*sin((angle_cut_off_left)*2*PI/360),iR*xl*cos((angle_cut_off_left)*2*PI/360), -length/2);
        glEnd();
    }

    //draw the cylinder

    glEnd();
    phi=(360.0/edges);
    i=(int) angle_cut_off_left/phi+1;  

    if(outer_face == true){
    //outer
    glBegin(GL_QUADS);
    glVertex3d(R*xl*sin((angle_cut_off_left)*2*PI/360), R*xl*cos((angle_cut_off_left)*2*PI/360), -length/2);
    glVertex3d(R*xl*sin((angle_cut_off_left)*2*PI/360), R*xl*cos((angle_cut_off_left)*2*PI/360), length/2);

    glVertex3d(R*sin(360.0/edges*(i)*2*PI/360.0),R*cos(360.0/edges*(i)*2*PI/360.0), length/2);
    glVertex3d(R*sin(360.0/edges*(i)*2*PI/360.0),R*cos(360.0/edges*(i)*2*PI/360.0), -length/2);
    glEnd();
    }


    if(inner_face == true){
    //inner
    glBegin(GL_QUADS);
    glVertex3d(iR*sin(360.0/edges*(i)*2*PI/360.0),iR*cos(360.0/edges*(i)*2*PI/360.0), -length/2);
    glVertex3d(iR*sin(360.0/edges*(i)*2*PI/360.0),iR*cos(360.0/edges*(i)*2*PI/360.0), length/2);
    glVertex3d(iR*xl*sin((angle_cut_off_left)*2*PI/360),iR*xl*cos((angle_cut_off_left)*2*PI/360), length/2);
    glVertex3d(iR*xl*sin((angle_cut_off_left)*2*PI/360),iR*xl*cos((angle_cut_off_left)*2*PI/360), -length/2);
    glEnd();
    }


    i=(int) angle_cut_off_left/phi+2;  


    for(;i<edges+1;i++){
        phi=360.0/edges*i;
        double phi2=360.0/edges*(i-1);

        if(360.0-phi <= angle_cut_off){
            double x = cos(2*PI/edges/2)/cos((360- (phi-360/edges/2)-angle_cut_off)*2*PI/360);

            if(outer_face == true){
            //outer
            glBegin(GL_QUADS);
            glVertex3d(R*sin(phi2*2*PI/360), R*cos(phi2*2*PI/360),-length/2);
            glVertex3d(R*sin(phi2*2*PI/360), R*cos(phi2*2*PI/360),length/2);
            glVertex3d(R*x*sin((360-angle_cut_off)*2*PI/360), R*x*cos((360-angle_cut_off)*2*PI/360), length/2);
            glVertex3d(R*x*sin((360-angle_cut_off)*2*PI/360), R*x*cos((360-angle_cut_off)*2*PI/360), -length/2);
            glEnd();
            }

            if(inner_face == true){
            //inner:
            glBegin(GL_QUADS);
            glVertex3d(iR*sin(phi2*2*PI/360), iR*cos(phi2*2*PI/360),-length/2);
            glVertex3d(iR*sin(phi2*2*PI/360), iR*cos(phi2*2*PI/360),length/2);
            glVertex3d(iR*x*sin((360-angle_cut_off)*2*PI/360), iR*x*cos((360-angle_cut_off)*2*PI/360), length/2);
            glVertex3d(iR*x*sin((360-angle_cut_off)*2*PI/360), iR*x*cos((360-angle_cut_off)*2*PI/360), -length/2);
            glEnd();
            }
            break;
        }else{
            if(outer_face == true){
            //outer
            glBegin(GL_QUADS);
            glVertex3d(R*sin(phi2*2*PI/360), R*cos(phi2*2*PI/360),-length/2);
            glVertex3d(R*sin(phi2*2*PI/360), R*cos(phi2*2*PI/360),length/2);
            glVertex3d(R*sin(phi*2*PI/360), R*cos(phi*2*PI/360), length/2);
            glVertex3d(R*sin(phi*2*PI/360), R*cos(phi*2*PI/360), -length/2);
            glEnd();
            }

            if(inner_face == true){
            //inner
            glBegin(GL_QUADS);
            glVertex3d(iR*sin(phi2*2*PI/360), iR*cos(phi2*2*PI/360),-length/2);
            glVertex3d(iR*sin(phi2*2*PI/360), iR*cos(phi2*2*PI/360),length/2);
            glVertex3d(iR*sin(phi*2*PI/360), iR*cos(phi*2*PI/360), length/2);
            glVertex3d(iR*sin(phi*2*PI/360), iR*cos(phi*2*PI/360), -length/2);
            glEnd();
            }
        }
    }
    //glEnd();
glPopMatrix();

}




/*
 * Fill matrixes with current world coordinates
 * !!! Must be called just before ced_do_draw_event() !!!
 */
void ced_prepare_objmap(void){
  glGetIntegerv(GL_VIEWPORT,viewport);
  glGetDoublev(GL_MODELVIEW_MATRIX,modelM);
  glGetDoublev(GL_PROJECTION_MATRIX,projM);
  
  omap_count=0;
}

/*
 * If return zero, will set World coordinates.
 *
 * !!! There is some danger that objects will change between
 *     last drawing and this function call...
 */
int ced_get_selected(int x,int y,GLfloat *wx,GLfloat *wy,GLfloat *wz){
  CED_ObjMap *p,*best;
  unsigned i;
  int dx,dy;
  int d,dist=0; // calculate dist as |x-x'|+|y-y'|
  
  y=viewport[3]-y-1; // to get correct direction
  for(i=0,p=omap,best=0;i<omap_count;i++,p++){
    //    printf("%d %d -- %d %d\n",x,y,p->x,p->y);
    dx=abs(p->x-x);
    dy=abs(p->y-y);
    if((dx>p->max_dxy) || (dy>p->max_dxy))
      continue;
    d=dx+dy;
    if(!best || (d<dist)){
      best=p;
      dist=d;
      //      printf("%f %f %f\n",p->p.x,p->p.y,p->p.z);
    }
  }
  if(!best)
    return 1;
  *wx=best->p.x;
  *wy=best->p.y;
  *wz=best->p.z;
  printf("Will center in: %.1f %.1f %.1f for HIT %d\n",*wx,*wy,*wz,best->ID);


  SELECTED_ID = best->ID;
  return 0;
}

/**
 * Enables to print string as 2D bitmaps in OpenGL 
 * @author: SD
 * @date: 02.09.09
 * */
static void renderBitmapString(
		float x, 
		float y, 
		void *font, 
		char* string) {
  char *c;
  glRasterPos2f(x,y);
  for (c=string; *c != '\0'; c++) {
    glutBitmapCharacter(font, *c);
  }
}


/*************************************************************** 
* hauke hoelbe 08.02.2010                                      *
* A extra picking function, do the same as ced_get_selected,   *
* without center the selected object                           *
***************************************************************/
int ced_picking(int x,int y,GLfloat *wx,GLfloat *wy,GLfloat *wz){
  mouse_x=x;
  mouse_y=y;

  CED_ObjMap *p,*best;
  unsigned i;
  int dx,dy;
  int d,dist=0; // calculate dist as |x-x'|+|y-y'|

  y=viewport[3]-y-1; // to get correct direction
  for(i=0,p=omap,best=0;i<omap_count;i++,p++){
    //    printf("%d %d -- %d %d\n",x,y,p->x,p->y);
    dx=abs(p->x-x);
    dy=abs(p->y-y);
    if((dx>p->max_dxy) || (dy>p->max_dxy))
      continue;
    d=dx+dy;
    if(!best || (d<dist)){
      best=p;
      dist=d;
    }
  }
  if(!best){
    SELECTED_ID =0; //hauke
    return 1;
  }
  printf("Picking: HIT %d\n",best->ID);

  SELECTED_ID = best->ID;
  //printf("select x = %d\n",best->x);
  //SELECTED_X  = best->x;
  //SELECTED_Y =  viewport[3]-best->y-1;

//test 
/*
  CED_Point point = best->p;
 // float center1[] = {point.x, point.y, point.z};
 // float center2[] = {0,0,0};

 // float size1[]={50.0,50.0,50.0};
 // float size2[]={5000.0,5000.0,5000.0};

//  ced_geobox(size1,  center1,  0xff00ff);
//  ced_geobox(size2,  center1,  0xff00ff);
//  ced_geobox(size1,  center2,  0xff00ff);
//  ced_hit(0, 0, 0, 1, 3, 0xffff00);


   CED_Point fisheye_point0;
   fisheye_point0 = fisheye_transform(point.x,point.y, point.z, fisheye_alpha);

  glLineWidth(2.);
  glBegin(GL_LINES);
  //glVertex3f(point.x-100,point.y-100,point.z-100);
  //glVertex3f(point.x+100,point.y-100, point.z-100);
  //glVertex3f(point.x+100,point.y+100, point.z-100);
  //glVertex3f(point.x+100,point.y+100, point.z+100);
glVertex3f(point.x/10,point.y/10, point.z/10);
glVertex3f(555555,55555,55555);




  glEnd();

//  glBegin(GL_LINES);
//  glVertex2i(0,0);
//  glVertex2i(50000,50000);
//  glEnd(); 
//glFlush(); 
glutSwapBuffers();
  //glutPostRedisplay();

  printf("selected cords: %f %f %f\n", point.x, point.y, point.z);

*/
/*
    void* font=GLUT_BITMAP_TIMES_ROMAN_10;
    char foo[100]; 
    sprintf(foo,"Picking Hit: %i", SELECTED_ID);
    
	glColor3f(1.0,1.0,1.0);
	renderBitmapString(800,-800, font, foo);

	glEnd();
glFlush();
glutSwapBuffers();

*/

//test end
  
  return 0;
}



inline int ced_selected() {
    return SELECTED_ID;
}


/*
 * To be called from drawing functions
 */
static void ced_add_objmap(CED_Point *p,int max_dxy, unsigned int ID){
  GLdouble winx,winy,winz;

/*
  for(i=0;i<omap_count;i++){
     if(omap[i].ID == ID){
        printf("ID %u is already in omap!!! pos = %i\n", ID, i);
        return;
     } 
  }

*/
  if(omap_count==omap_alloced){
    omap_alloced+=256;
    //omap_alloced+=10000;
    omap=(CED_ObjMap*) realloc(omap,omap_alloced*sizeof(CED_ObjMap));
  }
  if(gluProject((GLdouble)p->x,(GLdouble)p->y,(GLdouble)p->z, modelM,projM,viewport,&winx,&winy,&winz)!=GL_TRUE){
    return;
  }
  omap[omap_count].ID=ID;
  omap[omap_count].x=winx;
  omap[omap_count].y=winy;
  omap[omap_count].max_dxy=max_dxy;
  omap[omap_count++].p=*p;
}


/*************************************************************************/

/*
 * Helper function
 */

void ced_color(unsigned rgba){
  glColor4ub((rgba>>16)&0xff,(rgba>>8)&0xff,(rgba)&0xff,
	     0xff-((rgba>>24)&0xff));
}


/*
 * Hit element
 */

static unsigned HIT_ID=0;

static void ced_draw_hit(CED_Hit *h){
    GLfloat d;
    CED_Point p_new = fisheye_transform(h->p.x, h->p.y, h->p.z, fisheye_alpha);
    float x = p_new.x;
    float y = p_new.y;
    float z = p_new.z;


    if(!IS_VISIBLE(h->type))
      return;

    //    printf("Draw hit at : %f %f %f type = %d and ced_visible_layers = %d \n",h->p.x,h->p.y,h->p.z,h->type,ced_visible_layers);

    ced_color(h->color);

    switch((h->type&0xf)){
	case CED_HIT_CROSS:
	case CED_HIT_STAR:
	    glLineWidth(1.);
	    glBegin(GL_LINES);
	    if((h->type & CED_HIT_CROSS)==CED_HIT_CROSS){
	      //	      printf("cross type == %d \n",(h->type & CED_HIT_CROSS));
	      d=h->size/2;

	      glVertex3f(x-d,y-d,z+d);
	      glVertex3f(x+d,y+d,z-d);

	      glVertex3f(x+d,y-d,z+d);
	      glVertex3f(x-d,y+d,z-d);

	      glVertex3f(x+d,y+d,z+d);
	      glVertex3f(x-d,y-d,z-d);

	      glVertex3f(x-d,y+d,z+d);
	      glVertex3f(x+d,y-d,z-d);

	    } else {
	      //	      printf("star type == %d \n",(h->type & CED_HIT_STAR));
	      d=h->size/2.;
	      glVertex3f(x-d,y,z);
	      glVertex3f(x+d,y,z);
	      glVertex3f(x,y-d,z);
	      glVertex3f(x,y+d,z);
	      glVertex3f(x,y,z-d);
	      glVertex3f(x,y,z+d); 
	    }
	    break;
	default:
	    glPointSize((GLfloat)h->size);
	    glBegin(GL_POINTS);
	    glVertex3fv(&p_new.x);
    }
    glEnd();
    ced_add_objmap(&h->p,5,h->lcioID);
}

/*
 * Line element
 */

static unsigned LINE_ID=0;

static void ced_draw_line(CED_Line *h){

  //  printf("Draw line\n");

/*
  static int anz;
  cout << "draw line " << anz++ << endl;
*/

  if(!IS_VISIBLE(h->type))
    return;
   	

/*
    float pos[3];
    float length=(int)( pow(pow(h->p1.x - h->p0.x,2) + pow(h->p1.y - h->p0.y,2) + pow(h->p1.z - h->p0.z,2),0.5) ) ;
    
    int i=0;
    ced_add_objmap(&h->p0,5,h->lcioID);
    ced_add_objmap(&h->p1,5,h->lcioID);
    
    

    pos[0] = h->p0.x;
    pos[1] = h->p0.y; 
    pos[2] = h->p0.z; 
    while(pos[0] < h->p1.x && pos[1] < h->p1.y && pos[2] < h->p1.z){
        pos[0]+=(h->p1.x - h->p0.x)/length*1.10; 
        pos[1]+=(h->p1.y - h->p0.y)/length*1.10;
        pos[2]+=(h->p1.z - h->p0.z)/length*1.10;

        //printf("start: (%f, %f, %f)\n", h->p0.x, h->p0.y, h->p0.z);
        //printf("pos: (%f, %f, %f) + (%f, %f, %f)\n", pos[0], pos[1], pos[2], (h->p1.x - h->p0.x)/length*300.0, (h->p1.y - h->p0.y)/length*300.0, (h->p1.z - h->p0.z)/length*300.0);
        //printf("end: (%f, %f, %f)\n", h->p1.x, h->p1.y, h->p1.z);


        i++;
//        printf("==> length: %f,  seperated line in %i parts\n",length,  i);
 //       printf("test: abs(h->p1.x - h->p0.x)/length*300 = %f = %f / %f\n", (h->p1.x - h->p0.x)/length*300, h->p1.x - h->p0.x, length);

        CED_Hit *h;
        h->p.x = pos[0];
        h->p.y=pos[1];
        h->p.z=pos[2];
        h->size=2;
        h->type = CED_HIT_STAR;
        //ced_draw_hit(h);

        ced_add_objmap(&pos,5,h->lcioID);

    }

        //printf("start: (%f, %f, %f)\n", h->p0.x, h->p0.y, h->p0.z);
        //printf("pos: (%f, %f, %f) + (%f, %f, %f)\n", pos[0], pos[1], pos[2], (h->p1.x - h->p0.x)/length*300.0, (h->p1.y - h->p0.y)/length*300.0, (h->p1.z - h->p0.z)/length*300.0);
        //printf("end: (%f, %f, %f)\n", h->p1.x, h->p1.y, h->p1.z);

*/


    CED_Point fisheye_point0;
    CED_Point fisheye_point1;
    fisheye_point0 = fisheye_transform(h->p0.x, h->p0.y, h->p0.z, fisheye_alpha);
    fisheye_point1 = fisheye_transform(h->p1.x, h->p1.y, h->p1.z, fisheye_alpha);    	

   	glEnable(GL_BLEND);


  	ced_color(h->color);

//--- new
 GLUquadricObj *Sphere;
  Sphere = gluNewQuadric();
  gluQuadricNormals(Sphere, GLU_SMOOTH);
  gluQuadricTexture(Sphere, GL_TRUE);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glMatrixMode(GL_MODELVIEW);
  //TODO
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glColor4f((h->color>>16)&0xff,(h->color>>8)&0xff,(h->color)&0xff, 0.1);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);


//-- end new



  	glLineWidth(h->width);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  	glBegin(GL_LINES);
  	//glVertex3fv(&h->p0.x);
  	//glVertex3fv(&h->p1.x);
    glVertex3fv(&fisheye_point0.x); 
    glVertex3fv(&fisheye_point1.x); 

  	//glDisable(GL_BLEND);
  	glEnd();
    ced_add_objmap(&h->p0,5,h->lcioID);

}



static unsigned CED_PICKING_TEXT_ID=0;
static void ced_write_picking_text(CED_PICKING_TEXT *text){
//TODO TODO TODO!!!
    static int biggest_number_picking_text=0;
    if(text->id > biggest_number_picking_text){

     //based on: http://nehe.gamedev.net/data/articles/article.asp?article=13
GLfloat winX, winY, winZ;

winX=mouse_x;
winY=mouse_y;

     float x=winX;
     float y=winY;
            GLint viewport[4];
    GLdouble modelview[16];
    GLdouble projection[16];
    //GLfloat winX, winY, winZ;
    GLdouble posX, posY, posZ;

    glGetDoublev( GL_MODELVIEW_MATRIX, modelview );
    glGetDoublev( GL_PROJECTION_MATRIX, projection );
    glGetIntegerv( GL_VIEWPORT, viewport );

    winX = (float)x;
    winY = (float)viewport[3] - (float)y;
    glReadPixels( x, int(winY), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ );

    gluUnProject( winX, winY, winZ, modelview, projection, viewport, &posX, &posY, &posZ);
    std::cout << "x: " << posX << "Y: " << posY << "Z: " << posZ << std::endl;


        glMatrixMode(GL_MODELVIEW);

    glLineWidth(5);
    glColor3f(1,0,0);

   glBegin(GL_LINE);
   glVertex3d(winX,winY,winZ);
   glVertex3d(0,0,0);
   glEnd();

   glBegin(GL_LINE);
   glVertex3d(0,0,0);
   glVertex3d(10000,10000,10000);
   glEnd();
   glutPostRedisplay();


        std::cout << text->text << std::endl;
        
        biggest_number_picking_text = text->id;
        //std::cout << mm.mv.x << mm.mv.y << mm.mv.z << std::endl;

    }

}




static unsigned GEOT_ID=0;

static void ced_draw_geotube(CED_GeoTube *c){
    
    glPushMatrix();

    double transformed_shift = single_fisheye_transform(c->shift, fisheye_alpha);

    //SM-H: Fisheye code
    double d_o = single_fisheye_transform(c->r_o, fisheye_alpha);
    double d_i = single_fisheye_transform(c->r_i, fisheye_alpha);

    double z0 = transformed_shift;
    double z1 = single_fisheye_transform(c->z+c->shift, fisheye_alpha);
    double z = z1-z0;
    if(graphic[1] == 1){
    
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //default
    //glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR); //glass
    //glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA); //locks nice, but lines diapear, so switch it off after drawing

        glMatrixMode(GL_MODELVIEW);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        if(cut_angle < 360){
            glTranslatef(0.0, 0.0, transformed_shift);
            if(c->rotate_o > 0.01 ) glRotatef(c->rotate_o, 0, 0, 1);
            if(c->rotate_o <= cut_angle){ //dont cut if rotate angle is to big
                if(c->edges_o != c->edges_i || c->rotate_i != 0){
                    //fg: here we can change the transparancy => also below !!!!!!!
                    glColor4f((c->color>>16)&0xff,(c->color>>8)&0xff,(c->color)&0xff, 0.8);
                    glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);

                    //draw the inner shape
                    glRotatef(c->rotate_i, 0, 0, 1);
                    glPolygonOffset( 1.f, 1.f ); 
                    glEnable( GL_POLYGON_OFFSET_FILL );
                    glPolygonOffset( 2.f, 2.f );
                    drawPartialCylinder(z*2, d_o-(d_o-d_i)/5, d_i, c->edges_i, cut_angle - c->rotate_i- c->rotate_o, c->rotate_i + c->rotate_o,0,1); //draw the inner cylinder 
                    //draw the outer shape
                    glRotatef(-1*c->rotate_i, 0, 0, 1);
                    glPolygonOffset( 1.f, 1.f );
                    drawPartialCylinder(z*2, d_o, d_i+(d_o-d_i)/5, c->edges_o, cut_angle - c->rotate_o, c->rotate_o,1,0); //draw the outer cylinder


                   //lines --------------
                    //glLineWidth(0.5);
                    glLineWidth(1);

                    glColor4f(0.5,0.5,0.5, 0.4);

                    glRotatef(c->rotate_i, 0, 0, 1);
                    //draw the inner cylinder 
                    drawPartialLineCylinder(z*2, d_o-(d_o-d_i)/5, d_i, c->edges_i, cut_angle - c->rotate_i- c->rotate_o, c->rotate_i + c->rotate_o,0,1); 
                    glRotatef(-1*c->rotate_i, 0, 0, 1);
                    //draw the outer cylinder
                    drawPartialLineCylinder(z*2, d_o, d_i+(d_o-d_i)/5, c->edges_o, cut_angle - c->rotate_o, c->rotate_o,1,0);
                    //---------------
                }else{

                    //glColor4f(((c->color>>16)&0xff)*0.02,((c->color>>8)&0xff)*0.02,((c->color)&0xff)*0.02, 0.4);
                    //glLineWidth(5);

                    glColor4f((c->color>>16)&0xff,(c->color>>8)&0xff,(c->color)&0xff, 0.8);
                    glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);

                    drawPartialCylinder(z*2, d_o, d_i, c->edges_o, cut_angle - c->rotate_o, c->rotate_o);

                    //glLineWidth(0.5);
                    glLineWidth(1);

                    glColor4f(0.5,0.5,0.5, 0.4);
                    //glColor4f(1,1,1, 0.0);

                    drawPartialLineCylinder(z*2, d_o, d_i, c->edges_o, cut_angle - c->rotate_o, c->rotate_o);
                }

            }else{
                if(c->edges_o != c->edges_i || c->rotate_i != 0){
                    //fg: here we can change the transparancy => also below !!!!!!!
                    glColor4f((c->color>>16)&0xff,(c->color>>8)&0xff,(c->color)&0xff, 0.8);
                    glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);

                    //draw the inner shape
                    glRotatef(c->rotate_i, 0, 0, 1);
                    glPolygonOffset( 1.f, 1.f ); 
                    glEnable( GL_POLYGON_OFFSET_FILL );
                    glPolygonOffset( 2.f, 2.f );
                    drawPartialCylinder(z*2, d_o-(d_o-d_i)/5, d_i, c->edges_i, 0,0 ,0,1); //draw the inner cylinder 
                    //draw the outer shape
                    glRotatef(-1*c->rotate_i, 0, 0, 1);
                    glPolygonOffset( 1.f, 1.f );
                    drawPartialCylinder(z*2, d_o, d_i+(d_o-d_i)/5, c->edges_o, 0,0,1,0); //draw the outer cylinder


                   //lines --------------
                    //glLineWidth(0.5);
                    glLineWidth(1);

                    glColor4f(0.5,0.5,0.5, 0.4);

                    glRotatef(c->rotate_i, 0, 0, 1);
                    //draw the inner cylinder 
                    drawPartialLineCylinder(z*2, d_o-(d_o-d_i)/5, d_i, c->edges_i, 0,0,0,1); 
                    glRotatef(-1*c->rotate_i, 0, 0, 1);
                    //draw the outer cylinder
                    drawPartialLineCylinder(z*2, d_o, d_i+(d_o-d_i)/5, c->edges_o, 0,0,1,0);
                    //---------------
                }else{

                    //glColor4f(((c->color>>16)&0xff)*0.02,((c->color>>8)&0xff)*0.02,((c->color)&0xff)*0.02, 0.4);
                    //glLineWidth(5);

                    glColor4f((c->color>>16)&0xff,(c->color>>8)&0xff,(c->color)&0xff, 0.8);
                    glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);

                    drawPartialCylinder(z*2, d_o, d_i, c->edges_o, 0,0);

                    //glLineWidth(0.5);
                    glLineWidth(1);

                    glColor4f(0.5,0.5,0.5, 0.4);
                    //glColor4f(1,1,1, 0.0);

                    drawPartialLineCylinder(z*2, d_o, d_i, c->edges_o, 0,0);
                }
            }

        }
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //default

    }else{ 
        glLineWidth(1.);
        GLUquadricObj *q1 = gluNewQuadric();
        ced_color(c->color);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);    
      
        glTranslatef(0.0, 0.0, transformed_shift);
        if(c->rotate_o > 0.01 ) glRotatef(c->rotate_o, 0, 0, 1);
        gluQuadricNormals(q1, GL_SMOOTH);
        gluQuadricTexture(q1, GL_TRUE);

        gluCylinder(q1, d_o, d_o, z*2, c->edges_o, 1);
        if(d_i > 0){gluCylinder(q1, d_i, d_i, z*2, c->edges_i, 1); }


        gluDeleteQuadric(q1);


    }



    glPopMatrix();
}

/*
 * GeoCylinder
 */

static unsigned GEOC_ID=0;

static void ced_draw_geocylinder(CED_GeoCylinder *c){


  GLUquadricObj *q1 = gluNewQuadric();

  glPushMatrix();
  glLineWidth(1.);
  ced_color(c->color);
  
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);    

  double transformed_shift = single_fisheye_transform(c->shift, fisheye_alpha);
  glTranslatef(0.0, 0.0, transformed_shift);
//  if(c->rotate > 0.01 )
//    glRotatef(c->rotate, 0, 0, 1);
  gluQuadricNormals(q1, GL_SMOOTH);
  gluQuadricTexture(q1, GL_TRUE);
  //SM-H: Fisheye code
  double d = single_fisheye_transform(c->d, fisheye_alpha);

  double z0 = transformed_shift;
  double z1 = single_fisheye_transform(c->z+c->shift, fisheye_alpha);
  double z = z1-z0;


/*
//new
  if(graphic[1] == 1){
//  if(c->rotate > 0.01 )  //not working...
//    glRotatef(c->rotate, 0, 0, 1);

 //     printf("transparent!\n");
    
      //gluQuadricNormals(q1, GLU_SMOOTH);
      //gluQuadricTexture(q1, GL_TRUE);

      //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glMatrixMode(GL_MODELVIEW);
      //glEnable(GL_BLEND);
      //glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
      //glDisable(GL_DEPTH_TEST);
      glColor4f((c->color>>16)&0xff,(c->color>>8)&0xff,(c->color)&0xff, 0.5);
//const GLfloat color[]={(c->color>>16)&0xff,(c->color>>8)&0xff,(c->color)&0xff, 0.2};
      //glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color);

      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    


     //glTranslatef(0.0, 0.0, transformed_shift);

      if(cut_angle < 360){
            drawPartialCylinder(z*2, d, d-100, c->sides, cut_angle,30);
      }


      //drawPartialCylinder(z*2,d,d/5,c->sides,90);

      //std::cout << "drawPartialCylinder: offset: " << transformed_shift  << "outer radius: " << d << " inner radius: " << d-(d/20) << " (sides: " << c->sides << " length: " << z*2 << "  )" << std::endl;

      //gluCylinder(q1, d, d, z*2, c->sides, 1);
  }else{  */
    if(c->rotate > 0.01 )  //???
        glRotatef(c->rotate, 0, 0, 1);

    gluCylinder(q1, d, d, z*2, c->sides, 1);

  //}
  gluDeleteQuadric(q1);

  glPopMatrix();
}

/*
 * GeoCylinder
 */
static unsigned GEOCR_ID=0;
static void ced_draw_geocylinder_r(CED_GeoCylinderR *c){

//FIXME: implement fisheye here as well
//Non trivial due to possible rotations...
  GLUquadricObj *q1 = gluNewQuadric();

    if(!IS_VISIBLE(c->layer))
      return;

  glLineWidth(1.);
  ced_color(c->color);
  
  glPushMatrix();

  glTranslated(c->center[0],c->center[1],c->center[2]);
  
  glRotated(c->rotate[2], 0.0, 0.0, 1.0);
  glRotated(c->rotate[1], 0.0, 1.0, 0.0);
  glRotated(c->rotate[0], 1.0, 0.0, 0.0);
  
  // center!
  glTranslated(0.0,0.0,-(c->z)/2);

	glEnable(GL_BLEND);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);  
      
  gluQuadricNormals(q1, GL_SMOOTH);
  gluQuadricTexture(q1, GL_TRUE);
  gluCylinder(q1, c->d, c->d, c->z, c->sides, 1);
  //gluCylinder(q1, c->d, c->d, c->z, 1000, 1000);

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  gluDeleteQuadric(q1);

	//glDisable(GL_BLEND);
  glEnd();
	
  glPopMatrix();
  
}
  
/** Draws an ellipsoid 
 * Code based on http://www.opengl.org/discussion_boards/ubbthreads.php?ubb=showflat&Number=242991
 * */
static unsigned ELLIPSOID_ID = 0;

static void ced_draw_ellipsoid_r(CED_EllipsoidR * eli )  {


	if(!IS_VISIBLE(eli->layer))
      return;

	/** ellipsoid 'resolution' */
	int slices = 10;
	int stacks = 10;

  	glPushMatrix();

	/** Ellipsoid centre */
  	glTranslated(eli->center[0],eli->center[1],eli->center[2]);
  	
  	/** Rotate the ellipsoid */
	glRotated(eli->rotate[2], 0.0, 0.0, 1.0);
  	glRotated(eli->rotate[1], 0.0, 1.0, 0.0);
  	glRotated(eli->rotate[0], 1.0, 0.0, 0.0);
  	
   /** Quadric object */
    //TODO
   	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  	GLUquadricObj *Sphere;
  	
  	/** Obtain a new quadric */
	Sphere = gluNewQuadric();
  	gluQuadricNormals(Sphere, GLU_SMOOTH);
  	gluQuadricTexture(Sphere, GL_TRUE);

    /** Set polygon's filling */
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	/** Set the colour */
	ced_color(eli->color);

	/** Draw the cone */
	glEnable(GL_BLEND);

	 /** Alter scale factors so as to obtain an ellipsoid from a sphere */
	glScaled(eli->size[0]/2, eli->size[1]/2, eli->size[2]/2);

  	/**Draw the sphere */
	gluSphere(Sphere, 1.0, slices, stacks);

    //glDisable(GL_BLEND); //hauke test
    glPopMatrix();
  	glEndList();	
}

static unsigned CLUELLIPSE_ID = 0;

/** 
 * Draws 3 orthogonal elipses as wireframes
 */
static void ced_draw_cluellipse_r(CED_CluEllipseR * eli )  {


	if(!IS_VISIBLE(eli->layer))
      return;

  	glPushMatrix();
  	
	/** 1. Ellipsoid centre */
  	glTranslated(eli->center[0],eli->center[1],eli->center[2]);
  	
  	/** 1. Rotate the ellipsoid */
	glRotated(eli->rotate[2], 0.0, 0.0, 1.0);
  	glRotated(eli->rotate[1], 0.0, 1.0, 0.0);
	glRotated(eli->rotate[0], 1.0, 0.0, 0.0);
  	
	/** 1. Set the colour */
	ced_color(eli->color);
	
	/** 1. Case: filled */
	float x,y,z;
	/** ellipsoid 'resolution' */
	float n = 20;
	float t;
	
	/** openGL alpha blending */
	glEnable(GL_BLEND);
    //TODO
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	
	glBegin(GL_POLYGON);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	for(t = 0; t < 2*M_PI; t +=2*M_PI/(n) ){
		x = eli->radius/2*cos(t);
		y = eli->radius/2*sin(t);
		z = 0;
		glVertex3f(x, y, z);
	}		
	glEnd();
	
	/** 2. Case: unfilled */
	
	//glColor4f(eli->RGBAcolor[0], eli->RGBAcolor[1], eli->RGBAcolor[2], 1.0);
	
	glLineWidth(2.);
	glBegin(GL_LINE_LOOP);
	for(t = 0; t < 2*M_PI; t +=2*M_PI/(n) ){
		x = eli->radius/2*cos(t);
		y = eli->radius/2*sin(t);
		z = 0;
		glVertex3f(x, y, z);
	}		
	glEnd();
	
	/**
	 * 
	 * 
	 * 
	 */
	 
	glRotated(90.0, 0.0, 1.0, 0.0);
	
	/** 1. Set the colour */
	//glColor4f(eli->RGBAcolor[0], eli->RGBAcolor[1], eli->RGBAcolor[2], eli->RGBAcolor[3]);
	
	glBegin(GL_POLYGON);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	for(t = 0; t < 2*M_PI; t +=2*M_PI/(n) ){
		x = eli->height/2*cos(t);
		y = eli->radius/2*sin(t);
		z = 0;
		glVertex3f(x, y, z);
	}		
	glEnd();
	
	/** 2. Case: unfilled */
	
	//glColor4f(eli->RGBAcolor[0], eli->RGBAcolor[1], eli->RGBAcolor[2], 1.0);
	
	glLineWidth(2.);
	glBegin(GL_LINE_LOOP);
	for(t = 0; t < 2*M_PI; t +=2*M_PI/(n) ){
		x = eli->height/2*cos(t);
		y = eli->radius/2*sin(t);
		z = 0;
		glVertex3f(x, y, z);
	}		
	glEnd();
	
	
	/**
	 * 
	 * 
	 * 
	 * 
	 * 
	 */
	 
 	glRotated(90.0, 1.0, 0.0, 0.0);
	
	/** 1. Set the colour */
	//glColor4f(eli->RGBAcolor[0], eli->RGBAcolor[1], eli->RGBAcolor[2], eli->RGBAcolor[3]);
	
	glBegin(GL_POLYGON);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	for(t = 0; t < 2*M_PI; t +=2*M_PI/(n) ){
		x = eli->height/2*cos(t);
		y = eli->radius/2*sin(t);
		z = 0;
		glVertex3f(x, y, z);
	}		
	glEnd();
	
	/** 2. Case: unfilled */
	
	//glColor4f(eli->RGBAcolor[0], eli->RGBAcolor[1], eli->RGBAcolor[2], 1.0);
	
	glLineWidth(2.);
	glBegin(GL_LINE_LOOP);
	for(t = 0; t < 2*M_PI; t +=2*M_PI/(n) ){
		x = eli->height/2*cos(t);
		y = eli->radius/2*sin(t);
		z = 0;
		glVertex3f(x, y, z);
	}		
	glEnd();
	 
	 

	/** End commands */
    //glDisable(GL_BLEND); //hauke test
    glPopMatrix();
  	glEndList();	
}





/*
//hauke
//static unsigned TEXT_ID=0;
static void ced_draw_text(CED_TEXT *text){
    //int startY=-700;
    char message[400];
	void *font=GLUT_BITMAP_TIMES_ROMAN_10; //default font


    printf("ced_draw_text: %i text: %s\n", text->id, text->text);

    //renderBitmapString(SELECTED_X*10,SELECTED_Y*10,font,text->text);
    glLoadIdentity();
    int i,j;
    int k=0;
    for(i=0, j=0;i<strlen(text->text);i++){
        if(text->text[i] == '\n' || text->text[i] == 0){
            //printf("found newline\n");
            strncpy(message,text->text+k,i-k);
            message[i-k]=0;
            k=i+1;

            renderBitmapString(600,-700-70*j,font,"                     ");
            renderBitmapString(600,-700-70*j,font,message);
            j++;
        }
    }

    glEnd();

}
*/


static unsigned TEXT_ID=0;
static void print_layer_text(CED_TEXT *obj){
    addLayerDescriptionToMenu(obj->id, obj->text);

    /*
    printf("%s: %i\n", obj->text, obj->id);
    //this
    if(obj->id == -1){
        printf("Print picking\n");
        fflush(stdout);
    }
    */
}

//end hauke

static unsigned LEGEND_ID=0;


/**
 * Draws the energy spectrum legend 
 * @author: SD
 * @date: 1.09.09
 * */
static void ced_draw_legend(CED_Legend *legend){
    //saves the matrices on the stack
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    
    //changes the matrices to be compatible with the old ced_draw_legend code:
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    GLfloat w=glutGet(GLUT_SCREEN_WIDTH); 
    GLfloat h=glutGet(GLUT_SCREEN_HEIGHT); ;

    int  WORLD_SIZE=1000; //static worldsize maybe will get problems in the future...
    glOrtho(-WORLD_SIZE*w/h,WORLD_SIZE*w/h,-WORLD_SIZE,WORLD_SIZE, -15*WORLD_SIZE,15*WORLD_SIZE);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();


    //begin original code: 
	
	int color_steps = legend->color_steps;
	float ene_max = legend->ene_max;
	float ene_min = legend->ene_min;
	unsigned int ticks = legend->ticks;
	char scale = legend->scale;
	++ticks; // incremented so that input value is only the number of 'middle ticks'
	
	/*
	 * The legend position, width and height */
	float legendThickness = 20;
	float stripeThickness = 512/(float)color_steps;
	float x_min = 1100;
	float x_max = x_min+legendThickness;
	float y_min = 400;
	float y_max = y_min+stripeThickness;
	
	int tickNumber = 1; // 'middle' tick counter
	int i;
	
	/** ticks */
	char string[6];
	int x_offset = 34;
	int y_offset = 5;
	float num;
	
	/** Legend header */
	char* header = "GeV";
	char* footer = "LOG";
	int x_offset_legend = 60;
	int y_offset_legend = 20;
	
	void* font=GLUT_BITMAP_TIMES_ROMAN_10; //default font                           //draw into back right buffer
  	int tick_size = 10;
	
	/**
	 *  Legend header: GeV */
	glColor3f(1.0,1.0,1.0);
	renderBitmapString(x_min-x_offset_legend,y_min+stripeThickness*color_steps-y_offset_legend, font, header);
	glEnd();
	//glPopMatrix();
	
	/**
	 *  Legend footer: LOG or LIN */
	switch(scale){
		case 'a': default:
			renderBitmapString(x_min-x_offset_legend,y_min-y_offset_legend, font, footer);
			glEnd();
		break;
		/** LIN */
		case 'b':
			footer = "LIN";	
			renderBitmapString(x_min-x_offset_legend,y_min-y_offset_legend, font, footer);
			glEnd();
		break;
	}
	
	for (i=0; i<color_steps; ++i) {
				
		/** This draws the colour spectrum */					
		glColor3f(legend->rgb_matrix[i][0]/(float)color_steps,legend->rgb_matrix[i][1]/(float)color_steps,legend->rgb_matrix[i][2]/(float)color_steps);
		
		glBegin(GL_POLYGON);
		glRasterPos2f(x_min, y_min);
		glVertex3f( x_min,y_min+stripeThickness*i,0.0);
		glVertex3f( x_max,y_min+stripeThickness*i,0.0);
		glVertex3f( x_max,y_max+stripeThickness*i,0.0);
		glVertex3f( x_min,y_max+stripeThickness*i,0.0);
		glEnd();
		
		/**
		 * Legend: Max & min value display */
		if (i==0 || i==(color_steps-1)){
			glBegin(GL_POLYGON);
			glColor3f(1.0, 1.0, 1.0);
			glRasterPos2f(x_min, y_min);
			glVertex3f( x_max,y_min+stripeThickness*i,0.0);
			glVertex3f( x_max+tick_size,y_min+stripeThickness*i,0.0);
			glVertex3f( x_max+tick_size,y_max+stripeThickness*i,0.0);
			glVertex3f( x_max,y_max+stripeThickness*i,0.0);
			glEnd();
			
			/**
		 	 * Spectrum max & min value display */
			glColor3f(1.0f,1.0f,1.0f);
			
			if (i==0){
				snprintf(string, 6,  "%.1f", ene_min);
				renderBitmapString(x_min+x_offset,y_min+y_offset, font, string);
			}
			else if (i==(color_steps-1)){
				//printf("top\n");
				snprintf(string, 6, "%.1f", ene_max);
				renderBitmapString(x_min+x_offset,y_min+stripeThickness*i+y_offset, font, string);
			}
		}
		
		/**
		 *  Legend: middle ticks */
		else if ((i%((color_steps-1)/ticks))==0 && tickNumber<ticks){

			//printf("middle\n");

			float pos;
			pos = (float)tickNumber*(float)color_steps/(float)ticks;

			glBegin(GL_POLYGON);
			glColor3f(1.0, 1.0, 1.0);
			glRasterPos2f(x_min, y_min);
			glVertex3f( x_max,y_min+stripeThickness*pos,0.0);
			glVertex3f( x_max+tick_size,y_min+stripeThickness*pos,0.0);
			glVertex3f( x_max+tick_size,y_max+stripeThickness*pos,0.0);
			glVertex3f( x_max,y_max+stripeThickness*pos,0.0);
			glEnd();

			/** Mid-tick legend generation: LOG */
			switch(scale){
				case 'a': default:			
					num = pow( (ene_max +1)/(ene_min +1), (float)tickNumber/(float)ticks ) * (ene_min+1) - 1;
				break;
				/** LIN */
				case 'b':
					num = (((ene_max-ene_min)/ticks)*tickNumber) + ene_min;
				break;
			}
			
			snprintf(string, 6, "%.1f", num);
			renderBitmapString(x_min+x_offset,y_min+stripeThickness*pos+y_offset, font, string);

			++tickNumber;
		}
	}
	glEnd();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

//	glPopMatrix();
}

/*
 * GeoBox
 */
static unsigned GEOB_ID = 0;

static void ced_draw_geobox(CED_GeoBox * box )  {


  // a box has 8 vertices, four belonging to the first surface facing
  // the beam, the other four from the second surface
  const unsigned int nPoint = 4;
  const unsigned int nDim   = 3;
  const unsigned int nFace  = 2;
  double face[nFace][nPoint][nDim];
  //  unsigned int iDim, iPoint, iFace;
  unsigned int i, j;


  ced_color(box->color);
  glLineWidth(2.);

  face[0][0][0] = box->center[0] + (0.5 * box->sizes[0]);
  face[0][0][1] = box->center[1] + (0.5 * box->sizes[1]);
  face[0][0][2] = box->center[2] - (0.5 * box->sizes[2]);

  face[0][1][0] = box->center[0] + (0.5 * box->sizes[0]);
  face[0][1][1] = box->center[1] - (0.5 * box->sizes[1]);
  face[0][1][2] = box->center[2] - (0.5 * box->sizes[2]);

  face[0][2][0] = box->center[0] - (0.5 * box->sizes[0]);
  face[0][2][1] = box->center[1] - (0.5 * box->sizes[1]);
  face[0][2][2] = box->center[2] - (0.5 * box->sizes[2]);

  face[0][3][0] = box->center[0] - (0.5 * box->sizes[0]);
  face[0][3][1] = box->center[1] + (0.5 * box->sizes[1]);
  face[0][3][2] = box->center[2] - (0.5 * box->sizes[2]);    

  face[1][0][0] = box->center[0] + (0.5 * box->sizes[0]);
  face[1][0][1] = box->center[1] + (0.5 * box->sizes[1]);
  face[1][0][2] = box->center[2] + (0.5 * box->sizes[2]);

  face[1][1][0] = box->center[0] + (0.5 * box->sizes[0]);
  face[1][1][1] = box->center[1] - (0.5 * box->sizes[1]);
  face[1][1][2] = box->center[2] + (0.5 * box->sizes[2]);

  face[1][2][0] = box->center[0] - (0.5 * box->sizes[0]);
  face[1][2][1] = box->center[1] - (0.5 * box->sizes[1]);
  face[1][2][2] = box->center[2] + (0.5 * box->sizes[2]);

  face[1][3][0] = box->center[0] - (0.5 * box->sizes[0]);
  face[1][3][1] = box->center[1] + (0.5 * box->sizes[1]);
  face[1][3][2] = box->center[2] + (0.5 * box->sizes[2]);




  glBegin(GL_LINES);
  // drawing the first (i=0) and second (i=1) faces
  for(i = 0; i < 2; i++){
    glVertex3f( (float) face[i][0][0], (float) face[i][0][1],  (float) face[i][0][2] );
    for(j = 1; j < 4; j++){
      glVertex3f((float) face[i][j][0],(float) face[i][j][1],(float) face[i][j][2]);
      glVertex3f((float) face[i][j][0],(float) face[i][j][1],(float) face[i][j][2]);
    }      
    glVertex3f( (float) face[i][0][0], (float) face[i][0][1],  (float) face[i][0][2] );
  }

  // drawing the connections
  for(j = 0; j < 4; j++){
    glVertex3f( (float) face[0][j][0], (float) face[0][j][1],  (float) face[0][j][2] );
    glVertex3f( (float) face[1][j][0], (float) face[1][j][1],  (float) face[1][j][2] );
  }

  glEnd();

}




/*
 * GeoBoxR 
 */
static unsigned GEOBR_ID = 0;

static void ced_draw_geobox_r(CED_GeoBoxR * box )  {
	
	if(!IS_VISIBLE(box->layer))
      return;

  // a box has 8 vertices, four belonging to the first surface facing
  // the beam, the other four from the second surface
  const unsigned int nPoint = 4;
  const unsigned int nDim   = 3;
  const unsigned int nFace  = 2;
  double face[nFace][nPoint][nDim];
  //  unsigned int iDim, iPoint, iFace;
  unsigned int i, j;


  ced_color(box->color);
  glLineWidth(2);

  glPushMatrix(); // push the matrix onto the matrix stack and pop it off (preserving the original matrix)
  //SM-H: Fisheye transform the radial vector to the centre of the box
  //FIXME: Seems to break the test_ced, but works with sample events??!!
  //Technically also incorrect, in that it only moves the centre of the box, and does not transform the whole thing
  CED_Point center_transformed = fisheye_transform(box->center[0], box->center[1], box->center[2], fisheye_alpha);
  glTranslated(center_transformed.x,center_transformed.y,center_transformed.z);
  
  glRotated(box->rotate[2], 0.0, 0.0, 1.0);
  glRotated(box->rotate[1], 0.0, 1.0, 0.0);
  glRotated(box->rotate[0], 1.0, 0.0, 0.0);
 
  //Deal with z-axis as well, this is given by box-sizes[2]
  //Need half the box size, and also the distance from the axis 
  //since this determines how much the geobox is distorted by
  double z0 = center_transformed.z;
  double z1 = single_fisheye_transform(box->center[2]+box->sizes[2], fisheye_alpha);
  double box_z = z1-z0; 
  face[0][0][0] =  + (0.5 * box->sizes[0]);
  face[0][0][1] =  + (0.5 * box->sizes[1]);
  face[0][0][2] =  - (0.5 * box_z);

  face[0][1][0] =  + (0.5 * box->sizes[0]);
  face[0][1][1] =  - (0.5 * box->sizes[1]);
  face[0][1][2] =  - (0.5 * box_z);

  face[0][2][0] =  - (0.5 * box->sizes[0]);
  face[0][2][1] =  - (0.5 * box->sizes[1]);
  face[0][2][2] =  - (0.5 * box_z);

  face[0][3][0] =  - (0.5 * box->sizes[0]);
  face[0][3][1] =  + (0.5 * box->sizes[1]);
  face[0][3][2] =  - (0.5 * box_z);

  face[1][0][0] =  + (0.5 * box->sizes[0]);
  face[1][0][1] =  + (0.5 * box->sizes[1]);
  face[1][0][2] =  + (0.5 * box_z);

  face[1][1][0] =  + (0.5 * box->sizes[0]);
  face[1][1][1] =  - (0.5 * box->sizes[1]);
  face[1][1][2] =  + (0.5 * box_z);

  face[1][2][0] =  - (0.5 * box->sizes[0]);
  face[1][2][1] =  - (0.5 * box->sizes[1]);
  face[1][2][2] =  + (0.5 * box_z);

  face[1][3][0] =  - (0.5 * box->sizes[0]);
  face[1][3][1] =  + (0.5 * box->sizes[1]);
  face[1][3][2] =  + (0.5 * box_z);

  glBegin(GL_LINES);
  // drawing the first (i=0) and second (i=1) faces
  for(i = 0; i < 2; i++){
    glVertex3f( (float) face[i][0][0], (float) face[i][0][1],  (float) face[i][0][2] );
    for(j = 1; j < 4; j++){
      glVertex3f((float) face[i][j][0],(float) face[i][j][1],(float) face[i][j][2]);
      glVertex3f((float) face[i][j][0],(float) face[i][j][1],(float) face[i][j][2]);
    }      
    glVertex3f( (float) face[i][0][0], (float) face[i][0][1],  (float) face[i][0][2] );
  }

  // drawing the connections
  for(j = 0; j < 4; j++){
    glVertex3f( (float) face[0][j][0], (float) face[0][j][1],  (float) face[0][j][2] );
    glVertex3f( (float) face[1][j][0], (float) face[1][j][1],  (float) face[1][j][2] );
  }
  glEnd();

  glPopMatrix();
}

/**
 * Draws a opaque cone with a custom alpha colour channel.
 * Warning: the cone centre is the vertex (not the centre of teh base!)
 * @author: SD
 * @date: 02.09.09
 * */
 
static unsigned CONER_ID = 0;

static void ced_draw_cone_r(CED_ConeR * cone )  {

	if(!IS_VISIBLE(cone->layer))
      return;

	/** cone size */
	float base = cone->base;
	float height = cone->height;
	/** cone 'resolution' */
	int slices = 16;
	int stacks = 1;

	glMatrixMode(GL_MODELVIEW);
  	glPushMatrix(); // push the matrix onto the matrix stack and pop it off (preserving the original matrix)

	/** Spatial translation */
  	glTranslated(cone->center[0],cone->center[1],cone->center[2]);
  	
  	/** Rotate the cone */
	glRotated(cone->rotate[2], 0.0, 0.0, 1.0);
  	glRotated(cone->rotate[1], 0.0, 1.0, 0.0);
  	glRotated(cone->rotate[0], 1.0, 0.0, 0.0);
  	
  	/** Swap the vertex with the base, so that the cone has a vertex centered at center[] */
  	glRotated(180, 1.0, 0.0, 0.0);
  	glTranslated(0.0, 0.0, -(cone->height));

	/** Draw the cone */
    //TODO
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(cone->RGBAcolor[0], cone->RGBAcolor[1], cone->RGBAcolor[2], cone->RGBAcolor[3]);
	glutSolidCone(base, height, slices, stacks);
	//glDisable(GL_BLEND); //hauke test
	
	glEnd();
	glPopMatrix();
  	
}



/*
 * GeoBoxRSolid
 */
static unsigned GEOBRS_ID = 0;

static void ced_draw_geobox_r_solid(CED_GeoBoxR * box )  {
	
	if(!IS_VISIBLE(box->layer))
		return;
	
	// a box has 8 vertices, four belonging to the first surface facing
	// the beam, the other four from the second surface
	const unsigned int nPoint = 4;
	const unsigned int nDim   = 3;
	const unsigned int nFace  = 2;
	double face[nFace][nPoint][nDim];
	//  unsigned int iDim, iPoint, iFace;
	
	
	ced_color(box->color);
	glLineWidth(2);
	
	glPushMatrix(); // push the matrix onto the matrix stack and pop it off (preserving the original matrix)
	
	glTranslated(box->center[0],box->center[1],box->center[2]);
	
	glRotated(box->rotate[2], 0.0, 0.0, 1.0);
	glRotated(box->rotate[1], 0.0, 1.0, 0.0);
	glRotated(box->rotate[0], 1.0, 0.0, 0.0);
	
	face[0][0][0] =  + (0.5 * box->sizes[0]);
	face[0][0][1] =  + (0.5 * box->sizes[1]);
	face[0][0][2] =  - (0.5 * box->sizes[2]);
	
	face[0][1][0] =  + (0.5 * box->sizes[0]);
	face[0][1][1] =  - (0.5 * box->sizes[1]);
	face[0][1][2] =  - (0.5 * box->sizes[2]);
	
	face[0][2][0] =  - (0.5 * box->sizes[0]);
	face[0][2][1] =  - (0.5 * box->sizes[1]);
	face[0][2][2] =  - (0.5 * box->sizes[2]);
	
	face[0][3][0] =  - (0.5 * box->sizes[0]);
	face[0][3][1] =  + (0.5 * box->sizes[1]);
	face[0][3][2] =  - (0.5 * box->sizes[2]);
	
	face[1][0][0] =  + (0.5 * box->sizes[0]);
	face[1][0][1] =  + (0.5 * box->sizes[1]);
	face[1][0][2] =  + (0.5 * box->sizes[2]);
	
	face[1][1][0] =  + (0.5 * box->sizes[0]);
	face[1][1][1] =  - (0.5 * box->sizes[1]);
	face[1][1][2] =  + (0.5 * box->sizes[2]);
	
	face[1][2][0] =  - (0.5 * box->sizes[0]);
	face[1][2][1] =  - (0.5 * box->sizes[1]);
	face[1][2][2] =  + (0.5 * box->sizes[2]);
	
	face[1][3][0] =  - (0.5 * box->sizes[0]);
	face[1][3][1] =  + (0.5 * box->sizes[1]);
	face[1][3][2] =  + (0.5 * box->sizes[2]);
	
	
	//  drawing the first face
	glBegin(GL_POLYGON);
	
	glVertex3f( (float) face[0][0][0], (float) face[0][0][1],  (float) face[0][0][2] );
	glVertex3f( (float) face[0][1][0], (float) face[0][1][1],  (float) face[0][1][2] );
	glVertex3f( (float) face[0][2][0], (float) face[0][2][1],  (float) face[0][2][2] );
	glVertex3f( (float) face[0][3][0], (float) face[0][3][1],  (float) face[0][3][2] );
	
	glEnd();
	
	
	
	// drawing the second face
	glBegin(GL_POLYGON);
	
	glVertex3f( (float) face[1][0][0], (float) face[1][0][1],  (float) face[1][0][2] );
	glVertex3f( (float) face[1][1][0], (float) face[1][1][1],  (float) face[1][1][2] );
	glVertex3f( (float) face[1][2][0], (float) face[1][2][1],  (float) face[1][2][2] );
	glVertex3f( (float) face[1][3][0], (float) face[1][3][1],  (float) face[1][3][2] );
	
	glEnd();
	
	
	// drawing the sides
	glBegin(GL_POLYGON);
	
	glVertex3f( (float) face[0][0][0], (float) face[0][0][1],  (float) face[0][0][2] );
	glVertex3f( (float) face[0][1][0], (float) face[0][1][1],  (float) face[0][1][2] );
	glVertex3f( (float) face[1][1][0], (float) face[1][1][1],  (float) face[1][1][2] );
	glVertex3f( (float) face[1][0][0], (float) face[1][0][1],  (float) face[1][0][2] );
	
	glEnd();
	
	glBegin(GL_POLYGON);
	
	glVertex3f( (float) face[0][1][0], (float) face[0][1][1],  (float) face[0][1][2] );
	glVertex3f( (float) face[0][2][0], (float) face[0][2][1],  (float) face[0][2][2] );
	glVertex3f( (float) face[1][2][0], (float) face[1][2][1],  (float) face[1][2][2] );
	glVertex3f( (float) face[1][1][0], (float) face[1][1][1],  (float) face[1][1][2] );
	
	glEnd();
	
	glBegin(GL_POLYGON);
	
	glVertex3f( (float) face[0][2][0], (float) face[0][2][1],  (float) face[0][2][2] );
	glVertex3f( (float) face[0][3][0], (float) face[0][3][1],  (float) face[0][3][2] );
	glVertex3f( (float) face[1][3][0], (float) face[1][3][1],  (float) face[1][3][2] );
	glVertex3f( (float) face[1][2][0], (float) face[1][2][1],  (float) face[1][2][2] );
	
	glEnd();
	
	glBegin(GL_POLYGON);
	
	glVertex3f( (float) face[0][3][0], (float) face[0][3][1],  (float) face[0][3][2] );
	glVertex3f( (float) face[0][0][0], (float) face[0][0][1],  (float) face[0][0][2] );
	glVertex3f( (float) face[1][0][0], (float) face[1][0][1],  (float) face[1][0][2] );
	glVertex3f( (float) face[1][3][0], (float) face[1][3][1],  (float) face[1][3][2] );
	
	glEnd();
	
	glPopMatrix();
	
}


void ced_register_elements(void){

  //1
  GEOC_ID       =ced_register_element(sizeof(CED_GeoCylinder),(ced_draw_cb)ced_draw_geocylinder);
  //2
  GEOCR_ID      =ced_register_element(sizeof(CED_GeoCylinderR),(ced_draw_cb)ced_draw_geocylinder_r);
  //3
  LINE_ID       =ced_register_element(sizeof(CED_Line),(ced_draw_cb)ced_draw_line);
  //4
  HIT_ID        =ced_register_element(sizeof(CED_Hit),(ced_draw_cb)ced_draw_hit);
  //5
  GEOB_ID       =ced_register_element(sizeof(CED_GeoBox),(ced_draw_cb)ced_draw_geobox);
  //6
  GEOBR_ID      =ced_register_element(sizeof(CED_GeoBoxR),(ced_draw_cb)ced_draw_geobox_r);
  //7
  GEOBRS_ID     =ced_register_element(sizeof(CED_GeoBoxR),(ced_draw_cb)ced_draw_geobox_r_solid);
  //8
  CONER_ID      =ced_register_element(sizeof(CED_ConeR),(ced_draw_cb)ced_draw_cone_r);
  //9
  ELLIPSOID_ID  =ced_register_element(sizeof(CED_EllipsoidR),(ced_draw_cb)ced_draw_ellipsoid_r);
  //10
  CLUELLIPSE_ID =ced_register_element(sizeof(CED_CluEllipseR),(ced_draw_cb)ced_draw_cluellipse_r);
  //11
  TEXT_ID       =ced_register_element(sizeof(CED_TEXT),(ced_draw_cb)print_layer_text); //hauke

  /** due to an issue w/ drawing the legend (in 2D) this has to come last ! */
  //12
  LEGEND_ID  =ced_register_element(sizeof(CED_Legend),(ced_draw_cb)ced_draw_legend);
  //TEXT_ID   =ced_register_element(sizeof(CED_TEXT),(ced_draw_cb)ced_draw_text); //hauke
  //LAYER_TEXT_ID   =ced_register_element(sizeof(LAYER_TEXT),(ced_draw_cb)print_layer_text); //hauke

  //13
  GEOT_ID       =ced_register_element(sizeof(CED_GeoTube),(ced_draw_cb)ced_draw_geotube);
  //14
  CED_PICKING_TEXT_ID=ced_register_element(sizeof(CED_PICKING_TEXT),(ced_draw_cb)ced_write_picking_text);
}


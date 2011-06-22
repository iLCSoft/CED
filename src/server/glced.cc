/* "C" event display.
 * OpendGL (GLUT) based.
 *
 * Alexey Zhelezov, DESY/ITEP, 2005 
 *
 * July 2005, Jörgen Samson: Moved parts of the TCP/IP
 *            server to glut's timer loop to make glced 
 *            "standard glut" compliant
 *
 * June 2007, F.Gaede: - added world_size command line parameter
 *                     - added help message for "-help, -h, -?"
 *                     - replaced fixed size window geometry with geometry comand-line option
 *                     
 */




#include <iomanip>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#include "GL/gl.h"
#include <GL/glu.h>
#include <GL/glut.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <math.h>

#include <ced.h>
#include <ced_cli.h>

#include <errno.h>
#include <sys/select.h>

//hauke
#include <ctype.h> //toupper
#include <sys/time.h>
#include <time.h>
#include <netdb.h>
#include <iostream>
#include <fstream>
#include <stdlib.h> //getenv
using namespace std;


    //#include<jpeglib.h>

#define DETECTOR1               4001
#define DETECTOR2               4002
#define DETECTOR3               4003
#define DETECTOR4               4004
#define DETECTOR5               4005
#define DETECTOR6               4006
#define DETECTOR7               4007
#define DETECTOR8               4008
#define DETECTOR9               4009
#define DETECTOR10              4010
#define DETECTOR11              4011
#define DETECTOR12              4012
#define DETECTOR13               4013
#define DETECTOR14               4014
#define DETECTOR15               4015
#define DETECTOR16               4016
#define DETECTOR17               4017
#define DETECTOR18               4018
#define DETECTOR19               4019
#define DETECTOR20              4020

#define DETECTOR_ALL            4100


#define GRAFIC_HIGH             2000            
#define GRAFIC_LOW              2001
#define GRAFIC_PERSP            2002
#define GRAFIC_BUFFER           2003
#define GRAFIC_TRANS            2004
#define GRAFIC_LIGHT            2005
#define GRAFIC_ALIAS            2006


#define CUT_ANGLE0              2100
#define CUT_ANGLE30             2101
#define CUT_ANGLE90             2102
#define CUT_ANGLE135            2103
#define CUT_ANGLE180            2104
#define CUT_ANGLE270            2105
#define CUT_ANGLE360            2106

#define TRANS0                  3000
#define TRANS40                 3101

#define TRANS60                 3001
#define TRANS70                 3002
#define TRANS80                 3003
#define TRANS90                 3004
#define TRANS95                 3005
#define TRANS100                3006

#define FULLSCREEN              6001
#define AXES                    6002
#define FPS                     6003



static int available_cutangles[]={0,30,90,135, 180, 270, 360};  //for new angles add the new angle to this list and to define in top of this


#define BGCOLOR_WHITE           1000
#define BGCOLOR_SILVER          1001
#define BGCOLOR_DIMGRAY         1002
#define BGCOLOR_BLACK           1003
#define BGCOLOR_LIGHTSTEELBLUE  1004
#define BGCOLOR_STEELBLUE       1005
#define BGCOLOR_BLUE            1006
#define BGCOLOR_SEAGREEN        1007
#define BGCOLOR_ORANGE          1008
#define BGCOLOR_YELLOW          1009
#define BGCOLOR_VIOLET          1010

#define BGCOLOR_GAINSBORO       1011
#define BGCOLOR_LIGHTGREY       1012
#define BGCOLOR_DARKGRAY        1013
#define BGCOLOR_GRAY            1014

#define BGCOLOR_USER            1100


#define VIEW_FISHEYE    20
#define VIEW_FRONT      21
#define VIEW_SIDE       22
#define VIEW_ZOOM_IN    23
#define VIEW_ZOOM_OUT   24
#define VIEW_RESET      25
#define VIEW_CENTER     26

#define LAYER_0         30
#define LAYER_1         31
#define LAYER_2         32
#define LAYER_3         33
#define LAYER_4         34
#define LAYER_5         35
#define LAYER_6         36
#define LAYER_7         37
#define LAYER_8         38
#define LAYER_9         39
#define LAYER_10        40
#define LAYER_11        41
#define LAYER_12        42
#define LAYER_13        43
#define LAYER_14        44
#define LAYER_15        45
#define LAYER_16        46
#define LAYER_17        47
#define LAYER_18        48
#define LAYER_19        49
#define LAYER_ALL       60

#define HELP            100
#define SAVE            101

#define TOGGLE_PHI_PROJECTION   5000
#define TOGGLE_Z_PROJECTION     5001

//#define PHI_PROJECTION_OFF 5001


extern CEDsettings setting;

//extern int graphic[];  //= {0,0,0,0}; //light, transparence, perspective, anti aliasing
//extern double cut_angle;
//extern double trans_value;
//static double z_cutting=7000;
//static bool fixed_view=0;

//extern bool phi_projection;
//extern bool z_projection;

int ced_picking(int x,int y,GLfloat *wx,GLfloat *wy,GLfloat *wz); //from ced_srv.c, need header files!

static char layerDescription[MAX_LAYER][MAX_LAYER_CHAR]; 
const char layer_keys[] = {'0','1', '2','3','4','5','6','7','8','9',')', '!', '@', '#', '$', '%', '^', '&', '*', '(', 't', 'y', 'u', 'i', 'o'};

static int mainWindow=-1;
static int subWindow=-1;
static int layerMenu;
static int detectorMenu;
static int subsubMenu2;

//static int helpWindow=-1;
static int showHelp=0;

#define DEFAULT_WORLD_SIZE 1000.  //SJA:FIXED Reduce world size to give better scale

static float WORLD_SIZE;
static float FISHEYE_WORLD_SIZE;

double fisheye_alpha = 0.0;

//hauke
long int doubleClickTime=0;
static float BG_COLOR[4];

extern int SELECTED_ID ;

//fg - make axe a global to be able to rescale the world volume
static GLfloat axe[][3]={
  { 0., 0., 0., },
  { DEFAULT_WORLD_SIZE/2, 0., 0. },
  { 0., DEFAULT_WORLD_SIZE/2, 0. },
  { 0., 0., DEFAULT_WORLD_SIZE/2 }
};

// allows to reset the visible world size
static void set_world_size( float length) {
  WORLD_SIZE = length ;
  axe[1][0] = WORLD_SIZE / 2. ;
  axe[2][1] = WORLD_SIZE / 2. ;
  axe[3][2] = WORLD_SIZE / 2. ;
};


//hauke
static void set_bg_color(float one, float two, float three, float four){
    BG_COLOR[0]=one;
    BG_COLOR[1]=two;
    BG_COLOR[2]=three;
    BG_COLOR[3]=four;
}

typedef GLfloat color_t[4];

static color_t bgColors[] = {
  { 0.0, 0.2, 0.4, 0.0 }, //light blue
  { 0.0, 0.0, 0.0, 0.0 }, //black
  { 0.2, 0.2, 0.2, 0.0 }, //gray shades
  { 0.4, 0.4, 0.4, 0.0 },
  { 0.6, 0.6, 0.6, 0.0 },
  { 0.8, 0.8, 0.8, 0.0 },
  { 1.0, 1.0, 1.0, 0.0 }  //white
};

static float userDefinedBGColor[] = {-1.0, -1.0, -1.0, -1.0};

static unsigned int iBGcolor = 0;

/* AZ I check for TCP sockets as well,
 * function will return 0 when such "event" happenes */
extern struct __glutSocketList {
  struct __glutSocketList *next;
  int fd;
  void  (*read_func)(struct __glutSocketList *sock);
} *__glutSockets;


// from ced_srv.c
void ced_prepare_objmap(void);
int ced_get_selected(int x,int y,GLfloat *wx,GLfloat *wy,GLfloat *wz);
//SJA:FIXED set this to extern as it is a global from ced_srv.c
//extern unsigned long ced_visible_layers; 
//extern bool ced_visible_layers[100];

static struct _geoCylinder {
  GLuint obj;
  GLfloat d;       // radius
  //GLfloat ir;      
  GLuint  sides;   // poligon order
  GLfloat rotate;  // angle degree
  GLfloat z;       // 1/2 length
  GLfloat shift;   // in z
  GLfloat r;       // R 
  GLfloat g;       // G  color
  GLfloat b;       // B
} geoCylinder[] = {
  { 0,   50.0,  6,  0.0, 5658.5, -5658.5, 0.0, 0.0, 1.0 }, // beam tube
  { 0,  380.0, 24,  0.0, 2658.5, -2658.5, 0.0, 0.0, 1.0 }, // inner TPC
  { 0, 1840.0,  8, 22.5, 2700.0, -2700.0, 0.5, 0.5, 0.1 }, // inner ECAL
  { 0, 3000.0, 16,  0.0, 2658.5, -2658.5, 0.0, 0.8, 0.0 }, // outer HCAL
  { 0, 2045.7,  8, 22.5, 2700.0, -2700.0, 0.5, 0.5, 0.1 }, // outer ECAL
  { 0, 3000.0,  8, 22.5, 702.25,  2826.0, 0.0, 0.8, 0.0 }, // endcap HCAL
  { 0, 2045.7,  8, 22.5, 101.00,  2820.0, 0.5, 0.5, 0.1 }, // endcap ECAL
  { 0, 3000.0,  8, 22.5, 702.25, -4230.5, 0.0, 0.8, 0.0 }, // endcap HCAL
  { 0, 2045.7,  8, 22.5, 101.00, -3022.0, 0.5, 0.5, 0.1 }, // endcap ECAL
};

static GLuint makeCylinder(struct _geoCylinder *c){
    GLUquadricObj *q1 = gluNewQuadric();
    GLuint obj;
  
    glPushMatrix();
    obj = glGenLists(1);
    glNewList(obj, GL_COMPILE);
    glTranslatef(0.0, 0.0, c->shift);
    if(c->rotate > 0.01 )
        glRotatef(c->rotate, 0, 0, 1);
    gluQuadricNormals(q1, GL_SMOOTH);
    gluQuadricTexture(q1, GL_TRUE);
    gluCylinder(q1, c->d, c->d, c->z*2, c->sides, 1);
    glEndList();
    glPopMatrix();
    gluDeleteQuadric(q1);
    return obj;
}

static void makeGeometry(void) {
    unsigned i;

    // cylinders
    for(i=0;i<sizeof(geoCylinder)/sizeof(struct _geoCylinder);i++){
        geoCylinder[i].obj=makeCylinder(geoCylinder+i);
    }
}


static void init(void){

    //Set background color
    glClearColor(BG_COLOR[0],BG_COLOR[1], BG_COLOR[2], BG_COLOR[3]);

    //glShadeModel(GL_FLAT);
    glShadeModel(GL_SMOOTH);

    glClearDepth(1);

    glEnable(GL_DEPTH_TEST); //activate 'depth-test'

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //clear buffers

    //glDepthFunc(GL_LESS);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //default


    //glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
    //glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR); //glass
    //glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA); //locks nice, but lines diapear

    //glBlendFunc(GL_ONE, GL_ZERO);
    //glBlendFunc(GL_ONE, GL_ONE);
    //glClearColor(0,0,0,0);

    glEnableClientState(GL_VERTEX_ARRAY);
    // GL_NORMAL_ARRAY GL_COLOR_ARRAY GL_TEXTURE_COORD_ARRAY,GL_EDGE_FLAG_ARRAY

    // to make round points
    //glEnable(GL_POINT_SMOOTH);

    // to put text
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);

    // To enable Alpha channel (expensive !!!)
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    makeGeometry();
}

typedef struct {
    GLfloat x;
    GLfloat y;
    GLfloat z;
} Point;

static struct {
    GLfloat va; // vertical angle
    GLfloat ha; // horisontal angle
    GLfloat sf; // scale factor
    Point mv; // the center
    GLfloat va_start;
    GLfloat ha_start;
    GLfloat sf_start;
    Point mv_start;
} mm = {
    30.,
    150.,
    0.1, //hauke decrease zoom, //SJA:FIXED set redraw scale a lot smaller
    { 0., 0., 0. },
    0.,
    0.,
    1.,
    { 0., 0., 0. },
}, mm_reset ;


static GLfloat window_width=0.;
static GLfloat window_height=0.;
static enum {
    NO_MOVE,
    TURN_XY,
    ZOOM,
    ORIGIN
} move_mode;
static GLfloat mouse_x=0.;
static GLfloat mouse_y=0.;


// bitmaps for X,Y and Z
static unsigned char x_bm[]={ 
    0xc3,0x42,0x66,0x24,0x24,0x18,
    0x18,0x24,0x24,0x66,0x42,0xc3
};
static unsigned char y_bm[]={
    0xc0,0x40,0x60,0x20,0x30,0x10,
    0x18,0x2c,0x24,0x66,0x42,0xc3
};
static unsigned char z_bm[]={ 
    0xff,0x40,0x60,0x20,0x30,0x10,
    0x08,0x0c,0x04,0x06,0x02,0xff
};
  

static void axe_arrow(void){
    GLfloat k=WORLD_SIZE/window_height;
    glutSolidCone(8.*k,30.*k,16,5);
}

static void display_world(void){
/*   static GLfloat axe[][3]={ */
/*     { 0., 0., 0., }, */
/*     { WORLD_SIZE/2, 0., 0. }, */
/*     { 0., WORLD_SIZE/2, 0. }, */
/*     { 0., 0., WORLD_SIZE/2 } */
/*   }; */
  //  unsigned i;
    if(setting.show_axes == false){
        return;
    }

    glColor3f(0.2,0.2,0.8);
    //glLineWidth(2.);
    glLineWidth(0.5);

    glBegin(GL_LINES);
    glVertex3fv(axe[0]);
    glVertex3fv(axe[1]);
    glEnd();
    glBegin(GL_LINES);
    glVertex3fv(axe[0]);
    glVertex3fv(axe[2]);
    glEnd();
    glBegin(GL_LINES);
    glVertex3fv(axe[0]);
    glVertex3fv(axe[3]);
    glEnd();
  
    glColor3f(0.5,0.5,0.8);
    glPushMatrix();
    glTranslatef(WORLD_SIZE/2.-WORLD_SIZE/100.,0.,0.);
    glRotatef(90.,0.0,1.0,0.0);
    axe_arrow();
    glPopMatrix();
  
    glPushMatrix();
    glTranslatef(0.,WORLD_SIZE/2.-WORLD_SIZE/100.,0.);
    glRotatef(-90.,1.0,0.,0.);
    axe_arrow();
    glPopMatrix();
  
  
    glPushMatrix();
    glTranslatef(0.,0.,WORLD_SIZE/2.-WORLD_SIZE/100.);
    axe_arrow();
    glPopMatrix();
  
    // Draw X,Y,Z ...
    //glColor3f(1.,1.,1.); //white labels
    //glColor3f(0.,0.,0.); //black labels

    glGetDoublev(GL_COLOR_CLEAR_VALUE, setting.bgcolor);
    double dark=1.0-(setting.bgcolor[0]+setting.bgcolor[1]+setting.bgcolor[2])/3.0;
    //glColor3f(1-setting.bgcolor[0], 1-setting.bgcolor[1], 1-setting.bgcolor[2]);
    glColor3f(dark,dark,dark); 


    glRasterPos3f(WORLD_SIZE/2.+WORLD_SIZE/8,0.,0.);
    glBitmap(8,12,4,6,0,0,x_bm);
    glRasterPos3f(0.,WORLD_SIZE/2.+WORLD_SIZE/8,0.);
    glBitmap(8,12,4,6,0,0,y_bm);
    glRasterPos3f(0.,0.,WORLD_SIZE/2.+WORLD_SIZE/8);
    glBitmap(8,12,4,6,0,0,z_bm);
  
    
    // cylinders
    /*
    glLineWidth(1.);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);    
    for(i=0;i<sizeof(geoCylinder)/sizeof(struct _geoCylinder);i++){
      glPushMatrix();
      //  glPolygonMode(GL_FRONT_AND_BACK, (i<2)?GL_FILL:GL_LINE);    
      glColor4f(geoCylinder[i].r,geoCylinder[i].g,geoCylinder[i].b,
  	      (i>=2)?1.:0.2);
      glCallList(geoCylinder[i].obj);
      glPopMatrix();
    }
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    */
    //buildMenuPopup(); //hauke: test
    //glutAttachMenu(GLUT_RIGHT_BUTTON);
  
}


void printFPS(void){
    //calculate fps:
    //----------------------
    static int fps=0;
    static int old_fps=0;
    static double startTime;
    struct timeval tv;
    gettimeofday(&tv, 0); 

    if(tv.tv_sec+tv.tv_usec/1000000.0-startTime < 1.0){
        fps++;
    }else{
        startTime=tv.tv_sec+tv.tv_usec/1000000.0;
        //printf("FPS: %i\n", fps);
        old_fps=fps;
        fps=1;
    }

    if(setting.fps == false){
        return;
    }
    //print on screen: 
    //----------------------

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

    char text[400];
    void *font=GLUT_BITMAP_TIMES_ROMAN_10; //default font

    sprintf(text, "FPS: %i", old_fps);

    glLoadIdentity();

    double dark=1.0-(setting.bgcolor[0]+setting.bgcolor[1]+setting.bgcolor[2])/3.0;
    glColor3f(dark,dark,dark);

    glRasterPos2f(-1200,-950);
    char *c;
    for (c=text; *c != '\0'; c++) {
        glutBitmapCharacter(font, *c);
    }

    glEnd();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}
static void display(void){
 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
    glPushMatrix();
  
  
    glRotatef(mm.va,1.,0.,0.);
    glRotatef(mm.ha,0.,1.0,0.);
    glScalef(mm.sf,mm.sf,mm.sf); //streech the world





    
    glTranslatef(-mm.mv.x,-mm.mv.y,-mm.mv.z);
  
      //glMatrixMode(GL_MODELVIEW); //
  
    // draw static objects
    display_world(); //only axes?
  
  
     //glTranslatef(0,0,1000);
  
     const GLdouble clip_plane[]={0,0,-1,setting.z_cutting};
     if(setting.z_cutting < 6999){
          glEnable(GL_CLIP_PLANE0);
     }else{
          glDisable(GL_CLIP_PLANE0);
     }
     glClipPlane(GL_CLIP_PLANE0,clip_plane);
  
  
  
  
    // draw elements (hits + detector)
    ced_prepare_objmap();
    ced_do_draw_event();
  
    //cout << "mm.sf: " << mm.sf << "hinterer clipping plane: " << 5000*2.0*mm.sf << std::endl;
    //gluPerspective(60,window_width/window_height,100*2.0*mm.sf,5000*2.0*mm.sf);

//    std::cout  << "clipping planes: " << 200*2.0*mm.sf << " bis " << 5000*2.0*mm.sf << std::endl;
//
//    gluPerspective(60,window_width/window_height,200*2.0*mm.sf,5000*2.0*mm.sf);
//        glMatrixMode( GL_MODELVIEW );
//  
//        glLoadIdentity();
//        gluLookAt  (0,0,2000,    0,0,0,    0,1,0);
//
//


    printFPS();
    
    glPopMatrix();
  
    glutSwapBuffers();
}

static void reshape(int w,int h){
    // printf("Reshaped: %dx%d\n",w,h);
    window_width=w;
    window_height=h;
  
  
    //if(graphic[3]){
    if(setting.antia){

        //glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        //glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
        //glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
        //glHint(GL_POLYGON_SMOOTH,GL_FASTEST);
  
        //glEnable(GL_POINT_SMOOTH);
        //glEnable(GL_LINE_SMOOTH);
        //glEnable(GL_POLYGON_SMOOTH);
        //glShadeModel(GL_SMOOTH);
  
        //glEnable(GL_BLEND);
        //glEnable (GL_BLEND);
        //glBlendFunc (GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glEnable (GL_LINE_SMOOTH);
        glEnable (GL_BLEND);
        glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glHint (GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
    }else{
        glDisable(GL_POINT_SMOOTH);
        glDisable(GL_LINE_SMOOTH);
    }
  
    //if(graphic[2] == 0){
    if(setting.persp == false){

        glViewport(0,0,w,h);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(-WORLD_SIZE*w/h,WORLD_SIZE*w/h,-WORLD_SIZE,WORLD_SIZE, -15*WORLD_SIZE,15*WORLD_SIZE);
  
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity(); 
    }else{
        glViewport(0,0,w,h);
  
        glMatrixMode( GL_PROJECTION );
        glLoadIdentity();
        //gluPerspective(60,window_width/window_height,100,500000);
        //double plane1, plane2;
        //plane1=100.0*mm.sf;
        //plane2=50000.0*mm.sf;
        //gluPerspective(60,window_width/window_height,plane1,plane2);
        //gluPerspective(60,window_width/window_height,100.0,50000.0*mm.sf+50000/mm.sf);

        gluPerspective(45,window_width/window_height,100.0,50000.0*mm.sf+50000/mm.sf);


        //std::cout  << "clipping planes: " << plane1 << " bis " << plane2<< std::endl;


        glMatrixMode( GL_MODELVIEW );
  
        glLoadIdentity();
      
        //glClearDepth(1.0);                  
        //glEnable(GL_DEPTH_TEST);            
        //glDepthFunc(GL_LEQUAL);             
        //glDepthFunc(GL_LESS);             
  
  
  
  
        //glEnable (GL_LINE_SMOOTH);
  
        //glHint (GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
  
  
        //    glShadeModel(GL_SMOOTH);
   
        //glDepthMask(GL_TRUE);
  
        // //glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA); 
        // //glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
        //glBlendFunc(GL_ONE, GL_ZERO);
        //glEnable(GL_BLEND);
      
        gluLookAt  (0,0,2000,    0,0,0,    0,1,0);
    }
  
  
    //hauke
    if(showHelp == 1){
        glutSetWindow (subWindow);
        glutReshapeWindow (int(window_width-10),int(window_height/4));
    }
}

void saveSettings(void){
    ofstream file;
    const char *home = getenv("HOME");
    char filename[1000];
    snprintf(filename, 1000, "%s/.glced", home);

    //file.open(filename, ios::out | ios::binary);
    file.open(filename);

    if(file.is_open()){ 
//        file << setting.trans << endl;
//        file << setting.persp << endl;
//        file.close();
        setting.va=mm.va;
        setting.ha=mm.ha;
        setting.win_w=(int)window_width;
        setting.win_h=(int)window_height;
        setting.zoom = mm.sf; 
        setting.fisheye_alpha=fisheye_alpha;

        setting.fisheye_world_size = FISHEYE_WORLD_SIZE; 
        setting.world_size = WORLD_SIZE;
        //double bgcolor[4];
        glGetDoublev(GL_COLOR_CLEAR_VALUE, setting.bgcolor);
        //glGetDoublev(GL_COLOR_CLEAR_VALUE, bgcolor);
        //cout << "bgcolor: " << bgcolor[0] << ", " << bgcolor[1] << ", " << bgcolor[2] << ", "  << bgcolor[3] << "\n" ;

        //file.write((char*)&setting, sizeof(setting));
        file<<"#Config version:"<<std::endl<<VERSION_CONFIG << std::endl; 
        file<<"#Transp:"<<std::endl<<setting.trans << std::endl; 
        file<<"#Persp:"<<std::endl<<setting.persp  << std::endl;
        file<<"#Anti A:"<<std::endl<<setting.antia<< std::endl;
        file<<"#Light:"<<std::endl<<setting.light<< std::endl;
        file<<"#Cut angle:"<<std::endl<<setting.cut_angle<< std::endl;
        file<<"#Trans value:"<<std::endl<<setting.trans_value<< std::endl;
        for(int i=0;i<MAX_LAYER;i++){
            file<<"#Visibility Layer " << i << ":" <<std::endl<<setting.layer[i]<< std::endl;
        }
        file<<"#Phi projection:"<<std::endl<<setting.phi_projection<< std::endl;
        file<<"#Z projection:"<<std::endl<<setting.z_projection<< std::endl;
        for(int i=0;i<3;i++){
            file<<"#View setting" << i << ":" <<std::endl<<setting.view[i] << std::endl;
        }
        file<<"#Vertical angle:"<<std::endl<<setting.va<< std::endl;
        file<<"#Horiz angle:"<<std::endl<<setting.ha<< std::endl;
        file<<"#Fixed view:"<<std::endl<<setting.fixed_view<< std::endl;
        file<<"#Z cutting:"<<std::endl<<setting.z_cutting<< std::endl;
        file<<"#Window height:"<<std::endl<<setting.win_h<< std::endl;
        file<<"#Window width:"<<std::endl<<setting.win_w<< std::endl;
        file<<"#Zoom:"<<std::endl<<setting.zoom<< std::endl;
        file<<"#Fisheye_alpha:"<<std::endl<<setting.fisheye_alpha<< std::endl;
        file<<"#World size:"<<std::endl<<setting.world_size<< std::endl;
        file<<"#fisheye world size:"<<std::endl<<setting.fisheye_world_size<< std::endl;
        for(int i=0;i<4;i++){
            file<<"#Background color, value "<< i << ":" << std::endl<<setting.bgcolor[i]<< std::endl;
        }

        file<<"#Show axes:"<<std::endl<<setting.show_axes<< std::endl;
        file<<"#Show fps:"<<std::endl<<setting.fps<< std::endl;


        std::cout << "Save settings to: " << filename << std::endl;

    }else{
        std::cout << "Error open file: " << filename << std::endl;
    }
}

void defaultSettings(void){
        setting.trans=true;
        setting.persp=true;
        setting.light=false;
        setting.antia=false;
        setting.cut_angle=180;
        setting.trans_value=0.8;
        setting.z_cutting=7000;


        setting.win_w=500;
        setting.win_h=500;
        setting.show_axes=true;
        setting.fps=false;

        setting.va=mm.va;
        setting.ha=mm.ha;
    
        
        for(int i=0; i < MAX_LAYER; i++){
            setting.layer[i]=true; // turn all layers on
        }
        for(int i=0;i < 4; i++){
            setting.bgcolor[i]=0; //black
        }

        std::cout << "Set to default settings" << std::endl;
}

void idle(void){
    glutPostRedisplay();
}


void loadSettings(void){
    ifstream file;

    const char *home = getenv("HOME");
    char filename[1000];
    snprintf(filename, 1000, "%s/.glced", home);
    file.open(filename);

    if(file.is_open()){
        string line;
//        file.read((char*)&setting, sizeof(setting));
            getline(file,line);getline(file,line);
            if(VERSION_CONFIG != atoi(line.c_str())){
                std::cout << "WARNING: Cant read configfile (" << filename << ") please delete or rename it" << std::endl; 
                defaultSettings();
            }

            getline(file,line);getline(file,line);
            setting.trans=atoi(line.c_str());

            getline(file,line);getline(file,line);
            setting.persp=atoi(line.c_str());

            getline(file,line);getline(file,line);
            setting.antia=atoi(line.c_str());

            getline(file,line);getline(file,line);
            setting.light=atoi(line.c_str());

            getline(file,line);getline(file,line);
            setting.cut_angle=atof(line.c_str());

            getline(file,line);getline(file,line);
            setting.trans_value=atof(line.c_str());

            for(int i=0;i<MAX_LAYER;i++){
                getline(file,line);getline(file,line);
                setting.layer[i]=atoi(line.c_str());
            }

            getline(file,line);getline(file,line);
            setting.phi_projection=atoi(line.c_str());

            getline(file,line);getline(file,line);
            setting.z_projection=atoi(line.c_str());

            for(int i=0;i<3;i++){
                getline(file,line);getline(file,line);
                setting.view[i]=atof(line.c_str());
            }

            getline(file,line);getline(file,line);
            setting.va=atof(line.c_str());
            getline(file,line);getline(file,line);
            setting.ha=atof(line.c_str());

            getline(file,line);getline(file,line);
            setting.fixed_view=atoi(line.c_str());
            
            getline(file,line);getline(file,line);
            setting.z_cutting=atof(line.c_str());

            getline(file,line);getline(file,line);
            setting.win_h=atoi(line.c_str());

            getline(file,line);getline(file,line);
            setting.win_w=atoi(line.c_str());
            if(setting.win_w == 0 || setting.win_h == 0){
                setting.win_w = setting.win_h = 500;
            }

            getline(file,line);getline(file,line);
            setting.zoom=atof(line.c_str());

            getline(file,line);getline(file,line);
            setting.fisheye_alpha=atof(line.c_str());

            getline(file,line);getline(file,line);
            setting.world_size=atof(line.c_str());

            getline(file,line);getline(file,line);
            setting.fisheye_world_size=atof(line.c_str());

            for(int i=0;i<4;i++){
                getline(file,line);getline(file,line);
                setting.bgcolor[i] = atof(line.c_str());
            }

            getline(file,line);getline(file,line);
            setting.show_axes=atoi(line.c_str());

            getline(file,line);getline(file,line);
            setting.fps=atoi(line.c_str());



        mm.va=setting.va;
        mm.ha=setting.ha;
        mm.sf = setting.zoom; 
        fisheye_alpha=setting.fisheye_alpha;


        FISHEYE_WORLD_SIZE = setting.fisheye_world_size; 
        WORLD_SIZE=setting.world_size;




        //reshape(setting.win_w, setting.win_h);


        std::cout << "Read settings from: " << filename << std::endl;

    }else{ //set to default
        defaultSettings();
    }

}




void mouseWheel(int button, int dir, int x, int y){ //hauke
    if(dir > 0){
        selectFromMenu(VIEW_ZOOM_IN);
    }else{
        selectFromMenu(VIEW_ZOOM_OUT);
    }
}
static void mouse(int btn,int state,int x,int y){
    //hauke
    struct timeval tv;

    struct __glutSocketList *sock;

    //#ifdef __APPLE__
    //hauke
    int mouseWheelDown=9999;
    int mouseWheelUp=9999;

    #ifdef GLUT_WHEEL_UP
        mouseWheelDown = GLUT_WHEEL_DOWN;
        mouseWheelUp = GLUT_WHEEL_UP;
    #else
        if(glutDeviceGet(GLUT_HAS_MOUSE)){
            //printf("Your mouse have %i buttons\n", glutDeviceGet(GLUT_NUM_MOUSE_BUTTONS)); 
    
            mouseWheelDown= glutDeviceGet(GLUT_NUM_MOUSE_BUTTONS)+1;
            mouseWheelUp=glutDeviceGet(GLUT_NUM_MOUSE_BUTTONS);
        }
    #endif

    if(state!=GLUT_DOWN){
        move_mode=NO_MOVE;
        return;
    }
    mouse_x=x;
    mouse_y=y;
    mm.ha_start=mm.ha;
    mm.va_start=mm.va;
    mm.sf_start=mm.sf;
    mm.mv_start=mm.mv;
    switch(btn){
    case GLUT_LEFT_BUTTON:
        //hauke
        gettimeofday(&tv, 0); 
        //FIX IT: get the system double click time
        if( (tv.tv_sec*1000000+tv.tv_usec-doubleClickTime) < 300000 && (tv.tv_sec*1000000+tv.tv_usec-doubleClickTime) > 5){ //1000000=1sec
            //printf("Double Click %f\n", tv.tv_sec*1000000+tv.tv_usec-doubleClickTime);
            if(!ced_picking(x,y,&mm.mv.x,&mm.mv.y,&mm.mv.z)){
               sock=__glutSockets;
               int id = SELECTED_ID;
               //printf(" ced_get_selected : socket connected: %d", sock->fd );	
               send( sock->fd , &id , sizeof(int) , 0 );
            }
        }else{
            //printf("Single Click\n");
            if(setting.fixed_view == 0){ //dont rotate the view when in side or front projection
                move_mode=TURN_XY;
            }
        }
        doubleClickTime=tv.tv_sec*1000000+tv.tv_usec;
        return;
        case GLUT_RIGHT_BUTTON:
          move_mode=ZOOM;
          return;
        case GLUT_MIDDLE_BUTTON:
          //#ifdef __APPLE__
          //    move_mode=ZOOM;
          //#else
          //    move_mode=ORIGIN;
	      //#endif
          move_mode=ORIGIN;
          return;
        default:
          break;
    }


    //hauke
    if (btn == mouseWheelUp || btn == 3 ){ // 3 is mouse-wheel-up under ubuntu
    
        selectFromMenu(VIEW_ZOOM_IN);
      //  mm.mv.z+=150./mm.sf;
      //  glutPostRedisplay();
        return;
    }
    if (btn == mouseWheelDown || btn == 4 ){ // 4 is mouse-wheel-down under ubuntu
        selectFromMenu(VIEW_ZOOM_OUT);
        return;
    
      //  mm.mv.z-=150./mm.sf;
      //  glutPostRedisplay();
    }
    //end hauke
    
}

void printBinaer(int x){
    printf("Binaer:"); 
    int i;
    for(i=20;i>=0;--i) {
        printf("%d",((x>>i)&1));
    }
    printf("\n");
}
/*
int isLayerVisible(int x){
    if( ((1<<(x))&ced_visible_layers) > 0){
        return(1);
    }else{
        return(0);
    }
}
*/

int isLayerVisible(int x){
    //return(ced_visible_layers[x]);

    return(setting.layer[x]);
}


/*
static void toggle_layer(unsigned l){
    //printf("Toggle layer %u:\n",l);
    //printBinaer(ced_visible_layers);
    ced_visible_layers^=(1<<l);
    //std::cout << "ced_visible_layers: "<<ced_visible_layers << std::endl;

    //  printf("Toggle Layer %u  and ced_visible_layers = %u \n",l,ced_visible_layers);  
    //printBinaer(ced_visible_layers);
}
*/
static void toggle_layer(unsigned l){
    if(l > MAX_LAYER-1){ return; }

//    if(ced_visible_layers[l]){
//        ced_visible_layers[l]=false;
//    }else{
//        ced_visible_layers[l]=true;
//    }


    if(setting.layer[l]){
        setting.layer[l]=false;
    }else{
        setting.layer[l]=true;
    }

}

/*
static void show_all_layers(void){
  ced_visible_layers=0xffffffff;
  //  printf("show all layers  ced_visible_layers = %u \n",ced_visible_layers);
}
*/

static void keypressed(unsigned char key,int x,int y){
    //SM-H: TODO: socket list for communicating with client
    //struct __glutSocketList *sock;
  
    glutSetWindow(mainWindow); //hauke
  
    if(key=='r' || key=='R'){ 
        selectFromMenu(VIEW_RESET);
    } else if(key=='f'){     
       selectFromMenu(VIEW_FRONT);
    } else if(key == 'F'){
      selectFromMenu(TOGGLE_Z_PROJECTION); 
    } else if(key=='s'){
        selectFromMenu(VIEW_SIDE);
    } else if(key=='S'){
        selectFromMenu(TOGGLE_PHI_PROJECTION);
    } else if(key==27) { //esc
        exit(0);
    } else if(key=='c' || key=='C'){
      //selectFromMenu(VIEW_CENTER);
      if(!ced_get_selected(x,y,&mm.mv.x,&mm.mv.y,&mm.mv.z)) glutPostRedisplay();
    } else if(key=='v' || key=='V'){
          selectFromMenu(VIEW_FISHEYE);
    } else if((key>='0') && (key<='9')){
          selectFromMenu(LAYER_0+key-'0');
    } else if(key==')'){ // 0
          selectFromMenu(LAYER_10);
    } else if(key=='!'){ // 1
          selectFromMenu(LAYER_11);
    } else if(key=='@'){ // 2
          selectFromMenu(LAYER_12);
    } else if(key=='#'){ // 3
          selectFromMenu(LAYER_13);
    } else if(key=='$'){ // 4
          selectFromMenu(LAYER_14);
    } else if(key=='%'){ // 5
          selectFromMenu(LAYER_15);
    } else if(key=='^'){ // 6
          selectFromMenu(LAYER_16);
    } else if(key=='&'){ // 7
          selectFromMenu(LAYER_17);
    } else if(key=='*'){ // 8
          selectFromMenu(LAYER_18);
    } else if(key=='('){ // 9
          selectFromMenu(LAYER_19);
    } else if(key=='`'){
          selectFromMenu(LAYER_ALL);
    } else if(key == '+'){
          selectFromMenu(VIEW_ZOOM_IN);
    } else if(key == '-'){
          selectFromMenu(VIEW_ZOOM_OUT);
    } else if(key == 'z'){
          if(setting.z_cutting < 7000){ setting.z_cutting+=100; };
          glutPostRedisplay();
    } else if(key == 'Z'){
          if(setting.z_cutting > -7000){ setting.z_cutting-=100; };
          glutPostRedisplay();
    } else if(key=='t'){ // t - momentum at ip layer 2
      toggle_layer(20);
      glutPostRedisplay();
    } else if(key=='y'){ // y - momentum at ip layer = 3
      toggle_layer(21);
      glutPostRedisplay();
    } else if(key=='u'){ // u - momentum at ip layer = 4
      toggle_layer(22);
      glutPostRedisplay();
    } else if(key=='i'){ // i - momentum at ip layer = 5
      toggle_layer(23);
      glutPostRedisplay();
    } else if(key=='o'){ // o - momentum at ip layer = 6
      toggle_layer(24);
      glutPostRedisplay();
    } else if(key=='b'){ // toggle background color
        ++iBGcolor;
        if (iBGcolor >= sizeof(bgColors)/sizeof(color_t)){
            glClearColor(userDefinedBGColor[0],userDefinedBGColor[1],userDefinedBGColor[2],userDefinedBGColor[3]);
            iBGcolor=-1;
            printf("using color: %s\n","user defined");
            glutPostRedisplay();
            return;
        }else{
            glClearColor(bgColors[iBGcolor][0],bgColors[iBGcolor][1],bgColors[iBGcolor][2],bgColors[iBGcolor][3]);
            glutPostRedisplay();
            printf("using color %u\n",iBGcolor);
        }
    } else if(key == 'h'){
          toggleHelpWindow();
    }
}

static void SpecialKey( int key, int x, int y ){
   switch (key) {
   case GLUT_KEY_UP:
       mm.mv.z+=50.;
      break;
   case GLUT_KEY_DOWN:
       mm.mv.z-=50.;
      break;
   default:
      return;
   }
   glutPostRedisplay();
}


static void motion(int x,int y){
  
    // printf("Mouse moved: %dx%d %f\n",x,y,angle_z);
    if((move_mode == NO_MOVE) || !window_width || !window_height)
      return;
  
    if(move_mode == TURN_XY){
      //    angle_y=correct_angle(start_angle_y-(x-mouse_x)*180./window_width);
      //    turn_xy((x-mouse_x)*M_PI/window_height,
      //           (y-mouse_y)*M_PI/window_width);
      mm.ha=mm.ha_start+(x-mouse_x)*180./window_width;
      mm.va=mm.va_start+(y-mouse_y)*180./window_height;
      
      //todo
    } else if (move_mode == ZOOM){
        mm.sf=mm.sf_start+(y-mouse_y)*10./window_height;
        if(mm.sf<0)
  	  mm.sf=0.001;
        else if(mm.sf>2000.)
  	  mm.sf=2000.;
     
    } else if (move_mode == ORIGIN){
        /* 
        //old code: do not work with rotate 
        mm.mv.x=mm.mv_start.x-(x-mouse_x)*WORLD_SIZE/window_width
        mm.mv.y=mm.mv_start.y+(y-mouse_y)*WORLD_SIZE/window_height
        */
  
        float grad2rad=3.141*2/360;
        float x_factor_x =  cos(mm.ha*grad2rad);
        float x_factor_y =  cos((mm.va-90)*grad2rad)*cos((mm.ha+90)*grad2rad);
        float y_factor_x =  0; 
        float y_factor_y = -cos(mm.va*grad2rad);
        float z_factor_x =  cos((mm.ha-90)*grad2rad);
        float z_factor_y = -cos(mm.ha*grad2rad)*cos((mm.va+90)*grad2rad);
  
        //float scale_factor=2200/mm.sf/exp(log(window_width*window_height)/2) ;
        float scale_factor=580/mm.sf/exp(log(window_width*window_height)/2.5) ;
  
  
        //mm.mv.x=mm.mv_start.x- (x-mouse_x)*WORLD_SIZE/window_width*10*x_factor_x - (y-mouse_y)*WORLD_SIZE/window_width*10*x_factor_y;
        //mm.mv.y=mm.mv_start.y- (x-mouse_x)*WORLD_SIZE/window_width*10*y_factor_x - (y-mouse_y)*WORLD_SIZE/window_width*10*y_factor_y;
        //mm.mv.z=mm.mv_start.z - (x-mouse_x)*WORLD_SIZE/window_width*10*z_factor_x - (y-mouse_y)*WORLD_SIZE/window_width*10*z_factor_y;
  
        mm.mv.x=mm.mv_start.x- scale_factor*(x-mouse_x)*x_factor_x - scale_factor*(y-mouse_y)*x_factor_y;
        mm.mv.y=mm.mv_start.y- scale_factor*(x-mouse_x)*y_factor_x - scale_factor*(y-mouse_y)*y_factor_y;
        mm.mv.z=mm.mv_start.z -scale_factor*(x-mouse_x)*z_factor_x - scale_factor*(y-mouse_y)*z_factor_y;
  
  
        //printf("y_factor_x = %f, y_factor_y=%f\n", y_factor_x, y_factor_y);
        //printf("mm.ha = %f, mm.va = %f\n",mm.ha, mm.va);
    }   
    glutPostRedisplay();
}

static void timer (int val)
{
    //change timer for testing to 1
    fd_set fds;
    int rc;
    struct __glutSocketList *sock;
    int max_fd=0;
  
    /* set timeout to 0 for nonblocking select call */
    struct timeval timeout={0,0};
  
    FD_ZERO(&fds);
  
    for(sock=__glutSockets;sock;sock=sock->next)
      {
        FD_SET(sock->fd,&fds);
        if(sock->fd>max_fd)
  	max_fd=sock->fd;
      }
    /* FIXME? Is this the correct way for a non blocking select call? */
    rc = select(max_fd + 1, &fds, NULL, NULL, &timeout);
    if (rc < 0) 
      {
        if (errno == EINTR) 
         {
  	  //glutTimerFunc(500,timer,01);
        //glutTimerFunc(50,timer,1);
        glutTimerFunc(1,timer,1);
  
  	  return;
  	} 
        else 
  	{
  	  perror("In glced::timer, during select.");
  	  exit(-1);
  	}
      }
    // speedup if rc==0
    else if (rc>0)
      {
        for(sock=__glutSockets;sock;sock=sock->next)
  	{
  	  if(FD_ISSET(sock->fd,&fds))
  	    {
  	      (*(sock->read_func))(sock);
            //printf("reading...\n");
  	      //glutTimerFunc(500,timer,01);
            //glutTimerFunc(50,timer,01);
          glutTimerFunc(1,timer,01);
  
  
  	      return ; /* to avoid complexity with removed sockets */
  	    }
  	}
      }
  
    //fix for old glut version
    glutSetWindow(mainWindow); 
  
    //glutTimerFunc(200,timer,01);
    glutTimerFunc(1,timer,01);
    return;
}



int glut_tcp_server(unsigned short port, void (*user_func)(void *data));

static void input_data(void *data){
    if(ced_process_input(data)>0){
        glutPostRedisplay();
    }
}


//http://www.linuxfocus.org/English/March1998/article29.html
void drawString (char *s){
    unsigned int i;
    for (i = 0; i[s]; i++){
        glutBitmapCharacter (GLUT_BITMAP_HELVETICA_10, s[i]);
      //glutBitmapCharacter (GLUT_BITMAP_9_BY_15, s[i]);
    }
}

void drawStringBig (char *s){
    unsigned int i;
    for (i = 0; i[s]; i++){
        glutBitmapCharacter (GLUT_BITMAP_HELVETICA_18, s[i]);
    }
}

void drawHelpString (char *string, float x,float y){ //format help strings strings: "[<key>] <description>"
    unsigned int i;
    glRasterPos2f(x,y);
  
    int monospace = 0;
    for (i = 0; string[i]; i++){
        if(string[i] == '['){ 
            monospace = 1;
            glutBitmapCharacter (GLUT_BITMAP_HELVETICA_10, '[');
            i++;
        }
        else if(string[i] == ']'){
             monospace = 0;
        }
        if(monospace){
            glutBitmapCharacter(GLUT_BITMAP_8_BY_13, string[i]);
        }else{
            glutBitmapCharacter (GLUT_BITMAP_HELVETICA_10, string[i]);
        }
    }
}


void subDisplay(void){
    char label[MAX_LAYER_CHAR];
    int i;

    glutSetWindow(subWindow);
    //glClearColor(0.5, 0.5, 0.5, 100);
    glClearColor(0.5, 0.5, 0.5, 0.5);


    //std::cout << glutGet(GLUT_WINDOW_WIDTH) << " vs " << window_width << std::endl; 
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float line = 45/window_height; //height of one line
    //float column = 200/window_width;
    float column = 200/window_width; //width of one line

    const int ITEMS_PER_COLUMN=int(window_height/60.0); //how many lines per column?
    //const int MAX_COLUMN= window_width/100;
    //border
    glColor3f(0,0.9,.9);
    glBegin(GL_LINE_LOOP);
    glVertex2f(0.001, 0.01);
    glVertex2f(0.001, 0.99);
    glVertex2f(0.999, 0.99);
    glVertex2f(0.999, 0.01);
    glEnd();

    glColor3f(1.0, 1.0, 1.0); //white

    //printf("window_height %f\nwindow width %f\n", window_height, window_width);
    char *shortcuts[]={
            "[h] Toggle shortcut frame",
            "[r] Reset view",
            "[f] Font view",
            "[s] Side view", 
            "[v] Fish eye view",
            "[b] Change background color",
            "[+] Zoom in",
            "[-] Zoom out",
            "[c] Center",
            "[Z] Move in z-axe direction",
            "[z] Move in -z-axe direction",
            "[`] Toggle all layers",
            "[Esc] Quit CED"
            };

    glColor3f(1.0, 1.0, 1.0);
    sprintf (label, "Control keys");
    glRasterPos2f(((int)(0/ITEMS_PER_COLUMN))*column+0.02, 0.80F);
    drawStringBig(label);

    for(i=0;(unsigned) i<sizeof(shortcuts)/sizeof(char *);i++){
       //if((i/ITEMS_PER_COLUMN) > MAX_COLUMN) break;
       //sprintf(label,"%s", shortcuts[i]);
       //glRasterPos2f(((int)(i/ITEMS_PER_COLUMN))*column+0.02,(ITEMS_PER_COLUMN-(i%ITEMS_PER_COLUMN))*line);
       //printf("i=%i  lineposition=%i\n",i, (ITEMS_PER_COLUMN-(i%ITEMS_PER_COLUMN)));
       drawHelpString(shortcuts[i], ((int)(i/ITEMS_PER_COLUMN))*column+0.02, (ITEMS_PER_COLUMN-(i%ITEMS_PER_COLUMN))*line );
        //drawString(label);
    }

    int actual_column=(int)((i-1)/ITEMS_PER_COLUMN)+1;

    int aline=0;
    int j=0;
    char tmp[MAX_LAYER_CHAR];
    int jj=0;

    glColor3f(1.0, 1.0, 1.0);
    sprintf (label, "Layers");
    glRasterPos2f(((int)(aline/ITEMS_PER_COLUMN)+actual_column)*column, 0.80F);
    drawStringBig(label);

    for(i=0;i<NUMBER_DATA_LAYER;i++){
        for(j=0;j<MAX_LAYER_CHAR-1;j++){
            if(layerDescription[i][j] != ','){
                tmp[j]=layerDescription[i][j];
            }else{
                tmp[j]=0;
                j+=2;
                break;
            }
        }
        
       //sprintf(label,"[%c] %s%i: %s", layer_keys[i], (i<10)?"0":"", i, layerDescription[i]);
        sprintf(label,"[%c] %s%i: %s", layer_keys[i], (i<10)?"0":"", i, tmp);
        drawHelpString(label, ((int)(aline/ITEMS_PER_COLUMN)+actual_column)*column,(ITEMS_PER_COLUMN-(aline%ITEMS_PER_COLUMN))*line);
        aline++;

        jj=j;

        for(;j<MAX_LAYER_CHAR-1;j++){
            if(layerDescription[i][j] == ',' || layerDescription[i][j] == 0){
                tmp[j-jj]=0;
                j++; //scrip ", "
                jj=j+1;
                //drawHelpString(tmp, ((int)(aline/ITEMS_PER_COLUMN)+actual_column+.18)*column,(ITEMS_PER_COLUMN-(aline%ITEMS_PER_COLUMN))*line);
                sprintf(label,"[%c] %s%i: %s", layer_keys[i], (i<10)?"0":"", i, tmp);
                drawHelpString(label, ((int)(aline/ITEMS_PER_COLUMN)+actual_column)*column,(ITEMS_PER_COLUMN-(aline%ITEMS_PER_COLUMN))*line);

                aline++;
                if(layerDescription[i][j] == 0){ break; }
            }else{
                tmp[j-jj]=layerDescription[i][j];
            }
        }
    }
    glutSwapBuffers ();
}
void subReshape (int w, int h)
{
  glViewport (0, 0, w, h);
  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  gluOrtho2D (0.0F, 1.0F, 0.0F, 1.0F);
};
void writeString(char *str,int x,int y){
    int i;
    glColor3f(0, 0.0, 0.0);//print timer in red
    glRasterPos2f(x, y);

    for(i=0;str[i];i++){
       //glRasterPos2f(x+i*10,y);
       glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_10,str[i]);
       glutStrokeCharacter(GLUT_STROKE_ROMAN, str[i]); 
       printf("char = %c", str[i]);
    }
    //glPopMatrix();
}
void toggleHelpWindow(void){ //hauke
    mainWindow=glutGetWindow();
    
    if(showHelp == 1){
        glutDestroyWindow(subWindow);
        showHelp=0;
    }else if(showHelp == 0){
        subWindow=glutCreateSubWindow(mainWindow,5,5,int(window_width-10),int(window_height/4.0));

        glutDisplayFunc(subDisplay);
        glutReshapeFunc(subReshape);
            
        glutKeyboardFunc(keypressed);
        glutSpecialFunc(SpecialKey);

        glutPostRedisplay();

        glutSetWindow(mainWindow);
        showHelp=1;
    }    
    glutSetWindow(mainWindow);
}

void updateLayerEntryInPopupMenu(int id){ //id is layer id, not menu id!
    char string[200];
    char tmp[41];
    if(id < 0 || id > NUMBER_POPUP_LAYER-1){
        return;
    }
    strncpy(tmp, layerDescription[id], 40); 
    tmp[40]=0;
    
    sprintf(string,"[%s] Layer %s%i [%c]: %s%s",isLayerVisible(id)?"X":"   ", (id < 10)?"0":"" ,id, layer_keys[id], tmp, (strlen(layerDescription[id]) > 40)?"...":"");
    glutSetMenu(layerMenu);
    glutChangeToMenuEntry(id+2,string, id+LAYER_0);                     
}

void updateLayerEntryDetector(int id){ //id is layer id, not menu id!
    char string[200];
    char tmp[41];
    if(id < NUMBER_DATA_LAYER || id > NUMBER_DETECTOR_LAYER+NUMBER_DATA_LAYER-1 || id > MAX_LAYER-1 || id < 0){
        return;
    }
    strncpy(tmp, layerDescription[id], 40); 
    tmp[40]=0;
    
    //sprintf(string,"[%s] Layer %s%i [%c]: %s%s",isLayerVisible(id)?"X":"   ", (id < 10)?"0":"" ,id, layer_keys[id], tmp, (strlen(layerDescription[id]) > 40)?"...":"");
    sprintf(string,"[%s] Layer %s%i: %s%s",isLayerVisible(id)?"X":"   ", (id < 10)?"0":"" ,id,tmp, (strlen(layerDescription[id]) > 40)?"...":"");

    glutSetMenu(detectorMenu);
    glutChangeToMenuEntry(id-NUMBER_DATA_LAYER+2,string, id-NUMBER_DATA_LAYER+DETECTOR1);                     
}


void addLayerDescriptionToMenu(int id, char * str){
    if(id < 0 || id >= MAX_LAYER){
        printf("Warning: Layer id out of range\n");
        return;
    }
    strncpy(layerDescription[id], str,MAX_LAYER_CHAR-1);
    updateLayerEntryInPopupMenu(id);
    updateLayerEntryDetector(id);

}


void update_cut_angle_menu(void){
    char str[200];

    int i;

    glutSetMenu(subsubMenu2);

    for(i=0; (unsigned)i < sizeof(available_cutangles)/sizeof(available_cutangles[0]); i++){

        if(available_cutangles[i] == setting.cut_angle){
            sprintf(str,"[X] %i", available_cutangles[i]);
            glutChangeToMenuEntry(i+1, str,  CUT_ANGLE0+i);
        }else{
            sprintf(str,"[  ] %i", available_cutangles[i]);
            glutChangeToMenuEntry(i+1, str,  CUT_ANGLE0+i);
        }
    }

}


void selectFromMenu(int id){ //hauke
    int i;
    int anz;
    static float z_cutting_backup;
    static float cut_angle_backup;
    static float mm_ha_backup; 
    static float mm_va_backup;
    static int graphic_2_backup;
    static int fullscreen=false;


    glutSetWindow(mainWindow); //hauke


    switch(id){
        case BGCOLOR_GAINSBORO:
            glClearColor(0.862745,0.862745,0.862745,0);
            break;

        case BGCOLOR_LIGHTGREY:
            glClearColor(0.827451,0.827451,0.827451,0);
            break;

        case BGCOLOR_DARKGRAY:
            glClearColor(0.662745,0.662745,0.662745,0);
            break;

        case BGCOLOR_GRAY:
            glClearColor(0.501961,0.501961,0.501961,0);
            break;

        case BGCOLOR_SILVER:
            glClearColor(0.7529,0.7529,0.7529,0);
            break;

        case BGCOLOR_DIMGRAY:
            glClearColor(0.4118,0.4118,0.4118,0);
            break;

        case BGCOLOR_LIGHTSTEELBLUE:
            glClearColor(0.6902,0.7686 ,0.8706,0);
            break;

        case BGCOLOR_STEELBLUE:
            glClearColor(0.2745,0.5098,0.70588,0);
            break;

        case BGCOLOR_SEAGREEN:
            glClearColor(0.18039,0.54509,0.34117,0);
            break;

        case BGCOLOR_ORANGE:
            glClearColor(1,0.647,0,0);
            break;

        case BGCOLOR_YELLOW:
            glClearColor(1,1,0,0);
            break;

        case BGCOLOR_VIOLET:
            glClearColor(0.9333,0.5098,0.9333,0);
            break;

        case BGCOLOR_BLACK:
            glClearColor(0,0,0,0);
            break;

        case BGCOLOR_BLUE:
            glClearColor(0,0.2,0.4,0);
            break;

        case BGCOLOR_WHITE:
            glClearColor(1,1,1,0);
            break;

        case BGCOLOR_USER:
            glClearColor(userDefinedBGColor[0],userDefinedBGColor[1], userDefinedBGColor[2], userDefinedBGColor[3]);

        case VIEW_RESET:
            //if(graphic[2] == 0){selectFromMenu(GRAFIC_PERSP); }
            if(setting.persp == false){selectFromMenu(GRAFIC_PERSP); }
            setting.z_cutting=7000; //no z cutting
            setting.cut_angle=0;    // no detector cutting
            setting.phi_projection = false; // no phi projection
            setting.z_projection=false; // no phi projection;
            mm=mm_reset;
            //mm.sf = fisheye_alpha > 0 ? mm.sf*8.0: mm.sf;
            fisheye_alpha=0;
            setting.fixed_view=false;
            update_cut_angle_menu();
            set_world_size(DEFAULT_WORLD_SIZE ); 
            //std::cout << "DEFAULT_WORLD_SIZE "  << DEFAULT_WORLD_SIZE << "zoom: " << mm.sf << std::endl;
            break;


        case VIEW_FISHEYE:
            if(fisheye_alpha==0.0){
                mm.sf *= 8.0; //zoom in to hold the same detector size
                fisheye_alpha = 1e-3;
                FISHEYE_WORLD_SIZE = WORLD_SIZE/(WORLD_SIZE*fisheye_alpha); //<-- new
                set_world_size(WORLD_SIZE); // <-- old
            }
            else{
                mm.sf *= 1.0/8.0; //zoom out for the same look
                fisheye_alpha = 0.0;
                set_world_size(FISHEYE_WORLD_SIZE); //<-- old
            }
            break;

        case VIEW_FRONT:
            //mm=mm_reset;
            //mm.sf = fisheye_alpha > 0 ? mm.sf*8.0: mm.sf;

            if(setting.fixed_view){ break;}

                mm.ha=mm.va=0.;

            break;

        case VIEW_SIDE:
            //mm=mm_reset;
            //mm.sf = fisheye_alpha > 0 ? mm.sf*8.0: mm.sf;
            if(setting.fixed_view){ break;}

                mm.ha=90.;
                mm.va=0.;

            break;

        case TOGGLE_PHI_PROJECTION:
            if(setting.phi_projection){ //turn projection off
                setting.phi_projection=false;
                setting.z_cutting=z_cutting_backup;
                setting.cut_angle=cut_angle_backup;
                //if(graphic_2_backup != graphic[2]){selectFromMenu(GRAFIC_PERSP); } //restore persp setting
                if(graphic_2_backup != setting.persp){selectFromMenu(GRAFIC_PERSP); } //restore persp setting

                mm.ha = mm_ha_backup;
                mm.va = mm_va_backup;

                setting.fixed_view=false;

            }else{ //turn projection on
                if(setting.z_projection){
                    selectFromMenu(TOGGLE_Z_PROJECTION);
                }

                z_cutting_backup=setting.z_cutting;
                cut_angle_backup=setting.cut_angle;

                setting.phi_projection=true;

                //graphic_2_backup=graphic[2];
                graphic_2_backup=setting.persp;

                //if(graphic[2]==1){selectFromMenu(GRAFIC_PERSP); }
                if(setting.persp==1){selectFromMenu(GRAFIC_PERSP); }


                setting.cut_angle=180;
                setting.z_cutting=7000;
                mm_ha_backup=mm.ha;
                mm_va_backup = mm.va;
                mm.ha=90.;
                mm.va=0.;

                setting.fixed_view=true;
            }

            update_cut_angle_menu();
            break;

        case TOGGLE_Z_PROJECTION:
            if(setting.z_projection){ //turn projection off

                setting.z_projection=false;
                //z_cutting=7000;
                //selectFromMenu(GRAFIC_PERSP);
                setting.z_cutting=z_cutting_backup;
                setting.cut_angle=cut_angle_backup;
                //if(graphic[2]==0){selectFromMenu(GRAFIC_PERSP); }
                //if(graphic_2_backup != graphic[2]){selectFromMenu(GRAFIC_PERSP); } //restore persp setting
                if(graphic_2_backup != setting.persp){selectFromMenu(GRAFIC_PERSP); } //restore persp setting


                mm.ha = mm_ha_backup;
                mm.va = mm_va_backup;

                setting.fixed_view=false;
            }else{ //turn projection on

                if(setting.phi_projection){selectFromMenu(TOGGLE_PHI_PROJECTION);}

                z_cutting_backup=setting.z_cutting;
                cut_angle_backup=setting.cut_angle;

                setting.z_projection=true;
                setting.cut_angle=0;
                setting.z_cutting=10;


                //graphic_2_backup=graphic[2];
                graphic_2_backup=setting.persp;

                //if(graphic[2]==1){selectFromMenu(GRAFIC_PERSP); }
                if(setting.persp==true){selectFromMenu(GRAFIC_PERSP); }

               
               //side view
                mm_ha_backup=mm.ha;
                mm_va_backup = mm.va;

                mm.ha=0.;
                mm.va=0.;




                setting.fixed_view=true;
            }

            update_cut_angle_menu();
            break;

        case VIEW_ZOOM_IN:
            mm.sf += mm.sf*50.0/window_height;
            //reshape((int)window_width, (int)window_height);

            //if(mm.sf>50){ mm.sf=50; }
            break;

        case VIEW_ZOOM_OUT:
            mm.sf -= mm.sf*50.0/window_height;
            //reshape((int)window_width, (int)window_height);

            //if(mm.sf<0.01){ mm.sf=0.01; }
            break;

        case VIEW_CENTER:
            //ced_get_selected(x,y,&mm.mv.x,&mm.mv.y,&mm.mv.z);
            break;

        case LAYER_ALL:
            glutSetMenu(layerMenu);
            anz=0;
            for(i=0;i<NUMBER_POPUP_LAYER;i++){ //try to turn all layers on
                if(!isLayerVisible(i)){
                   //sprintf(string,"[X] Layer %s%i [%c]: %s", (i < 10)?"0":"" ,i, layer_keys[i], layerDescription[i]);
                   //glutChangeToMenuEntry(i+2,string, LAYER_0+i);                     
                   toggle_layer(i);
                   updateLayerEntryInPopupMenu(i);
                   anz++;
                }
            }
            if(anz == 0){ //turn all layers off
                for(i=0;i<NUMBER_POPUP_LAYER;i++){
                   //sprintf(string,"[   ] Layer %s%i [%c]: %s",(i < 10)?"0":"" ,i, layer_keys[i], layerDescription[i]);
                   //glutChangeToMenuEntry(i+2,string, LAYER_0+i);                     
                   toggle_layer(i);
                   updateLayerEntryInPopupMenu(id);
                }
            }
            break;

        case DETECTOR_ALL:
            glutSetMenu(detectorMenu);
            anz=0;
            for(i=NUMBER_DATA_LAYER;i<NUMBER_DETECTOR_LAYER+NUMBER_DATA_LAYER;i++){ //try to turn all layers on
                if(!isLayerVisible(i)){
                   //sprintf(string,"[X] Layer %s%i [%c]: %s", (i < 10)?"0":"" ,i, layer_keys[i], layerDescription[i]);
                   //glutChangeToMenuEntry(i+2,string, LAYER_0+i);                     
                   toggle_layer(i);
                   updateLayerEntryDetector(i);
                   anz++;
                }
            }
            if(anz == 0){ //turn all layers off
                for(i=NUMBER_DATA_LAYER;i<NUMBER_DETECTOR_LAYER+NUMBER_DATA_LAYER;i++){
                   //sprintf(string,"[   ] Layer %s%i [%c]: %s",(i < 10)?"0":"" ,i, layer_keys[i], layerDescription[i]);
                   //glutChangeToMenuEntry(i+2,string, LAYER_0+i);                     
                   toggle_layer(i);
                   updateLayerEntryDetector(id);
                }
            }
            break;


        case DETECTOR1:
        case DETECTOR2:
        case DETECTOR3:
        case DETECTOR4:
        case DETECTOR5:
        case DETECTOR6:
        case DETECTOR7:
        case DETECTOR8:
        case DETECTOR9:
        case DETECTOR10:
        case DETECTOR11:
        case DETECTOR12:
        case DETECTOR13:
        case DETECTOR14:
        case DETECTOR15:
        case DETECTOR16:
        case DETECTOR17:
        case DETECTOR18:
        case DETECTOR19:
        case DETECTOR20:

            glutSetMenu(detectorMenu);
            toggle_layer(id-DETECTOR1+NUMBER_DATA_LAYER);
            //std::cout << "toogle layer " << id-DETECTOR1 + NUMBER_DATA_LAYER<< std::endl;
            updateLayerEntryDetector(id-DETECTOR1+NUMBER_DATA_LAYER);
            break;



        case LAYER_0:
        case LAYER_1:
        case LAYER_2:
        case LAYER_3:
        case LAYER_4:
        case LAYER_5:
        case LAYER_6:
        case LAYER_7:
        case LAYER_8:
        case LAYER_9:
        case LAYER_10:
        case LAYER_11:
        case LAYER_12:
        case LAYER_13:
        case LAYER_14:
        case LAYER_15:
        case LAYER_16:
        case LAYER_17:
        case LAYER_18:
        case LAYER_19:
            glutSetMenu(layerMenu);
            toggle_layer(id-LAYER_0);
            //std::cout << "toogle layer " << id-LAYER_0 << std::endl;
            updateLayerEntryInPopupMenu(id-LAYER_0);
            break;

        case CUT_ANGLE0:
            setting.cut_angle=0; 
            update_cut_angle_menu();
            break;

        case CUT_ANGLE30:
            setting.cut_angle=30; 
            update_cut_angle_menu();
            break;

        case CUT_ANGLE90:
            setting.cut_angle=90;
            update_cut_angle_menu();
            break;

        case CUT_ANGLE135:
            setting.cut_angle=135;
            update_cut_angle_menu();
            break;

        case CUT_ANGLE180:
            setting.cut_angle=180;
            update_cut_angle_menu();
            break;

        case CUT_ANGLE270:
            setting.cut_angle=270;
            update_cut_angle_menu();
            break;

        case CUT_ANGLE360:
            setting.cut_angle=360;
            update_cut_angle_menu();
            break;

        case TRANS0:
            setting.trans_value=0;
            break;

        case TRANS40:
            setting.trans_value=0.4;
            break;

        case TRANS60:
            setting.trans_value=0.6;
            break;

        case TRANS70:
            setting.trans_value=0.7;
            break;

        case TRANS80:
            setting.trans_value=0.8;
            break;

        case TRANS90:
            setting.trans_value=0.9;
            break;

        case TRANS95:
            setting.trans_value=0.95;
            break;

        case TRANS100:
            setting.trans_value=1.0;
            break;
//        case FULLSCREEN:
//////            glutDestroyWindow(mainWindow);;
//////            glutGameModeString("1280x1024:32@60");
//////            glutEnterGameMode();
////            if(fullscreen == false){
////                glutFullScreen(); 
////                fullscreen = true;
////            }else{
////                fullscreen = false; 
////                reshape(setting.win_w, setting.win_h);
////            }
//            
        case AXES:
            if(setting.show_axes){
                setting.show_axes= false;
            }else{
                setting.show_axes= true;
            }
            break;

        case FPS:  
            if(setting.fps){
                glutIdleFunc(NULL);
                setting.fps=false;
            }else{
                glutIdleFunc(idle);
                setting.fps=true;
            }
            break;

        case GRAFIC_HIGH:
            setting.light=true;
            setting.trans=false;
            setting.persp=false;
            selectFromMenu(GRAFIC_TRANS);
            selectFromMenu(GRAFIC_LIGHT);
            selectFromMenu(GRAFIC_PERSP);
            break;
            
        case GRAFIC_LOW:
            setting.light=true;
            setting.trans=true;
            setting.persp=true;
            selectFromMenu(GRAFIC_TRANS);
            selectFromMenu(GRAFIC_LIGHT);
            selectFromMenu(GRAFIC_PERSP);
            break;

        case GRAFIC_TRANS:
/*
            if(graphic[1] == 1){
                //printf("Transparency  is now off\n");
                graphic[1] = 0;
            }else{
                //printf("Transparency  is now on\n");
                graphic[1] = 1;
            }

*/

            if(setting.trans == true){
                //printf("Transparency  is now off\n");
                setting.trans = false;
            }else{
                //printf("Transparency  is now on\n");
                setting.trans = true;
            }

            break;
            
        case GRAFIC_LIGHT:
            //if(graphic[0] == 1){
            if(setting.light == true){

                //printf("Light  is now on\n");
                //graphic[0] = 0;
                setting.light=false;
                glDisable(GL_LIGHTING); 
            }else{
                 //printf("Light is now on\n");
                 //graphic[0] = 1;
                setting.light = true;

                 break; //do nothing...

                 //TODO: CHANGE IT
                 GLfloat light0_spec[] = {1, 1, 1, 0.5};
                 GLfloat light0_pos[] = {0, 0, 8000};
                 //GLfloat light0_ambi[]= {0.5, 0.5, 0.5, 0.5};     

                 //glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_ambi);
                 //glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambi);
                 glLightfv(GL_LIGHT0, GL_SPECULAR, light0_spec);


                 glLightfv(GL_LIGHT0, GL_POSITION, light0_pos);
                 
                 ////glClearColor (0.0, 0.0, 0.0, 0.0);
                 //glShadeModel (GL_SMOOTH);



                 //glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
                 //glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

                 glColorMaterial ( GL_FRONT_AND_BACK, GL_EMISSION ) ;
                 glColorMaterial (GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE) ;
                 glEnable (GL_COLOR_MATERIAL) ;


                 glEnable(GL_NORMALIZE);

                 glEnable(GL_LIGHTING); 
                 glEnable(GL_LIGHT0);

                 glEnable(GL_DEPTH_TEST);

                 glMatrixMode(GL_MODELVIEW);
            }
            break;

        case GRAFIC_ALIAS:
            //if(graphic[3] == 1){
            if(setting.antia == true){
                printf("Anti aliasing is off\n");
                //graphic[3] = 0;
                setting.antia = false;
                reshape((int)window_width, (int)window_height);
            }else{
                printf("Anti aliasing is on\n");
                //graphic[3] = 1;
                setting.antia=true;
                reshape((int)window_width, (int)window_height);
            }
            break;

        case GRAFIC_PERSP:
            if(setting.persp == true){
                //printf("Perspective is now flat\n");
                setting.persp = false;

                reshape((int)window_width, (int)window_height); //hack, call resize function to overwrite perspectivic settings
            }else{
                //printf("Perspective is now 3d\n");
                setting.persp = true;
                reshape((int)window_width, (int)window_height); //hack, call resize function to overwrite perspectivic settings
            }
            break;
        case HELP:
            toggleHelpWindow();
            break;
        case SAVE:
            saveSettings(); 
            break;

    }

    reshape((int)window_width, (int)window_height);
    glutPostRedisplay();
}

int buildMenuPopup(void){ //hauke
    int menu;
    int subMenu1;
    int subMenu2;
    int subMenu3;
    int subMenu4;
    int subsubMenu1;
    int subsubMenu3;
    int DetectorComponents;
    //int subsubMenu2; //extern




    subMenu1 = glutCreateMenu(selectFromMenu);
    glutAddMenuEntry("White",BGCOLOR_WHITE);
    glutAddMenuEntry("Gainsboro", BGCOLOR_GAINSBORO);
    glutAddMenuEntry("Lightgrey", BGCOLOR_LIGHTGREY);
    glutAddMenuEntry("Silver", BGCOLOR_SILVER);
    glutAddMenuEntry("Darkgray", BGCOLOR_DARKGRAY);
    glutAddMenuEntry("Gray", BGCOLOR_GRAY);
    glutAddMenuEntry("Dimgray", BGCOLOR_DIMGRAY);
    glutAddMenuEntry("Black",BGCOLOR_BLACK);


    glutAddMenuEntry("Lightsteelblue",BGCOLOR_LIGHTSTEELBLUE);
    glutAddMenuEntry("Steelblue",BGCOLOR_STEELBLUE);
    glutAddMenuEntry("Blue",BGCOLOR_BLUE);

    glutAddMenuEntry("Seagreen",BGCOLOR_SEAGREEN);

    glutAddMenuEntry("Orange",BGCOLOR_ORANGE);

    glutAddMenuEntry("Yellow",BGCOLOR_YELLOW);

    glutAddMenuEntry("Violet",BGCOLOR_VIOLET);

    if(userDefinedBGColor[0] >= 0){ //is set
        glutAddMenuEntry("User defined",BGCOLOR_USER);
    }





    subMenu2 = glutCreateMenu(selectFromMenu);
    glutAddMenuEntry("Reset view [r]", VIEW_RESET);
    glutAddMenuEntry("Front view [f]", VIEW_FRONT);
    glutAddMenuEntry("Side view [s]", VIEW_SIDE);
    glutAddMenuEntry("Toggle Phi-projection [S]", TOGGLE_PHI_PROJECTION);
    glutAddMenuEntry("Toggle Z-projection [S]", TOGGLE_Z_PROJECTION);
    glutAddMenuEntry("Toggle Fisheye projection [v]",VIEW_FISHEYE);
    glutAddMenuEntry("Zoom in [+]", VIEW_ZOOM_IN);
    glutAddMenuEntry("Zoom out [-]", VIEW_ZOOM_OUT);
    //glutAddMenuEntry("Center [c]", VIEW_CENTER);



    //set up detector components and data layer menu
    int i;
    subMenu3 = glutCreateMenu(selectFromMenu);
    layerMenu=subMenu3;


    glutAddMenuEntry("Show/Hide all data Layers [`]", LAYER_ALL);
    for(i=0;i<NUMBER_POPUP_LAYER;i++){
        //sprintf(string,"[%s] Layer %s%i [%c]: %s",isLayerVisible(i)?"X":"   ", (i < 10)?"0":"" ,i, layer_keys[i], layerDescription[i]);
        glutAddMenuEntry(" ",LAYER_0+i);
        //updateLayerEntryInPopupMenu(LAYER_0+i);
    }




    DetectorComponents = glutCreateMenu(selectFromMenu);
    glutAddMenuEntry("Show/Hide all detector components", DETECTOR_ALL);
    for(i=NUMBER_DATA_LAYER;i<NUMBER_DETECTOR_LAYER+NUMBER_DATA_LAYER;i++){
        glutAddMenuEntry(" ",DETECTOR1+i-NUMBER_DATA_LAYER);
    }
    detectorMenu=DetectorComponents;



    subsubMenu1 = glutCreateMenu(selectFromMenu);
    glutAddMenuEntry("Perspective",GRAFIC_PERSP);
    //glutAddMenuEntry("Deepbuffer", GRAFIC_BUFFER);
    glutAddMenuEntry("Transparency/mesh", GRAFIC_TRANS);
    //glutAddMenuEntry("Light", GRAFIC_LIGHT);
    glutAddMenuEntry("Anti Aliasing", GRAFIC_ALIAS);
    glutAddMenuEntry("Toggle visible of axes", AXES);
    #ifndef __APPLE__
        glutAddMenuEntry("Show FPS", FPS);
    #endif








    subsubMenu2 = glutCreateMenu(selectFromMenu);

    for(i=0; (unsigned) i < sizeof(available_cutangles) / sizeof(available_cutangles[0]); i++){
        glutAddMenuEntry(" ",  CUT_ANGLE0+i);
    }

    update_cut_angle_menu();


    subsubMenu3=glutCreateMenu(selectFromMenu);
    glutAddMenuEntry("  0%",TRANS0);
    glutAddMenuEntry("40%",TRANS40);
    glutAddMenuEntry("60%",TRANS60);
    glutAddMenuEntry("70%",TRANS70);
    glutAddMenuEntry("80%",TRANS80);
    glutAddMenuEntry("90%",TRANS90);
    glutAddMenuEntry("95%",TRANS95);
    glutAddMenuEntry("100%",TRANS100);




    subMenu4 = glutCreateMenu(selectFromMenu);
    glutAddMenuEntry("Classic View",GRAFIC_LOW);
    glutAddMenuEntry("New View", GRAFIC_HIGH);
//    glutAddMenuEntry("Full Screen mode", FULLSCREEN);
    glutAddSubMenu("Graphic details", subsubMenu1);
    glutAddSubMenu("Transparency value", subsubMenu3);







    menu=glutCreateMenu(selectFromMenu);
    glutAddSubMenu("View", subMenu2);
    glutAddSubMenu("Data layers", subMenu3);
    glutAddSubMenu("Detector components", DetectorComponents);
    glutAddSubMenu("Detector cuts", subsubMenu2);
    glutAddSubMenu("Background Color", subMenu1);
    glutAddSubMenu("Graphics options", subMenu4);
    glutAddMenuEntry("Save Settings",SAVE);
    glutAddMenuEntry("Toggle help [h]",HELP);


    return menu;
}


int main(int argc,char *argv[]){
    bool geometry = false;

    mm_reset=mm;
    WORLD_SIZE = DEFAULT_WORLD_SIZE ;

    loadSettings();  
    set_bg_color(setting.bgcolor[0],setting.bgcolor[1],setting.bgcolor[2],setting.bgcolor[2]); //set to default (black)

    //set_bg_color(0.0,0.0,0.0,0.0); //set to default (black)
    //set_bg_color(bgColors[0][0],bgColors[0][1],bgColors[0][2],bgColors[0][3]); //set to default (light blue [0.0, 0.2, 0.4, 0.0])
  
    //graphic[1]=1; //transp
    //graphic[2]=1; //persp
    //cut_angle=0; //degrees
    //phi_projection=false;
    //projection=false;
  
    //trans_value=0.8;



    char hex[]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
    int tmp[6];
  
    int i;
    for(i=1;i<argc ; i++){
  
      if(!strcmp( argv[i] , "-world_size" ) ) {
        float w_size = atof(  argv[++i] )  ;
        printf( "  setting world size to  %f " , w_size ) ;
        //set_world_size( w_size ) ;
        mm.sf = 205.0/w_size;
      } else if(!strcmp(argv[i], "-bgcolor") && i < argc-1){
        i++;
        if (!strcmp(argv[i],"Black") || !strcmp(argv[i],"black")){
          printf("Set background color to black.\n");
          set_bg_color(0.0,0.0,0.0,0.0); //Black
        } else if (!strcmp(argv[i],"Blue") || !strcmp(argv[i],"blue")){
          printf("Set background color to blue.\n");
          set_bg_color(0.0,0.2,0.4,0.0); //Dark blue
        }else if (!strcmp(argv[i],"White") || !strcmp(argv[i],"white")){
          printf("Set background color to white.\n");
          set_bg_color(1.0,1.0,1.0,0.0); //White
        }else if((strlen(argv[i]) == 8 && argv[i][0] == '0' && toupper(argv[i][1]) == 'X') || strlen(argv[i]) == 6){
          printf("Set background to user defined color.\n");
          int n=0;
          if(strlen(argv[i]) == 8){
              n=2;
          }
          int k;
          for(k=0;k<6;k++){
              int j;
              tmp[k]=999;
              for(j=0;j<16;j++){
                  if(toupper(argv[i][k+n]) == hex[j]){
                      tmp[k]=j;
                  }
  
              }
              if(tmp[k]==999){
                  printf("Unknown digit '%c'!\nSet background color to default value.\n",argv[i+1][k+n]);
                  break;
              }
              if(k==5){
                  userDefinedBGColor[0] = (tmp[0]*16 + tmp[1])/255.0;
                  userDefinedBGColor[1] = (tmp[2]*16 + tmp[3])/255.0;
                  userDefinedBGColor[2] = (tmp[4]*16 + tmp[5])/255.0;
                  userDefinedBGColor[3] = 0.0;
              
                  printf("set color to: %f/%f/%f\n",(tmp[0]*16 + tmp[1])/255.0, (tmp[2]*16 + tmp[3])/255.0, (tmp[4]*16 + tmp[5])/255.0); 
                  set_bg_color((tmp[0]*16 + tmp[1])/255.0,(tmp[2]*16 + tmp[3])/255.0,(tmp[4]*16 + tmp[5])/255.0,0.0);
              }
          }
        } else {
          printf("Unknown background color.\nPlease choose black/blue/white or a hexadecimal number with 6 digits!\nSet background color to default value.\n");
        }
      
      } else if(!strcmp( argv[i] , "-h" ) || 
         !strcmp( argv[i] , "--help" )|| 
         !strcmp( argv[i] , "-?" )
         ) {


      printf( "\n  CED event display server: \n\n"
          "   Usage:  glced [-bgcolor COLOR] [-world_size LENGTH] [-geometry X_GEOMETRY] [-trust TRUSTED_HOST]\n\n" 
          "        options:  \n"
          "              COLOR:        Background color (values: black, white, blue or hexadecimal number)\n"
          "              LENGTH:       Visible world-cube size in mm (default: 6000) \n"
          "              X_GEOMETRY:   Window position and size in the form WxH+X+Y \n"
          "                              (W:width, H: height, X: x-offset, Y: y-offset) \n"
          "              TRUSTED_HOST: Ip or name of the host who is allowed to connect to CED\n\n"
          "   Example: \n\n"
          "     ./bin/glced -bgcolor 4C4C66 -world_size 1000. -geometry 600x600+500+0  -trust 192.168.11.22 > /tmp/glced.log 2>&1 & \n\n" 
          "    "
          "   Change port (before starting glced):"
              "         export CED_PORT=<portnumber>\n\n\n"
          "   To connect Marlin from a remote machine set variables CED_HOST=<this_host> and CED_PORT=<this_CED_PORT> on the machine where Marlin is started from\n\n"
          "   On this machine start glced with option: -trust <host_where_Marlin_is_started_from> to accept the connection from the remote host"
          "\n\n"
          ) ;

        exit(0) ;
      } else if(!strcmp(argv[i], "-trust")){
          i++;
          if(i >= argc){
              printf("wrong syntax!\n");
              exit(0);
          }

          //printf("test: %s %s\n",argv[i], argv[i+1]);
          struct hostent *host = gethostbyname(argv[i]);
          if (host != NULL){
  	    extern char trusted_hosts[50];

              snprintf(trusted_hosts, 50, "%u.%u.%u.%u",(unsigned char)host->h_addr[0] ,(unsigned char)host->h_addr[1] ,(unsigned char)host->h_addr[2] ,(unsigned char)host->h_addr[3]);
  
             printf("Trust ip: %s\n", trusted_hosts);
          } else{
              printf("ERROR: Host %s is unknown!\n", argv[i+1]);  
          }
      } else if(!strcmp(argv[i], "-geometry")){
        geometry = true;
      }else {
          //printf("ERROR: Unknown parameter %s\n Try %s -h for help\n", argv[i], argv[0]);
          //exit(1);
      }
    }
  
    ced_register_elements();
  
    char *p;
    p = getenv ( "CED_PORT" );
    if(p != NULL){
      printf("Try to use user defined port %s.\n", p);
      glut_tcp_server(atoi(p),input_data);
    }else{
      glut_tcp_server(7286,input_data);
    }
  
  


  
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH|GLUT_ALPHA);
    //glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    //glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
    //  glutInitWindowSize(600,600); // change to smaller window size */
    /*   glutInitWindowPosition(500,0); */
  
    //glutInitWindowSize(500,500);
    //cout << setting.win_w << " x " << setting.win_h << std::endl;


            //glutGameModeString("1280x1024:32@60");
            //glutEnterGameMode();
 
    if(geometry == false){
        glutInitWindowSize(setting.win_w,setting.win_h);
    }

    mainWindow=glutCreateWindow("C Event Display (CED)");

  
    //glHint (GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
    //glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    //glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    //glHint(GL_POLYGON_SMOOTH,GL_FASTEST); 
    //glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    //glEnable(GL_POLYGON_SMOOTH);
    glShadeModel(GL_SMOOTH);
  
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);



    //set_bg_color(setting.bgcolor[0],setting.bgcolor[1],setting.bgcolor[2],setting.bgcolor[2]); //set to default (black)
    //glClearColor(BG_COLOR[0],BG_COLOR[1], BG_COLOR[2], BG_COLOR[3]);
    init();


   

    #ifndef __APPLE__
    //glutMouseWheelFunc(mouseWheel); //dont works under mac os!
    #endif
  

  
    glutMouseFunc(mouse);
    glutDisplayFunc(display);
    if(setting.fps){
        glutIdleFunc(idle); //to show fps
    }

    glutReshapeFunc(reshape);
    glutKeyboardFunc(keypressed);
    glutSpecialFunc(SpecialKey);
    glutMotionFunc(motion);

  
    //glutTimerFunc(2000,time,23);
    //glutTimerFunc(500,timer,23);
  

    buildMenuPopup(); //hauke
    glutAttachMenu(GLUT_RIGHT_BUTTON); 

  
    for(i=0;i<NUMBER_POPUP_LAYER;i++){ //fill the layer section
      updateLayerEntryInPopupMenu(i);
    }


    for(i=NUMBER_DATA_LAYER;i<NUMBER_DETECTOR_LAYER+NUMBER_DATA_LAYER;i++){ //fill the layer section
      updateLayerEntryDetector(i);
    }



  

    glutTimerFunc(500,timer,1);
  

//    glDisable(GL_BLEND);




    glutMainLoop();
    return 0;
}

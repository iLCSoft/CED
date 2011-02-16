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

//#include "glced.h"
//#include "ced_srv.h"

#define GRAFIC_HIGH             2000            
#define GRAFIC_LOW              2001
#define GRAFIC_PERSP            2002
#define GRAFIC_BUFFER           2003
#define GRAFIC_TRANS            2004
#define GRAFIC_LIGHT            2005

#define CUT_ANGLE0              2100
#define CUT_ANGLE30             2101
#define CUT_ANGLE90             2102
#define CUT_ANGLE180            2103
#define CUT_ANGLE270            2104
#define CUT_ANGLE360            2105


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

extern int graphic[];  //= {0,0,0}; //light, transparence, perspective
extern double cut_angle;

int ced_picking(int x,int y,GLfloat *wx,GLfloat *wy,GLfloat *wz); //from ced_srv.c, need header files!

static char layerDescription[MAX_LAYER][MAX_LAYER_CHAR]; 
const char layer_keys[] = {'0','1', '2','3','4','5','6','7','8','9',')', '!', '@', '#', '$', '%', '^', '&', '*', '(', 't', 'y', 'u', 'i', 'o'};

static int mainWindow=-1;
static int subWindow=-1;
static int layerMenu;

//static int helpWindow=-1;
static int showHelp=0;

#define DEFAULT_WORLD_SIZE 1000.  //SJA:FIXED Reduce world size to give better scale

static float WORLD_SIZE;
static float FISHEYE_WORLD_SIZE;
//static float BACKUP_WORLD_SIZE;

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
extern unsigned ced_visible_layers; 


// The size of initialy visible world (+-)
//#define WORLD_SIZE 6000.

static struct _geoCylinder {
  GLuint obj;
  GLfloat d;       // radius
  GLfloat ir;      // inner radius
  GLuint  sides;   // poligon order
  GLfloat rotate;  // angle degree
  GLfloat z;       // 1/2 length
  GLfloat shift;   // in z
  GLfloat r;       // R 
  GLfloat g;       // G  color
  GLfloat b;       // B
} /*geoCylinder[] = { //hauke test, with inner radius (NOT THE REAL INNER RADIUS!!!)
  { 0,   50.0,  49.0,   6,  0.0, 5658.5, -5658.5, 0.0, 0.0, 1.0 }, // beam tube
  { 0,  380.0,  51.0,  24,  0.0, 2658.5, -2658.5, 0.0, 0.0, 1.0 }, // inner TPC
  { 0, 1840.0, 381.0,   8, 22.5, 2700.0, -2700.0, 0.5, 0.5, 0.1 }, // inner ECAL
  { 0, 3000.0, 2046.0, 16,  0.0, 2658.5, -2658.5, 0.0, 0.8, 0.0 }, // outer HCAL
  { 0, 2045.7,1841.0,   8, 22.5, 2700.0, -2700.0, 0.5, 0.5, 0.1 }, // outer ECAL
  { 0, 3000.0,  51,     8, 22.5, 702.25,  2826.0, 0.0, 0.8, 0.0 }, // endcap HCAL
  { 0, 2045.7,  51,     8, 22.5, 101.00,  2820.0, 0.5, 0.5, 0.1 }, // endcap ECAL
  { 0, 3000.0,  51,     8, 22.5, 702.25, -4230.5, 0.0, 0.8, 0.0 }, // endcap HCAL
  { 0, 2045.7,  51,     8, 22.5, 101.00, -3022.0, 0.5, 0.5, 0.1 }, // endcap ECAL

*/
geoCylinder[] = {
  { 0,   50.0,  6,  0.0, 5658.5, -5658.5, 0.0, 0.0, 1.0 }, // beam tube
  { 0,  380.0, 24,  0.0, 2658.5, -2658.5, 0.0, 0.0, 1.0 }, // inner TPC
  { 0, 1840.0,  8, 22.5, 2700.0, -2700.0, 0.5, 0.5, 0.1 }, // inner ECAL
  { 0, 3000.0, 16,  0.0, 2658.5, -2658.5, 0.0, 0.8, 0.0 }, // outer HCAL
  { 0, 2045.7,  8, 22.5, 2700.0, -2700.0, 0.5, 0.5, 0.1 }, // outer ECAL
  { 0, 3000.0,  8, 22.5, 702.25,  2826.0, 0.0, 0.8, 0.0 }, // endcap HCAL
  { 0, 2045.7,  8, 22.5, 101.00,  2820.0, 0.5, 0.5, 0.1 }, // endcap ECAL
  { 0, 3000.0,  8, 22.5, 702.25, -4230.5, 0.0, 0.8, 0.0 }, // endcap HCAL
  { 0, 2045.7,  8, 22.5, 101.00, -3022.0, 0.5, 0.5, 0.1 }, // endcap ECAL
 //original detector
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
  for(i=0;i<sizeof(geoCylinder)/sizeof(struct _geoCylinder);i++)
    geoCylinder[i].obj=makeCylinder(geoCylinder+i);
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
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


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
  0.2, //SJA:FIXED set redraw scale a lot smaller
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

  glColor3f(0.2,0.2,0.8);
  glLineWidth(2.);
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
  glColor3f(0.,0.,0.); //black labels
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

static void display(void){

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glPushMatrix();


  glRotatef(mm.va,1.,0.,0.);
  glRotatef(mm.ha,0.,1.0,0.);
  glScalef(mm.sf,mm.sf,mm.sf);
  glTranslatef(-mm.mv.x,-mm.mv.y,-mm.mv.z);

    //glMatrixMode(GL_MODELVIEW); //

  // draw static objects
  display_world(); //only axes?

  // draw elements (hits + detector)
  ced_prepare_objmap();
  ced_do_draw_event();

  glPopMatrix();

  glutSwapBuffers();
}

static void reshape(int w,int h){
  // printf("Reshaped: %dx%d\n",w,h);
  window_width=w;
  window_height=h;


  if(graphic[2] == 0){

  glViewport(0,0,w,h);
 

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();


  glOrtho(-WORLD_SIZE*w/h,WORLD_SIZE*w/h,-WORLD_SIZE,WORLD_SIZE,
	  -15*WORLD_SIZE,15*WORLD_SIZE);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity(); 
  } else{
  glViewport(0,0,w,h);

                glMatrixMode( GL_PROJECTION );
                glLoadIdentity();
                gluPerspective(60,window_width/window_height,100,10000);
                glMatrixMode( GL_MODELVIEW );

                glLoadIdentity();
    
                //glClearDepth(1.0);                  
                //glEnable(GL_DEPTH_TEST);            
                //glDepthFunc(GL_LEQUAL);             
                //glDepthFunc(GL_LESS);             

  
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
    glutReshapeWindow (window_width-10,window_height/4);
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
      move_mode=TURN_XY;
    }
    doubleClickTime=tv.tv_sec*1000000+tv.tv_usec;
    return;
  case GLUT_RIGHT_BUTTON:
    move_mode=ZOOM;
    return;
  case GLUT_MIDDLE_BUTTON:
    move_mode=ORIGIN;
    return;
  default:
    break;
  }


//hauke
if (btn == mouseWheelUp){

    selectFromMenu(VIEW_ZOOM_IN);
  //  mm.mv.z+=150./mm.sf;
  //  glutPostRedisplay();
    return;
}
if (btn == mouseWheelDown){
    selectFromMenu(VIEW_ZOOM_OUT);
    return;

  //  mm.mv.z-=150./mm.sf;
  //  glutPostRedisplay();
}

/*
if (btn == mouseWheelUp){
    //calice
    //mm.mv.z+=150./mm.sf;
    
    //hauke
    //mm.sf+=(100.)/window_height;
    selectFromMenu(VIEW_ZOOM_IN);

    //mm.sf += mm.sf*50./window_height;

    //if(mm.sf<0.2){ mm.sf=0.2; }
    //else if(mm.sf>20.){ mm.sf=20.; } 
    //glutPostRedisplay();
    return;
}

if (btn ==  mouseWheelDown){
    //calice
    //mm.mv.z-=150./mm.sf;

    // hauke
    selectFromMenu(VIEW_ZOOM_OUT);

    //mm.sf+=(-100.)/window_height;
    //mm.sf -= mm.sf*50.0/window_height;
    //if(mm.sf<0.2){ mm.sf=0.2; }
    //else if(mm.sf>20.){ mm.sf=20.; } 
    //glutPostRedisplay();
    return;
}
*/
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
int isLayerVisible(int x){
//    return(((1<<((x>>8)&0xff))&ced_visible_layers));
    //printBinaer(ced_visible_layers);
    //printBinaer(1<<(x+1));
    //printf("here: %i\n",(1<<(x+1))&ced_visible_layers);
    if( ((1<<(x))&ced_visible_layers) > 0){
        return(1);
    }else{
        return(0);
    }
}

static void toggle_layer(unsigned l){
  //printf("Toggle layer %u:\n",l);
  //printBinaer(ced_visible_layers);
  ced_visible_layers^=(1<<l);
  //  printf("Toggle Layer %u  and ced_visible_layers = %u \n",l,ced_visible_layers);  
  //printBinaer(ced_visible_layers);
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

  //printf("Key at %dx%d: %u('%c')\n",x,y,key,key);
  if(key=='r' || key=='R'){ 
      selectFromMenu(VIEW_RESET);
      //mm=mm_reset;
      //glutPostRedisplay();
  } else if(key=='f' || key=='F'){ 
      selectFromMenu(VIEW_FRONT);
      ////mm=mm_reset;
      //mm.ha=mm.va=0.;
      //glutPostRedisplay();
  } else if(key=='s' || key=='S'){
      selectFromMenu(VIEW_SIDE);
      ////mm=mm_reset;
      //mm.ha=90.;
      //mm.va=0.;
      //glutPostRedisplay();
  } else if(key==27) { //esc
      
      exit(0);
  }
  else if(key=='c' || key=='C'){
    //selectFromMenu(VIEW_CENTER);
    if(!ced_get_selected(x,y,&mm.mv.x,&mm.mv.y,&mm.mv.z)) glutPostRedisplay();
  } 
/*
  //hauke hoelbe: 08.02.2010
  else if(key=='p' || key=='P'){
    if(!ced_picking(x,y,&mm.mv.x,&mm.mv.y,&mm.mv.z))
      //hauke hoelbe: 08.02.2010
      //SM-H
      //TODO: Picking code: needs to work well with corresponding code in MarlinCED
      sock=__glutSockets ;
      //
      int id = SELECTED_ID;
      //printf(" ced_get_selected : socket connected: %d", sock->fd );	
     
      send( sock->fd , &id , sizeof(int) , 0 ) ;

  }
*/
  else if(key=='v' || key=='V'){
        selectFromMenu(VIEW_FISHEYE);
//
//    if(fisheye_alpha==0.0){
//                fisheye_alpha = 1e-3;
//        FISHEYE_WORLD_SIZE = WORLD_SIZE/(1.0+WORLD_SIZE*fisheye_alpha);
//        set_world_size(WORLD_SIZE);
//        set_world_size(FISHEYE_WORLD_SIZE);
//
//        printf("Fisheye view on\n");
//    }
//    else{
//        fisheye_alpha = 0.0;
//        set_world_size(FISHEYE_WORLD_SIZE);
//        set_world_size(WORLD_SIZE);
//        printf("Fisheye view off\n");
//    }
//    glutPostRedisplay();
  } else if((key>='0') && (key<='9')){
        selectFromMenu(LAYER_0+key-'0');
    //toggle_layer(key-'0');
    //glutPostRedisplay();
  } else if(key==')'){ // 0
        selectFromMenu(LAYER_10);
//    toggle_layer(10);
 //   glutPostRedisplay();
  } else if(key=='!'){ // 1
        selectFromMenu(LAYER_11);
//    toggle_layer(11);
//    glutPostRedisplay();
  } else if(key=='@'){ // 2
        selectFromMenu(LAYER_12);
    //toggle_layer(12);
    //glutPostRedisplay();
  } else if(key=='#'){ // 3
        selectFromMenu(LAYER_13);
    //toggle_layer(13);
    //glutPostRedisplay();
  } else if(key=='$'){ // 4
        selectFromMenu(LAYER_14);
    //toggle_layer(14);
    //glutPostRedisplay();
  } else if(key=='%'){ // 5
        selectFromMenu(LAYER_15);
    //toggle_layer(15);
    //glutPostRedisplay();
  } else if(key=='^'){ // 6
        selectFromMenu(LAYER_16);
    //toggle_layer(16);
    //glutPostRedisplay();
  } else if(key=='&'){ // 7
        selectFromMenu(LAYER_17);
    //toggle_layer(17);
    //glutPostRedisplay();
  } else if(key=='*'){ // 8
        selectFromMenu(LAYER_18);
    //toggle_layer(18);
    //glutPostRedisplay();
  } else if(key=='('){ // 9
        selectFromMenu(LAYER_19);
    //toggle_layer(19);
    //glutPostRedisplay();
  } else if(key=='`'){
        selectFromMenu(LAYER_ALL);
    //show_all_layers();
    //glutPostRedisplay();

  // SD: added layers
  } else if(key == '+'){
        selectFromMenu(VIEW_ZOOM_IN);
  } else if(key == '-'){
        selectFromMenu(VIEW_ZOOM_OUT);
  } else if(key == 'z'){
        mm.mv.z+=150./mm.sf;
        glutPostRedisplay();
  } else if(key == 'Z'){
        mm.mv.z-=150./mm.sf;
        glutPostRedisplay();
  }


  else if(key=='t'){ // t - momentum at ip layer 2
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
  }


  else if(key=='b'){ // toggle background color
    ++iBGcolor;
    if (iBGcolor >= sizeof(bgColors)/sizeof(color_t)){
        glClearColor(userDefinedBGColor[0],userDefinedBGColor[1],userDefinedBGColor[2],userDefinedBGColor[3]);
        iBGcolor=-1;
        printf("using color: %s\n","user defined");
        glutPostRedisplay();
        return;
    }else{
    // else if (iBGcolor > sizeof(bgColors)/sizeof(color_t)){
    //    iBGcolor = 0;
    //}
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


//static void ppoint(Point *p){
//  printf("= %f %f %f\n",p->x,p->y,p->z);
//}

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
      if(mm.sf<0.2)
	  mm.sf=0.2;
      else if(mm.sf>20.)
	  mm.sf=20.;
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
      glutTimerFunc(50,timer,1);
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
          glutTimerFunc(50,timer,01);

	      return ; /* to avoid complexity with removed sockets */
	    }
	}
    }

  //fix for old glut version
  glutSetWindow(mainWindow); 

  glutTimerFunc(200,timer,01);

  return;

}



int glut_tcp_server(unsigned short port,
		    void (*user_func)(void *data));

static void input_data(void *data){
  if(ced_process_input(data)>0)
    glutPostRedisplay();
}


//http://www.linuxfocus.org/English/March1998/article29.html
void drawString (char *s){
  unsigned int i;
  for (i = 0; i[s]; i++)
    glutBitmapCharacter (GLUT_BITMAP_HELVETICA_10, s[i]);
    //glutBitmapCharacter (GLUT_BITMAP_9_BY_15, s[i]);

}

void drawStringBig (char *s){
  unsigned int i;
  for (i = 0; i[s]; i++)
    glutBitmapCharacter (GLUT_BITMAP_HELVETICA_18, s[i]);
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
    glClearColor(0.5, 0.5, 0.5, 100);

    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float line = 45/window_height; //height of one line
    //float column = 200/window_width;
    float column = 200/window_width; //width of one line

    const int ITEMS_PER_COLUMN=window_height/60; //how many lines per column?
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

    for(i=0;i<sizeof(shortcuts)/sizeof(char *);i++){
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

    for(i=0;i<MAX_LAYER;i++){
/*        if(i==0){
            drawHelpString("Layers:", ((int)(aline/ITEMS_PER_COLUMN)+actual_column)*column,(ITEMS_PER_COLUMN-(aline%ITEMS_PER_COLUMN))*line);
            aline++;
            continue;
        }     
*/
       //if(((i/ITEMS_PER_COLUMN)+1) > MAX_COLUMN) break;
        
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


/*
    //write topic
    glColor3f(1.0, 1.0, 1.0);
    sprintf (label, "Shortcuts");
    glRasterPos2f(0.40F, 0.80F);
    drawStringBig(label);

*/
    glutSwapBuffers ();
}
void subReshape (int w, int h)
{
    //int border=1;
    //glutPositionWindow(w,h);
    //glutReshapeWindow(window_width,window_height);
    
    // set the projection matrix for the current window

  glViewport (0, 0, w, h);
  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  gluOrtho2D (0.0F, 1.0F, 0.0F, 1.0F);
};
void writeString(char *str,int x,int y){
    int i;
   // glPushMatrix();
    //glTranslatef(x, y, 0);
    //glColor3f(1.0, 0.0, 0.0);//print timer in red
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
        //printf("Hide help subwindow\n");
        glutDestroyWindow(subWindow);
        //glutSetWindow(subWindow);
        //glutHideWindow();
        showHelp=0;
    }else if(showHelp == 0){
        //helpWindow = glutCreateWindow("Help",);
        //printf("create help subwindow\n");

        //double height=window_height/4>150?150:window_height/4;
        //if(height<100) height=window_height;
        //printf("height = %f\n", height);
        //subWindow=glutCreateSubWindow(mainWindow,5,5,window_width-10,height);

        subWindow=glutCreateSubWindow(mainWindow,5,5,window_width-10,window_height/4);

        glutDisplayFunc(subDisplay);
        glutReshapeFunc(subReshape);


        //glutMotionFunc(motion);
        //glutSetWindow(subWindow);
            
        glutKeyboardFunc(keypressed);
        glutSpecialFunc(SpecialKey);

        //buildMenuPopup(); //hauke
        //glutAttachMenu(GLUT_RIGHT_BUTTON); 
        //int i;
        //for(i=0;i<20;i++){ //fill the layer section
        //    updateLayerEntryInPopupMenu(i);
        //}


        glutPostRedisplay();

        glutSetWindow(mainWindow);
        showHelp=1;
    } /*else if(showHelp == 0) {
        printf("show help subwindow\n");
        //glutDestroyWindow(subWindow);
        glutSetWindow(subWindow);
        glutShowWindow();
        showHelp=1;
    } */
  // else if(showHelp){
  //      printf("hide help subwindow\n");
  //      glutSetWindow(subWindow);
  //      glutHideWindow();
  //      showHelp=0;
  //  }else{
  //      printf("show help subwindow\n");
  //      glutSetWindow(subWindow);
  //      glutShowWindow();
  //      showHelp=1;
  //  }
    glutSetWindow(mainWindow);
    //glutHideWindow();
    //glutShowWindow();

}

void updateLayerEntryInPopupMenu(int id){ //id is layer id, not menu id!
    char string[200];
    char tmp[41];
    if(id < 0 || id > MAX_LAYER_POPUP-1){
        return;
    }
    strncpy(tmp, layerDescription[id], 40); 
    tmp[40]=0;
    
    sprintf(string,"[%s] Layer %s%i [%c]: %s%s",isLayerVisible(id)?"X":"   ", (id < 10)?"0":"" ,id, layer_keys[id], tmp, (strlen(layerDescription[id]) > 40)?"...":"");
    glutSetMenu(layerMenu);
    glutChangeToMenuEntry(id+2,string, id+LAYER_0);                     
}

void addLayerDescriptionToMenu(int id, char * str){
    //char string[200];
    //if(id == -1){ //reset all
    //    int i=0;
    //    for(i=0;i<25;i++){
    //        strcpy(layerDescription[i], "");
    //        sprintf(string,"[%s] Layer %s%i [%c]: %s",isLayerVisible(id)?"X":"   ", (i < 10)?"0":"" ,i, layer_keys[i], layerDescription[i]);
    //        glutSetMenu(layerMenu);
    //        glutChangeToMenuEntry(i+2,string, i+LAYER_0);                     
    //    }
    //    return;
    //}

    if(id < 0 || id >= MAX_LAYER){
        printf("Warning: Layer id out of range\n");
        return;
    }
    strncpy(layerDescription[id], str,MAX_LAYER_CHAR-1);
//--------
    //sprintf(string,"[%s] Layer %s%i [%c]: %s",isLayerVisible(id)?"X":"   ", (id < 10)?"0":"" ,id, layer_keys[id], layerDescription[id]);
    //glutSetMenu(layerMenu);
    //glutChangeToMenuEntry(id+2,string, id+LAYER_0);                     
    updateLayerEntryInPopupMenu(id);
 
    //printf("layerDescription: layer %i text: %s\n", id, layerDescription[id]);
}
void selectFromMenu(int id){ //hauke
    //char string[250];
    int i;
    int anz;
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
            mm=mm_reset;
            break;
        case VIEW_FISHEYE:
            if(fisheye_alpha==0.0){
                fisheye_alpha = 1e-3;
                 //FISHEYE_WORLD_SIZE = WORLD_SIZE/(1.0+WORLD_SIZE*fisheye_alpha); //<--old
                FISHEYE_WORLD_SIZE = WORLD_SIZE/(WORLD_SIZE*fisheye_alpha); //<-- new
                set_world_size(WORLD_SIZE); // <-- old
                //set_world_size(FISHEYE_WORLD_SIZE); //<-- new
                //mm.sf += 0.69; //<-- new
                //printf("Fisheye view on\n");
            }
            else{
                fisheye_alpha = 0.0;
                //BACKUP_WORLD_SIZE=WORLD_SIZE; //<-- new

                set_world_size(FISHEYE_WORLD_SIZE); //<-- old
                //set_world_size(BACKUP_WORLD_SIZE); //<-- new
                //mm.sf -= 0.69; //<-- new
                //printf("Fisheye view off\n");
            }
            break;

        case VIEW_FRONT:
            mm=mm_reset;
            mm.ha=mm.va=0.;
            break;
        case VIEW_SIDE:
            mm=mm_reset;
            mm.ha=90.;
            mm.va=0.;
            break;
        case VIEW_ZOOM_IN:
            mm.sf += mm.sf*50.0/window_height;
            if(mm.sf>50){ mm.sf=50; }
            break;
        case VIEW_ZOOM_OUT:
            mm.sf -= mm.sf*50.0/window_height;
            if(mm.sf<0.01){ mm.sf=0.01; }
            break;
        case VIEW_CENTER:
            //ced_get_selected(x,y,&mm.mv.x,&mm.mv.y,&mm.mv.z);
            break;
        case LAYER_ALL:
            glutSetMenu(layerMenu);
            anz=0;
            for(i=0;i<MAX_LAYER_POPUP;i++){ //try to turn all layers on
                if(!isLayerVisible(i)){
                   //sprintf(string,"[X] Layer %s%i [%c]: %s", (i < 10)?"0":"" ,i, layer_keys[i], layerDescription[i]);
                   //glutChangeToMenuEntry(i+2,string, LAYER_0+i);                     
                   toggle_layer(i);
                   updateLayerEntryInPopupMenu(i);
                   anz++;
                }
            }
            if(anz == 0){ //turn all layers off
                for(i=0;i<MAX_LAYER_POPUP;i++){
                   //sprintf(string,"[   ] Layer %s%i [%c]: %s",(i < 10)?"0":"" ,i, layer_keys[i], layerDescription[i]);
                   //glutChangeToMenuEntry(i+2,string, LAYER_0+i);                     
                   toggle_layer(i);
                   updateLayerEntryInPopupMenu(id);
                }
            }
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
                //printf("Toggle layer: %i\n", id-LAYER_0);
                //sprintf(string,"[%s] Layer %s%i [%c]: %s",isLayerVisible(id-LAYER_0)?"X":"   ", (id-LAYER_0 < 10)?"0":"" ,id-LAYER_0, layer_keys[id-LAYER_0], layerDescription[id-LAYER_0]);


                //glutChangeToMenuEntry(id-LAYER_0+2,string, id);                     
                updateLayerEntryInPopupMenu(id-LAYER_0);

                //int i;
                //for(i=0;i<20;i++){
                //    printf("layer %i description: %s\n", i, layerDescription[i]);
                //}
                break;
        case CUT_ANGLE0:
            cut_angle=0; 
            break;

        case CUT_ANGLE30:
            cut_angle=30; 
            break;
        case CUT_ANGLE90:
             cut_angle=90;
             break;

        case CUT_ANGLE180:
             cut_angle=180;
             break;

        case CUT_ANGLE270:
             cut_angle=270;
             break;

        case CUT_ANGLE360:
             cut_angle=360;
             break;


        case GRAFIC_HIGH:
            graphic[0] = 0; //todo: little bit dirty, make it better
            graphic[1] = 0;
            graphic[2] = 0;
            selectFromMenu(GRAFIC_TRANS);
            selectFromMenu(GRAFIC_LIGHT);
            selectFromMenu(GRAFIC_PERSP);
            break;
            
        case GRAFIC_LOW:
            graphic[0] = 1; //todo: little bit dirty, make it better
            graphic[1] = 1;
            graphic[2] = 1;
            selectFromMenu(GRAFIC_TRANS);
            selectFromMenu(GRAFIC_LIGHT);
            selectFromMenu(GRAFIC_PERSP);
            break;

        case GRAFIC_TRANS:
            if(graphic[1] == 1){
                printf("Transparency  is now off\n");
                graphic[1] = 0;
            }else{
                printf("Transparency  is now on\n");
                graphic[1] = 1;

            }

            break;
            
        case GRAFIC_LIGHT:
            if(graphic[0] == 1){
                printf("Light  is now on\n");
                graphic[0] = 0;
                glDisable(GL_LIGHTING); 
            }else{
                printf("Light is now on\n");
                 graphic[0] = 1;
                 //TODO: CHANGE IT
                 GLfloat mat_specular[] = {1.0, 1.0, 1.0, 1.0};
                 GLfloat mat_shininess[] = {50.0};
                 GLfloat light_position[] = {1000, 1000, 0, 0};

                 GLfloat lightAmbient[]= {0.5, 0.5, 0.5, 1};     
                 //glLightfv(GL_LIGHT0, GL_DIFFUSE, lightAmbient);
                 glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);

                 glLightfv(GL_LIGHT0, GL_POSITION, light_position);
                 //glClearColor (0.0, 0.0, 0.0, 0.0);
                 glShadeModel (GL_SMOOTH);

                 //glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
                 //glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

                 glColorMaterial ( GL_FRONT_AND_BACK, GL_EMISSION ) ;
                 glColorMaterial (GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE) ;
                 glEnable (GL_COLOR_MATERIAL) ;

                 glEnable(GL_LIGHTING); 

                 glEnable(GL_LIGHT0);
                 glMatrixMode(GL_MODELVIEW);
                 //glEnable(GL_DEPTH_TEST);
            }
            break;

        case GRAFIC_PERSP:
            if(graphic[2] == 1){
                printf("Perspective is now flat\n");
                graphic[2] = 0;
                reshape(window_width, window_height); //hack, call resize function to overwrite perspectivic settings
/*
                //gluLookAt(0,0,2000,    0,0,0,    0,1,0);


                //gluLookAt(0,0,100,    0,0,0,    0,1,0);
                //glLoadIdentity ();
  glViewport(0,0,window_width, window_height);

                //gluOrtho2D (0.0F, 1.0F, 0.0F, 1.0F);
                //glMatrixMode( GL_MODELVIEW );
                //glDisable(GL_DEPTH_TEST);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();


//glMatrixMode(GL_PROJECTION);
                glOrtho(-WORLD_SIZE* window_width/window_height,WORLD_SIZE* window_width/window_height,-WORLD_SIZE,WORLD_SIZE, -15*WORLD_SIZE,15*WORLD_SIZE);
//glOrtho(0.0f, windowWidth, windowHeight, 0.0f, 0.0f, 1.0f);

                      //glOrtho(0,WORLD_SIZE* window_width/window_height,-WORLD_SIZE,WORLD_SIZE, -15*WORLD_SIZE,15*WORLD_SIZE);


*/
            }else{
                printf("Perspective is now 3d\n");
                graphic[2] = 1;


                //printf("Change Perspective\n");
    
                //gluPerspective(fovy, aspect, zNear, zFar : glDouble);
                //gluPerspective(0.5, 1, 0.1, 5);
                //glClearColor( 0.0, 0.0, 0.0, 0.0 );
    
                //glViewport( 0, 0, 600, 600 );
    /*
                 gluLookAt(kameraX, kameraY, kameraZ, // hier stehe ich 
                  szeneX,  szeneY,  szeneZ,  // hier schaue ich hin
                  obenX,   obenY,   obenZ);  // dieser Vektor zeigt senkrecht nach oben
    */
                //gluLookAt(0,0,1000,  0,0,100,    0,1,0);
    
               


       //glDepthMask(GL_FALSE); 

                glMatrixMode( GL_PROJECTION );
                glLoadIdentity();
                gluPerspective(60,window_width/window_height,100,10000);
                glMatrixMode( GL_MODELVIEW );

                //glLoadIdentity();
    
                glClearDepth(1.0);                  
                glEnable(GL_DEPTH_TEST);            
                //glDepthFunc(GL_LEQUAL);             
glDepthFunc(GL_LESS);             

  
       //glDepthMask(GL_TRUE);

  //glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA); 
  //glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
  glBlendFunc(GL_ONE, GL_ZERO);
  glEnable(GL_BLEND);
    
  
               gluLookAt  (0,0,2000,    0,0,0,    0,1,0);
            }
            break;
        case HELP:
            toggleHelpWindow();
            break;

    }
    glutPostRedisplay();
    //printf("selected id %i\n",id); 
}

int buildMenuPopup(void){ //hauke
    int menu;
    int subMenu1;
    int subMenu2;
    int subMenu3;
    int subMenu4;
    int subsubMenu1;
    int subsubMenu2;



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
    glutAddMenuEntry("Fisheye [v]",VIEW_FISHEYE);
    glutAddMenuEntry("Zoom in [+]", VIEW_ZOOM_IN);
    glutAddMenuEntry("Zoom out [-]", VIEW_ZOOM_OUT);
    //glutAddMenuEntry("Center [c]", VIEW_CENTER);


    subMenu3 = glutCreateMenu(selectFromMenu);
    layerMenu=subMenu3;

  
    glutAddMenuEntry("Show all Layers [`]", LAYER_ALL);

    int i;
    //char string[100];

    for(i=0;i<MAX_LAYER_POPUP;i++){
        //sprintf(string,"[%s] Layer %s%i [%c]: %s",isLayerVisible(i)?"X":"   ", (i < 10)?"0":"" ,i, layer_keys[i], layerDescription[i]);
        glutAddMenuEntry(" ",LAYER_0+i);
        //updateLayerEntryInPopupMenu(LAYER_0+i);
    }


    subsubMenu1 = glutCreateMenu(selectFromMenu);
    glutAddMenuEntry("Perspective",GRAFIC_PERSP);
    //glutAddMenuEntry("Deepbuffer", GRAFIC_BUFFER);
    glutAddMenuEntry("Transparency", GRAFIC_TRANS);
    glutAddMenuEntry("Light", GRAFIC_LIGHT);

    subsubMenu2 = glutCreateMenu(selectFromMenu);
    glutAddMenuEntry("0",  CUT_ANGLE0);
    glutAddMenuEntry("30", CUT_ANGLE30);
    glutAddMenuEntry("90", CUT_ANGLE90);
    glutAddMenuEntry("180", CUT_ANGLE180);
    glutAddMenuEntry("270", CUT_ANGLE270);
    glutAddMenuEntry("360", CUT_ANGLE360);






    subMenu4 = glutCreateMenu(selectFromMenu);
    glutAddMenuEntry("Classic View",GRAFIC_LOW);
    glutAddMenuEntry("New View", GRAFIC_HIGH);
    glutAddSubMenu("Graphic details", subsubMenu1);
    glutAddSubMenu("Cut angle", subsubMenu2);




    menu=glutCreateMenu(selectFromMenu);
    glutAddSubMenu("View", subMenu2);
    glutAddSubMenu("Layers", subMenu3);
    glutAddSubMenu("Background Color", subMenu1);
    glutAddSubMenu("Graphics options", subMenu4);
    glutAddMenuEntry("Toggle help [h]",HELP);

    return menu;
}

 int main(int argc,char *argv[]){

  WORLD_SIZE = DEFAULT_WORLD_SIZE ;
  //set_bg_color(0.0,0.0,0.0,0.0); //set to default (black)
  set_bg_color(bgColors[0][0],bgColors[0][1],bgColors[0][2],bgColors[0][3]); //set to default (light blue [0.0, 0.2, 0.4, 0.0])

  graphic[1]=1; //transp
  graphic[2]=1; //persp
  cut_angle=0;


  char hex[]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
  int tmp[6];

  int i;
  for(i=0;i<argc ; i++){

    if(!strcmp( argv[i] , "-world_size" ) ) {
      float w_size = atof(  argv[i+1] )  ;
      printf( "  setting world size to  %f " , w_size ) ;
      set_world_size( w_size ) ;
    }

    if(!strcmp(argv[i], "-bgcolor") && i < argc-1){
      if (!strcmp(argv[i+1],"Black") || !strcmp(argv[i+1],"black")){
        printf("Set background color to black.\n");
        set_bg_color(0.0,0.0,0.0,0.0); //Black
      } else if (!strcmp(argv[i+1],"Blue") || !strcmp(argv[i+1],"blue")){
        printf("Set background color to blue.\n");
        set_bg_color(0.0,0.2,0.4,0.0); //Dark blue
      }else if (!strcmp(argv[i+1],"White") || !strcmp(argv[i+1],"white")){
        printf("Set background color to white.\n");
        set_bg_color(1.0,1.0,1.0,0.0); //White
      }else if((strlen(argv[i+1]) == 8 && argv[i+1][0] == '0' && toupper(argv[i+1][1]) == 'X') || strlen(argv[i+1]) == 6){
        printf("Set background to user defined color.\n");
        int n=0;
        if(strlen(argv[i+1]) == 8){
            n=2;
        }
        int k;
        for(k=0;k<6;k++){
            int j;
            tmp[k]=999;
            for(j=0;j<16;j++){
                if(toupper(argv[i+1][k+n]) == hex[j]){
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
    
    }

    if(!strcmp( argv[i] , "-h" ) || 
       !strcmp( argv[i] , "--help" )|| 
       !strcmp( argv[i] , "-?" )
       ) {
      printf( "\n  CED event display server: \n\n" 
	      "   Usage:  glced [-bgcolor color] [-world_size length] [-geometry x_geometry] [--trust trusted_host]\n" 
	      "        where:  \n"
          "              - color:        Background color (values: black, white, blue or hexadecimal number)\n"
	      "              - length:       Visible world-cube size in mm (default: 6000) \n" 
	      "              - x_geometry:   Window position and size in the form WxH+X+Y \n" 
	      "                              (W:width, H: height, X: x-offset, Y: y-offset) \n"
          "              - trusted_host: Ip or name of the host who is allowed to connect to CED\n"  
	      "   Example: \n\n"
	      "     ./bin/glced -bgcolor 4C4C66 -world_size 1000. -geometry 600x600+500+0  --trust 192.168.11.22 > /tmp/glced.log 2>&1 & \n\n"  
	      "    "
	      "   Change port (before starting glced):"
              "         export CED_PORT=<portnumber>"
	      ) ;      
      exit(0) ;
    } if(!strcmp(argv[i], "--trust")){
        if(i+1 >= argc){
            printf("wrong syntax!\n");
            exit(0);
        }
        struct hostent *host = gethostbyname(argv[i+1]);
        if (host != NULL){
	    extern char trusted_hosts[50];
            snprintf(trusted_hosts, 50, "%u.%u.%u.%u",(unsigned char)host->h_addr[0] ,(unsigned char)host->h_addr[1] ,(unsigned char)host->h_addr[2] ,(unsigned char)host->h_addr[3]);

           //printf("trust ip %s\n", trusted_hosts);
        } else{
            printf("ERROR: Host %s is unknown!\n", argv[i+1]);  
        }
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

  glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glHint (GL_LINE_SMOOTH_HINT, GL_DONT_CARE);

      glEnable (GL_LINE_SMOOTH);


    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   glShadeModel(GL_SMOOTH);

 //glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
/*   glutInitWindowSize(600,600); // change to smaller window size */
/*   glutInitWindowPosition(500,0); */




  mainWindow=glutCreateWindow("C Event Display (CED)");


  init();
  mm_reset=mm;

  



#ifndef __APPLE__
  //glutMouseWheelFunc(mouseWheel); //dont works under mac os!
#endif


  glutMouseFunc(mouse);
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keypressed);
  glutSpecialFunc(SpecialKey);
  glutMotionFunc(motion);



      //glutTimerFunc(2000,time,23);
  //glutTimerFunc(500,timer,23);

  buildMenuPopup(); //hauke
  glutAttachMenu(GLUT_RIGHT_BUTTON); 

  for(i=0;i<MAX_LAYER_POPUP;i++){ //fill the layer section
    updateLayerEntryInPopupMenu(i);
  }


  glutTimerFunc(500,timer,1);




  glutMainLoop();

   return 0;
 }

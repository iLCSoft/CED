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
 * 2010 - 2012, H. Hoelbe:
 *                     - improved picking function 
 *                     - added main und popup menu
 *                     - added help menu
 *                     - added grafik features:
 *                         - detector in mesh or polygonal view
 *                         - perspectivic view
 *                         - detector cuts (phi and z)
 *                         - tranformations: front and side view
 *                         - background color
 *                         - transparency
 *                      - added features:
 *                         - screenshot 
 *                         - distance
 *                         - frames per secound
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
#include <ced_config.h>

#include <errno.h>
#include <sys/select.h>

#include <ctype.h> 
#include <sys/time.h>
#include <time.h>
#include <netdb.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <stdlib.h> 
#include <sys/stat.h>

static GLfloat window_width=0.;
static GLfloat window_height=0.;

#include <ced_menu.h>


#define DEFAULT_WORLD_SIZE 1000.  //SJA:FIXED Reduce world size to give better scale

using namespace std;


int ced_picking(int x,int y,GLfloat *wx,GLfloat *wy,GLfloat *wz); //from ced_srv.c, need header files!


//*************** global variables ***************************************//
//for new angles add the new angle to this list and to define in ced_menu.h
static int available_cutangles[]={0,30,45,90,100,135,120,150,170,180,190,200,220,240,260,270,280,290,310,330,340};
int last_selected_layer;
extern CEDsettings setting;
CEDsettings setting_old[5];
static char layerDescription[CED_MAX_LAYER][CED_MAX_LAYER_CHAR]; 
//const char layer_keys[] = {'0','1', '2','3','4','5','6','7','8','9',')', '!', '@', '#', '$', '%', '^', '&', '*', '(', 't', 'y', 'u', 'i', 'o'};
const char layer_keys[] = { DATALAYER_SHORTKEY_00, DATALAYER_SHORTKEY_01, DATALAYER_SHORTKEY_02, DATALAYER_SHORTKEY_03, DATALAYER_SHORTKEY_04, DATALAYER_SHORTKEY_05, DATALAYER_SHORTKEY_06, DATALAYER_SHORTKEY_07, DATALAYER_SHORTKEY_08, DATALAYER_SHORTKEY_09, DATALAYER_SHORTKEY_10, DATALAYER_SHORTKEY_11, DATALAYER_SHORTKEY_12, DATALAYER_SHORTKEY_13, DATALAYER_SHORTKEY_14, DATALAYER_SHORTKEY_15, DATALAYER_SHORTKEY_16, DATALAYER_SHORTKEY_17, DATALAYER_SHORTKEY_18, DATALAYER_SHORTKEY_19, DATALAYER_SHORTKEY_20, DATALAYER_SHORTKEY_21, DATALAYER_SHORTKEY_22, DATALAYER_SHORTKEY_23, DATALAYER_SHORTKEY_24};

//const char detec_layer_keys[] = {'t','y','u','i','o','p','[',']','\\', 'T', 'Y','U','I','O','P','{','}','|',' ',' ',' '};
const char detec_layer_keys[] = { DETECTORLAYER_SHORTKEY_00,   DETECTORLAYER_SHORTKEY_01,   DETECTORLAYER_SHORTKEY_02,   DETECTORLAYER_SHORTKEY_03,   DETECTORLAYER_SHORTKEY_04,   DETECTORLAYER_SHORTKEY_05,   DETECTORLAYER_SHORTKEY_06,   DETECTORLAYER_SHORTKEY_07,   DETECTORLAYER_SHORTKEY_08,   DETECTORLAYER_SHORTKEY_09,   DETECTORLAYER_SHORTKEY_10,   DETECTORLAYER_SHORTKEY_11,   DETECTORLAYER_SHORTKEY_12,   DETECTORLAYER_SHORTKEY_13,   DETECTORLAYER_SHORTKEY_14,   DETECTORLAYER_SHORTKEY_15,   DETECTORLAYER_SHORTKEY_16,   DETECTORLAYER_SHORTKEY_17,   DETECTORLAYER_SHORTKEY_18,   DETECTORLAYER_SHORTKEY_19};   

static int mainWindow=-1;
static int layerMenu;
static int detectorMenu;
static int subsubMenu2;
static int subscreenshot;
static int subSave;
static int subLoad;
static int showHelp=0;
static float WORLD_SIZE;
static float FISHEYE_WORLD_SIZE;
double fisheye_alpha = 0.0;
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

extern bool client_connected;

CED_SubSubMenu *detectorlayermenu;
CED_SubSubMenu *datalayermenu;
CED_PopUpMenu *popupmenu;
CED_Menu *ced_menu=NULL;

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


//************ function declarations ************************* //
void updateScreenshotMenu(void);
void screenshot(char *name, int times);
void buildLayerMenus(void);
void buildMainMenu(void);
void buildPopUpMenu(int x, int y);
void ced_prepare_objmap(void);
int ced_get_selected(int x,int y,GLfloat *wx,GLfloat *wy,GLfloat *wz);
int find_selected_object(int x,int y,GLfloat *wx,GLfloat *wy,GLfloat *wz, int *id, int *layer, int *type);


// ********** function definitions  (rest of file) ************************** //

//set background color (hauke)
static void set_bg_color(float one, float two, float three, float four){
    BG_COLOR[0]=one;
    BG_COLOR[1]=two;
    BG_COLOR[2]=three;
    BG_COLOR[3]=four;

    glClearColor(BG_COLOR[0],BG_COLOR[1],BG_COLOR[2],BG_COLOR[3]);
}

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

int isLayerVisible(int x){
    //return(ced_visible_layers[x]);

    return(setting.layer[x]);
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

//glEnable(GL_POLYGON_STIPPLE);


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

Point pick_point;
bool  select_nothing=true;


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


    glColor3f(AXES_COLOR);
    //glLineWidth(2.);
    glLineWidth(AXES_LINE_SIZE);

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
    //glTranslatef(mm.mv.x,mm.mv.y,mm.mv.z);
    glTranslatef(WORLD_SIZE/2.-WORLD_SIZE/100.,0.,0.);
    glRotatef(90.,0.0,1.0,0.0);
    axe_arrow();
    glPopMatrix();
  
    glPushMatrix();
    //glTranslatef(mm.mv.x,mm.mv.y,mm.mv.z);
    glTranslatef(0.,WORLD_SIZE/2.-WORLD_SIZE/100.,0.);
    glRotatef(-90.,1.0,0.,0.);
    axe_arrow();
    glPopMatrix();
  
  
    glPushMatrix();
    //glTranslatef(mm.mv.x,mm.mv.y,mm.mv.z);
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


    if(setting.fps == false){
        return;
    }


    gettimeofday(&tv, 0); 

    if(tv.tv_sec+tv.tv_usec/1000000.0-startTime < 1.0){
        fps++;
    }else{
        startTime=tv.tv_sec+tv.tv_usec/1000000.0;
        //printf("FPS: %i\n", fps);
        old_fps=fps;
        fps=1;
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

    sprintf(text, "FPS: %i", old_fps);

    glLoadIdentity();

    double dark=1.0-(setting.bgcolor[0]+setting.bgcolor[1]+setting.bgcolor[2])/3.0;
    glColor3f(dark,dark,dark);


    //void *font=GLUT_BITMAP_TIMES_ROMAN_10; //default font
    //glRasterPos2f(-1200,-950);
    //char *c;
    //for (c=text; *c != '\0'; c++) {
    //    glutBitmapCharacter(font, *c);
    //}
    
    drawHelpString(text,-1200,-950);

    glEnd();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}
void printShortcuts(void){

    const unsigned int MAX_STR_LEN=30;
    int i;
    
    int height=10;
    int width=2;
    if(setting.font==0){
        height=10+2;
        width=6;
    }
    if(setting.font==1){
        height=12+2;
        width=8;
    }
    if(setting.font==2){
        height=20+2;
        width=11;
    }


    //float line = 12; //height of one line
    //float column = MAX_STR_LEN*5; //width of one line

    float line = height; //height of one line
    float column = MAX_STR_LEN*width; //width of one line



    
    vector<string> shortcuts;
    shortcuts.push_back( "GENERAL SHORTCUTS:" );


    shortcuts.push_back( "[Esc] Quit CED" );
    shortcuts.push_back( "[h] Toggle shortcut frame" );
    shortcuts.push_back( "[r] Reset view" );
    shortcuts.push_back( "[R] Reset CED" );
    shortcuts.push_back( "[f] Font view" );
    shortcuts.push_back( "[s] Side view" );
    shortcuts.push_back( "[F] Front projection" );
    shortcuts.push_back( "[S] Side projection" );
    shortcuts.push_back( "[v] Fisheye projection" );
    shortcuts.push_back( "[b] Change background color" );
    shortcuts.push_back( "[+] Zoom in" );
    shortcuts.push_back( "[-] Zoom out" );
    shortcuts.push_back( "[c] Center" );
    shortcuts.push_back( "[Z] Cut in z-axe direction" );
    shortcuts.push_back( "[z] Cut in -z-axe direction" );
    shortcuts.push_back( "[>] Increase transparency" );
    shortcuts.push_back( "[<] Decrease transparency" );
    shortcuts.push_back( "[m] Increase detector cut angle" );
    shortcuts.push_back( "[m] Decrease detector cut angle" );
    shortcuts.push_back( "[->] Move in z-direction" );
    shortcuts.push_back( "[<-] Move in -z-direction" );
    shortcuts.push_back( "[`] Toggle all data layers" );
    shortcuts.push_back( "[~] Toggle all detector layers" );


    shortcuts.push_back( "  " );
    shortcuts.push_back( "DATA LAYERS:" );


    char label[MAX_STR_LEN+1];

    for(i=0;i<NUMBER_DATA_LAYER;i++){
        snprintf(label,MAX_STR_LEN+1, "(%s) [%c] %s%i: %s", isLayerVisible(i)?"X":"_",layer_keys[i], (i<10)?"0":"", i, layerDescription[i]);
        if(strlen(label) >= MAX_STR_LEN){
            label[MAX_STR_LEN-3]='.';
            label[MAX_STR_LEN-2]='.';
            label[MAX_STR_LEN-1]='.';
            label[MAX_STR_LEN]=0;
        }
        shortcuts.push_back(label);
    }
        
    shortcuts.push_back( " " );
    shortcuts.push_back( "DETECTOR LAYERS: " );

    for(i=NUMBER_DATA_LAYER;i<NUMBER_DETECTOR_LAYER+NUMBER_DATA_LAYER;i++){
        snprintf(label,MAX_STR_LEN+1, "(%s) [%c] %s%i: %s", isLayerVisible(i)?"X":"_",detec_layer_keys[-1*NUMBER_DATA_LAYER+i], ((i)<10)?"0":"", (i), layerDescription[i]);
        if(strlen(label) >= MAX_STR_LEN){
            label[MAX_STR_LEN-3]='.';
            label[MAX_STR_LEN-2]='.';
            label[MAX_STR_LEN-1]='.';
            label[MAX_STR_LEN]=0;
        }
        shortcuts.push_back(label);
    }

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();


    glMatrixMode(GL_PROJECTION);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);


    glLoadIdentity();

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    GLfloat w=glutGet(GLUT_WINDOW_WIDTH);
    GLfloat h=glutGet(GLUT_WINDOW_HEIGHT); ;

    int  WORLD_SIZE=1000; //static worldsize maybe will get problems in the future...

    //glOrtho(0,w,h, 0,0,15*WORLD_SIZE);

    //glOrtho(0,w,h,-10,0,15*WORLD_SIZE);

    glOrtho(0,w,h,-1*height,0,15*WORLD_SIZE);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    

    double border_factor_line=0.005;
    double border_factor_quad=0.0052;


    double boarder_quad = 1000*border_factor_quad;
    double boarder_line = 1000*border_factor_line;


    if(int(w/column) > 1){
        h=(boarder_quad*2.+(shortcuts.size()*1./int((w-3.*boarder_quad)/column) + 1.)*line)*3.+5;
    }else{
        h*=3;
    }

    //glColor4f(HELP_FRAME_FILL_COLOR);

    if((setting.bgcolor[0] + setting.bgcolor[1] + setting.bgcolor[2]) < 0.5*3){
        glColor4f(0.1,0.1,0.1,0.5);
    }else{
        glColor4f(0.9,0.9,0.9,0.5);
    }
   

    const int ITEMS_PER_COLUMN=int((h/3.0-boarder_quad*2)/(line)); //how many lines per column?
    glBegin(GL_QUADS); 
    glVertex3f(boarder_quad, boarder_quad,0);
    glVertex3f(w-boarder_quad,boarder_quad,0);
    glVertex3f(w-boarder_quad, h/3.-boarder_quad,0);
    glVertex3f(boarder_quad, h/3.-boarder_quad,0);
    glEnd();



    //glColor4f(HELP_FRAME_BOARDER_COLOR);
    if((setting.bgcolor[0] + setting.bgcolor[1] + setting.bgcolor[2]) < 0.5*3){
        glColor4f(0.2,0.2,0.2,0.5);
    }else{
        glColor4f(0.8,0.8,0.8,0.5);
    }
 
    glLineWidth(HELP_FRAME_BOARDER_LINE_SIZE);
    glBegin(GL_LINES); 
    glVertex3f(boarder_line, boarder_line,0);
    glVertex3f(w-boarder_line,boarder_line,0);


    glVertex3f(w-boarder_line, h/3-boarder_line,0);
    glVertex3f(boarder_line, h/3.-boarder_line,0);

    glVertex3f(boarder_line, boarder_line,0);
    glVertex3f(boarder_line, h/3. - boarder_line,0);

    glVertex3f(w-boarder_line,boarder_line,0);
    glVertex3f(w-boarder_line, h/3.-boarder_line,0);
    glEnd();

    //glColor3f(HELP_FRAME_TEXT_COLOR);
    if((setting.bgcolor[0] + setting.bgcolor[1] + setting.bgcolor[2]) < 0.5*3){
        glColor3f(1,1,1);
    }else{
        glColor3f(0,0,0);
    }
 

    


    for(i=0;(unsigned) i<shortcuts.size();i++){
       drawHelpString(shortcuts[i],  int(i/ITEMS_PER_COLUMN)*column+boarder_quad+5, (i%ITEMS_PER_COLUMN)*line+boarder_quad+10);
    }

    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

}

static void display(void){
 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
    glPushMatrix();
  
    
  
    // TODO: fix it! 
    // in case of no rotate, in some cases it could get strange 
    // lines in fisheye view from (0,0,0) to (-inf, -inf,x)

    setting.zoom=mm.sf;
    glScalef(mm.sf,mm.sf,mm.sf); //zoom

//#if ROTATE_MODE == 1
//        glTranslatef(-mm.mv.x,-mm.mv.y,-mm.mv.z); //move
//        glRotatef(mm.va,1.,0.,0.); //rotate
//        glRotatef(mm.ha,0.,1.0,0.); //rotate
//#else
//        glRotatef(mm.va,1.,0.,0.); //rotate
//        glRotatef(mm.ha,0.,1.0,0.); //rotate
//        glTranslatef(-mm.mv.x,-mm.mv.y,-mm.mv.z); //move
//#endif


        glRotatef(mm.va,1.,0.,0.); //rotate
        glRotatef(mm.ha,0.,1.0,0.); //rotate
        glTranslatef(-mm.mv.x,-mm.mv.y,-mm.mv.z); //move






    if(setting.picking_highlight==true && select_nothing == false){
        glColor3f(1,0,0);
        glPointSize(10);
        glBegin(GL_POINTS);
        //cout<< "point: " << pick_point.x << ", " << pick_point.y << ", " << pick_point.z << endl;
        glVertex3f(pick_point.x,pick_point.y,pick_point.z);
        glEnd();
    }

    
  
      //glMatrixMode(GL_MODELVIEW); //
  
    // draw static objects
    display_world(); //only axes?
  
  
     //glTranslatef(0,0,1000);
  
     //z cutting with clipping plane
     //const GLdouble clip_plane[]={0,0,-1,setting.z_cutting};
     //if(setting.z_cutting < 6999){
     //     glEnable(GL_CLIP_PLANE0);
     //}else{
     //     glDisable(GL_CLIP_PLANE0);
     //}
     //glClipPlane(GL_CLIP_PLANE0,clip_plane);
  
  
  
  
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


    if(showHelp == 1){
        printShortcuts();
    }





    glDisable(GL_LIGHTING);
    ced_menu->draw();
    popupmenu->draw();
    printFPS();

    if(setting.light==true){
        glEnable(GL_LIGHTING);
    }



  
    glutSwapBuffers();

    glPopMatrix();
}

static void write_world_into_front_buffer(void){
    glMatrixMode(GL_PROJECTION);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glRotatef(mm.va,1.,0.,0.);
    glRotatef(mm.ha,0.,1.0,0.);
    glScalef(mm.sf,mm.sf,mm.sf); //streech the world
  
      //glMatrixMode(GL_MODELVIEW); //
  
    // draw static objects

    glMatrixMode(GL_MODELVIEW);

    //glTranslatef(-mm.mv.x,-mm.mv.y,-mm.mv.z);

    display_world(); 


    glTranslatef(-mm.mv.x,-mm.mv.y,-mm.mv.z);


   //glTranslatef(0,0,1000);

     //const GLdouble clip_plane[]={0,0,-1,setting.z_cutting};
     //if(setting.z_cutting < 6999){
     //     glEnable(GL_CLIP_PLANE0);
     //}else{
     //     glDisable(GL_CLIP_PLANE0);
     //}
     //glClipPlane(GL_CLIP_PLANE0,clip_plane);
  
  
  
  
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

    if(setting.light==true){
        glEnable(GL_LIGHTING);
    }


    //printFPS();
}

//void drawStringBig (char *s){
//    unsigned int i;
//    for (i = 0; i[s]; i++){
//        glutBitmapCharacter (GLUT_BITMAP_HELVETICA_18, s[i]);
//    }
//}

//void drawHelpString (const string & str, float x,float y){ //format help strings strings: "[<key>] <description>"
//    unsigned int i;
//    glRasterPos2f(x,y);
//  
//    int monospace = 0;
//    for (i = 0; str[i]; i++){
//        if(str[i] == '['){ 
//            monospace = 1;
//            if(setting.font == 0){
//                glutBitmapCharacter (GLUT_BITMAP_HELVETICA_10, '[');
//            }else if(setting.font == 1){
//                glutBitmapCharacter (GLUT_BITMAP_HELVETICA_12, '[');
//            }else if(setting.font == 2){
//                glutBitmapCharacter (GLUT_BITMAP_HELVETICA_18, '[');
//            }
//            i++;
//        }
//        else if(str[i] == ']'){
//             monospace = 0;
//        }
//        if(monospace){
//            if(setting.font == 0){
//                glutBitmapCharacter(GLUT_BITMAP_8_BY_13, str[i]);
//            }else if(setting.font == 1){
//                glutBitmapCharacter(GLUT_BITMAP_8_BY_13, str[i]);
//            }else if(setting.font == 2){
//                glutBitmapCharacter(GLUT_BITMAP_9_BY_15, str[i]);
//            }
//        }else{
//            //glutBitmapCharacter (GLUT_BITMAP_HELVETICA_10, str[i]);
//            //glutBitmapCharacter ( GLUT_BITMAP_HELVETICA_12 , str[i]);
//            //glutBitmapCharacter ( GLUT_BITMAP_HELVETICA_18 , str[i]);
//            if(setting.font == 0){
//                glutBitmapCharacter (GLUT_BITMAP_HELVETICA_10, str[i]);
//            }else if(setting.font == 1){
//                glutBitmapCharacter (GLUT_BITMAP_HELVETICA_12, str[i]);
//            }else if(setting.font == 2){
//                glutBitmapCharacter (GLUT_BITMAP_HELVETICA_18, str[i]);
//            }
//        }
//    }
//}




static void reshape(int w,int h){
    // printf("Reshaped: %dx%d\n",w,h);
    window_width=w;
    window_height=h;
    setting.win_w=w;
    setting.win_h=h;
 
  
  
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

        //gluPerspective(45,window_width/window_height,100.0,50000.0*mm.sf+50000/mm.sf);
        gluPerspective(CAMERA_FIELD_OF_VIEW,window_width/window_height,CAMERA_MIN_DISTANCE,CAMERA_MAX_DISTANCE);


        //gluPerspective(170,window_width/window_height,100.0,50000.0*mm.sf+50000/mm.sf);


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
      
        gluLookAt  (CAMERA_POSITION,    0,0,0,    0,1,0);
    }
  
  
 //   //hauke
 //   if(showHelp == 1){
 //       glutSetWindow (subWindow);
 //       glutReshapeWindow (int(window_width-10),int(window_height/4));
 //   }



    //buildMainMenu(); 
    buildLayerMenus();
    //updateScreenshotMenu();
}

void saveSettings(int slot){
    ofstream file;
    const char *home = getenv("HOME");
    char filename[1000];
    char dirname[1000];

    snprintf(dirname, 1000, "%s/.glced_cfg/", home);
    //if(exists){
       mkdir(dirname,700); 
    //}
    snprintf(filename, 1000, "%s/.glced_cfg/settings%i", home, slot);

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
        //file<<"#Cut angle:"<<std::endl<<setting.cut_angle<< std::endl;
        //file<<"#Trans value:"<<std::endl<<setting.trans_value<< std::endl;
        //for(int i=0;i<CED_MAX_LAYER;i++){
        //    file<<"#Visibility Layer " << i << ":" <<std::endl<<setting.layer[i]<< std::endl;
        //}
        file<<"#Phi projection:"<<std::endl<<setting.phi_projection<< std::endl;
        file<<"#Z projection:"<<std::endl<<setting.z_projection<< std::endl;
        for(int i=0;i<3;i++){
            file<<"#View setting" << i << ":" <<std::endl<<setting.view[i] << std::endl;
        }
        file<<"#Vertical angle:"<<std::endl<<setting.va<< std::endl;
        file<<"#Horiz angle:"<<std::endl<<setting.ha<< std::endl;
        file<<"#Fixed view:"<<std::endl<<setting.fixed_view<< std::endl;
        //file<<"#Z cutting:"<<std::endl<<setting.z_cutting<< std::endl;
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

        file<<"#User interface font size"<<std::endl<<setting.font << std::endl;


        for(int i=0;i<CED_MAX_LAYER;i++){
            file<<"#Visibility of data layer " << i << std::endl;
            file<< setting.layer[i] << std::endl;
        }

        for(int i=0;i<NUMBER_DETECTOR_LAYER;i++){
            file<<"#Transparency value of detector layer: " << i << std::endl;
            file << setting.detector_trans[i] << std::endl;

            file<<"#Cut angle of detector layer: " << i << std::endl;
            file << setting.detector_cut_angle[i] << std::endl;

            file<<"#Cut z-value of detector layer: " << i << std::endl;
            file << setting.detector_cut_z[i] << std::endl;
        }

        file<<"#Enable detector picking:"<<std::endl<<setting.detector_picking<< std::endl;

        file<<"#Position:"<<std::endl<<
                mm.mv.x<< std::endl <<
                mm.mv.y<< std::endl << 
                mm.mv.z<< std::endl;


        file<<"#Picking marker:"<<std::endl<< setting.picking_highlight << std::endl;

        std::cout << "Save settings to: " << filename << std::endl;

    }else{
        std::cout << "Error open file: " << filename << std::endl;
    }
}

void defaultSettings(void){
        setting.trans=true;
        setting.light=false;
        setting.antia=false;
        //setting.cut_angle=180;
        //setting.trans_value=0.8;
        //setting.z_cutting=7000;


        setting.win_w=500;
        setting.win_h=500;
        setting.show_axes=true;
        setting.fps=false;
        setting.persp=true;
        setting.picking_highlight=false;


    
        

        for(int i=0;i < 4; i++){
            //setting.bgcolor[i]=0; //black
            setting.bgcolor[i]=1; //white
        }


        setting.font=0;

        for(int i=0; i < CED_MAX_LAYER; i++){
            setting.layer[i]=true; // turn all layers on
        }

        for(int i=0;i<NUMBER_DETECTOR_LAYER;i++){
            setting.detector_trans[i] =0.8;
            setting.detector_cut_angle[i] = 0;//180;
            setting.detector_cut_z[i] = 7000;
        }

            setting.phi_projection = false; // no phi projection
            setting.z_projection=false; // no phi projection;
            //mm.sf = fisheye_alpha > 0 ? mm.sf*8.0: mm.sf;
            //fisheye_alpha=0;
            setting.fixed_view=false;
            //update_cut_angle_menu();

            set_world_size(DEFAULT_WORLD_SIZE );


        //mm=mm_reset;
        //setting.va=mm.va;
        //setting.ha=mm.ha;


       //setting.zoom=0.072033;
       //mm.sf = setting.zoom;
//       fisheye_alpha=setting.fisheye_alpha;

//        FISHEYE_WORLD_SIZE = setting.fisheye_world_size;
//        WORLD_SIZE=setting.world_size;
//        selectFromMenu(VIEW_RESET);

            if((setting.trans == true && setting.persp == false) || (setting.trans == false && setting.persp == true)){
                selectFromMenu(GRAFIC_PERSP); //switch persp on in new view, switch persp off in classic view
            }
            //setting.z_cutting=7000; //no z cutting
            //setting.cut_angle=0;    // no detector cutting
            for(int i = 0; i<NUMBER_DETECTOR_LAYER;i++){
                setting.detector_trans[i]=0.8;
                setting.detector_cut_angle[i]=0;
                setting.detector_cut_z[i]=7000;
            }

            for(int i = 0; i<CED_MAX_LAYER;i++){
                setting.layer[i]=true;
            }
            setting.phi_projection = false; // no phi projection
            setting.z_projection=false; // no phi projection;
            mm=mm_reset;
            //mm.sf = fisheye_alpha > 0 ? mm.sf*8.0: mm.sf;
            fisheye_alpha=0;
            setting.fixed_view=false;
            //update_cut_angle_menu();
            set_world_size(DEFAULT_WORLD_SIZE ); 
            //std::cout << "DEFAULT_WORLD_SIZE "  << DEFAULT_WORLD_SIZE << "zoom: " << mm.sf << std::endl;
 

            setting.va=mm.va;
            setting.ha=mm.ha;
            setting.zoom=mm.sf;
            setting.fisheye_alpha=fisheye_alpha;
            
            setting.fisheye_world_size= FISHEYE_WORLD_SIZE ; 
            setting.world_size= WORLD_SIZE;

            std::cout << "Set options to default settings" << std::endl;
}

void idle(void){
    glutPostRedisplay();
}


void loadSettings(int slot){
    ifstream file;

    const char *home = getenv("HOME");
    char filename[1000];
    snprintf(filename, 1000, "%s/.glced_cfg/settings%i",home, slot);
    //std::cout << "Read config: " << filename << std::endl;
    file.open(filename);

    if(file.is_open()){
        string line;
//        file.read((char*)&setting, sizeof(setting));
            getline(file,line);getline(file,line);
            if(VERSION_CONFIG != atoi(line.c_str())){
                //std::cout << "WARNING: Cant read configfile (" << filename << ") please delete or rename it" << std::endl; 
                std::cout << "WARNING: Cant read configfile (" << filename << ") version does not match! Please delete or rename the file" << std::endl; 
                defaultSettings();
                return;
            } else{ 
                getline(file,line);getline(file,line);
                setting.trans=atoi(line.c_str());

                getline(file,line);getline(file,line);
                setting.persp=atoi(line.c_str());

                getline(file,line);getline(file,line);
                setting.antia=atoi(line.c_str());

                getline(file,line);getline(file,line);
                setting.light=atoi(line.c_str());



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


                getline(file,line);getline(file,line);
                setting.font = atoi(line.c_str());

                for(int i=0;i<CED_MAX_LAYER;i++){
                    getline(file,line);getline(file,line);
                    setting.layer[i]=atoi(line.c_str());
                }

                for(int i=0;i<NUMBER_DETECTOR_LAYER;i++){
                    getline(file,line);getline(file,line);
                    setting.detector_trans[i]=atof(line.c_str());

                    getline(file,line);getline(file,line);
                    setting.detector_cut_angle[i]=atof(line.c_str());

                    getline(file,line);getline(file,line);
                    setting.detector_cut_z[i]=atof(line.c_str());
                }

                getline(file,line);getline(file,line);
                setting.detector_picking = atoi(line.c_str());



                getline(file,line);getline(file,line);
                mm.mv.x = atof(line.c_str());
                getline(file,line);
                mm.mv.y = atof(line.c_str());
                getline(file,line);
                mm.mv.z = atof(line.c_str());


                getline(file,line);getline(file,line);
                setting.picking_highlight = atoi(line.c_str());




            //set_bg_color(setting.bgcolor[0],setting.bgcolor[1],setting.bgcolor[2],setting.bgcolor[3]); 
            std::cout << "Read settings from: " << filename << std::endl;
        }

    }else{ //set to default
        std::cout << "WARNING: Failed to read settings from: " << filename << std::endl;
        defaultSettings();
    }

    mm.va=setting.va;
    mm.ha=setting.ha;
    mm.sf = setting.zoom; 
    fisheye_alpha=setting.fisheye_alpha;


    FISHEYE_WORLD_SIZE = setting.fisheye_world_size; 
    WORLD_SIZE=setting.world_size;

    //reshape(setting.win_w, setting.win_h);
}




void mouseWheel(int button, int dir, int x, int y){ //hauke
    if(dir > 0){
        selectFromMenu(VIEW_ZOOM_IN);
    }else{
        selectFromMenu(VIEW_ZOOM_OUT);
    }
}

static void mouse_passive(int x,int y){
    //hier ced_menu 
    ced_menu->mouseMove(x,y);
    popupmenu->mouseMove(x,y);
    //cout << "x = " << x <<  endl;
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

    //double angle;
    switch(btn){
    case GLUT_LEFT_BUTTON:
        ced_menu->clickAt((int)mouse_x,(int)mouse_y);
        popupmenu->clickAt((int)mouse_x,(int)mouse_y);
        //reshape((int)window_width, (int)window_height);
        glutPostRedisplay();



        //hauke
        gettimeofday(&tv, 0); 
        //FIX IT: get the system double click time
        if( (tv.tv_sec*1000000+tv.tv_usec-doubleClickTime) < 300000 && (tv.tv_sec*1000000+tv.tv_usec-doubleClickTime) > 5){ //1000000=1sec
            //printf("Double Click %f\n", tv.tv_sec*1000000+tv.tv_usec-doubleClickTime);
            if(!ced_picking(x,y,&mm.mv.x,&mm.mv.y,&mm.mv.z)){

                GLfloat p_x, p_y, p_z;
                int id, layer, type;
                if(!find_selected_object(x,y,&p_x,&p_y,&p_z, &id, &layer, &type)){ //if ==1 found hit, else clicked on background
                    pick_point.x=p_x;
                    pick_point.y=p_y;
                    pick_point.z=p_z;
                    select_nothing=false;
                }


               sock=__glutSockets;
               id = SELECTED_ID;
               //printf(" ced_get_selected : socket connected: %d", sock->fd );	
               if(client_connected){
                    send( sock->fd , &id , sizeof(int) , 0 );
                }
            }else{
                select_nothing=true;
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
          //cout << "right button clicked" << endl;
          ced_menu->clickAt((int)mouse_x,(int)mouse_y);
          buildPopUpMenu(x,y);
          glutPostRedisplay();
          if(ZOOM_RIGHT_CLICK == false){
            return;
          }
          move_mode=ZOOM;
          return;
        case GLUT_MIDDLE_BUTTON:
          popupmenu->isExtend=false;
          //cout << "middle button clicked" << endl;
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

          popupmenu->isExtend=false;
        selectFromMenu(VIEW_ZOOM_IN);
      //  mm.mv.z+=150./mm.sf;
      //  glutPostRedisplay();
        return;
    }
    if (btn == mouseWheelDown || btn == 4 ){ // 4 is mouse-wheel-down under ubuntu

          popupmenu->isExtend=false;

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
    if(l > CED_MAX_LAYER-1){ return; }

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
    //if(key==0x1A ){ //ctrl+z

    //if(key=='u' ){ //ctrl+z

    //std::cout << "key: " << int(key) << endl;
    if(false ){ //ctrl+z
        //selectFromMenu(UNDO);
    } else if(key=='r'){ 
        selectFromMenu(VIEW_RESET);
    } else if(key=='R'){ 
        selectFromMenu(CED_RESET);
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
    }/*else if(key=='w'){
      mm.mv.x+=5; 
      //mm.mv.y=y;
      //mm.mv.z)) 
        glutPostRedisplay();
    }*/  else if(key=='v' || key=='V'){
          selectFromMenu(VIEW_FISHEYE);
    //} else if((key>='0') && (key<='9')){
    //      selectFromMenu(LAYER_0+key-'0');
    //} else if(key==')'){ // 0
    //      selectFromMenu(LAYER_10);
    //} else if(key=='!'){ // 1
    //      selectFromMenu(LAYER_11);
    //} else if(key=='@'){ // 2
    //      selectFromMenu(LAYER_12);
    //} else if(key=='#'){ // 3
    //      selectFromMenu(LAYER_13);
    //} else if(key=='$'){ // 4
    //      selectFromMenu(LAYER_14);
    //} else if(key=='%'){ // 5
    //      selectFromMenu(LAYER_15);
    //} else if(key=='^'){ // 6
    //      selectFromMenu(LAYER_16);
    //} else if(key=='&'){ // 7
    //      selectFromMenu(LAYER_17);
    //} else if(key=='*'){ // 8
    //      selectFromMenu(LAYER_18);
    //} else if(key=='('){ // 9
    //      selectFromMenu(LAYER_19);
    } else if(key== DATALAYER_SHORTKEY_00){
    selectFromMenu(LAYER_0);
    } else if(key== DATALAYER_SHORTKEY_01){
    selectFromMenu(LAYER_1);
    } else if(key== DATALAYER_SHORTKEY_02){
    selectFromMenu(LAYER_2);
    } else if(key== DATALAYER_SHORTKEY_03){
    selectFromMenu(LAYER_3);
    } else if(key== DATALAYER_SHORTKEY_04){
    selectFromMenu(LAYER_4);
    } else if(key== DATALAYER_SHORTKEY_05){
    selectFromMenu(LAYER_5);
    } else if(key== DATALAYER_SHORTKEY_06){
    selectFromMenu(LAYER_6);
    } else if(key== DATALAYER_SHORTKEY_07){
    selectFromMenu(LAYER_7);
    } else if(key== DATALAYER_SHORTKEY_08){
    selectFromMenu(LAYER_8);
    } else if(key== DATALAYER_SHORTKEY_09){
    selectFromMenu(LAYER_9);
    } else if(key== DATALAYER_SHORTKEY_10){
    selectFromMenu(LAYER_10);
    } else if(key== DATALAYER_SHORTKEY_11){
    selectFromMenu(LAYER_11);
    } else if(key== DATALAYER_SHORTKEY_12){
    selectFromMenu(LAYER_12);
    } else if(key== DATALAYER_SHORTKEY_13){
    selectFromMenu(LAYER_13);
    } else if(key== DATALAYER_SHORTKEY_14){
    selectFromMenu(LAYER_14);
    } else if(key== DATALAYER_SHORTKEY_15){
    selectFromMenu(LAYER_15);
    } else if(key== DATALAYER_SHORTKEY_16){
    selectFromMenu(LAYER_16);
    } else if(key== DATALAYER_SHORTKEY_17){
    selectFromMenu(LAYER_17);
    } else if(key== DATALAYER_SHORTKEY_18){
    selectFromMenu(LAYER_18);
    } else if(key== DATALAYER_SHORTKEY_19){
    selectFromMenu(LAYER_19);
    } else if(key== DATALAYER_SHORTKEY_20){
    selectFromMenu(LAYER_20);
    } else if(key== DATALAYER_SHORTKEY_21){
    selectFromMenu(LAYER_21);
    } else if(key== DATALAYER_SHORTKEY_22){
    selectFromMenu(LAYER_22);
    } else if(key== DATALAYER_SHORTKEY_23){
    selectFromMenu(LAYER_23);
    } else if(key== DATALAYER_SHORTKEY_24){
    selectFromMenu(LAYER_24);
    } else if(key=='`'){
    selectFromMenu(LAYER_ALL);


//const char detec_layer_keys[] = {'t','y','u','i','o','p','[',']','\\', 'T', 'Y','U','I','O','P','{','}','|',' ',' ',' '};
   } else if(key==DETECTORLAYER_SHORTKEY_00){
          selectFromMenu(DETECTOR1);
   } else if(key==DETECTORLAYER_SHORTKEY_01){
          selectFromMenu(DETECTOR2);
   } else if(key==DETECTORLAYER_SHORTKEY_02){
          selectFromMenu(DETECTOR3);
   } else if(key==DETECTORLAYER_SHORTKEY_03){
          selectFromMenu(DETECTOR4);
   } else if(key==DETECTORLAYER_SHORTKEY_04){
          selectFromMenu(DETECTOR5);
   } else if(key==DETECTORLAYER_SHORTKEY_05){
          selectFromMenu(DETECTOR6);
   } else if(key==DETECTORLAYER_SHORTKEY_06){
          selectFromMenu(DETECTOR7);
   } else if(key==DETECTORLAYER_SHORTKEY_07){
          selectFromMenu(DETECTOR8);
   } else if(key==DETECTORLAYER_SHORTKEY_08){
          selectFromMenu(DETECTOR9);
   } else if(key==DETECTORLAYER_SHORTKEY_09){
          selectFromMenu(DETECTOR10);
   } else if(key==DETECTORLAYER_SHORTKEY_10){
          selectFromMenu(DETECTOR11);
   } else if(key==DETECTORLAYER_SHORTKEY_11){
          selectFromMenu(DETECTOR12);
   } else if(key==DETECTORLAYER_SHORTKEY_12){
          selectFromMenu(DETECTOR13);
   } else if(key==DETECTORLAYER_SHORTKEY_13){
          selectFromMenu(DETECTOR14);
   } else if(key==DETECTORLAYER_SHORTKEY_14){
          selectFromMenu(DETECTOR15);
   } else if(key==DETECTORLAYER_SHORTKEY_15){
          selectFromMenu(DETECTOR16);
   } else if(key==DETECTORLAYER_SHORTKEY_16){
          selectFromMenu(DETECTOR17);
   } else if(key==DETECTORLAYER_SHORTKEY_17){
          selectFromMenu(DETECTOR18);
   } else if(key==DETECTORLAYER_SHORTKEY_18){
          selectFromMenu(DETECTOR19);
   } else if(key==DETECTORLAYER_SHORTKEY_19){
          selectFromMenu(DETECTOR20);

//    } else if(key=='t'){
//          selectFromMenu(DETECTOR1);
//    } else if(key=='y'){
//          selectFromMenu(DETECTOR2);
//    } else if(key=='u'){
//          selectFromMenu(DETECTOR3);
//    } else if(key=='i'){
//          selectFromMenu(DETECTOR4);
//    } else if(key=='o'){
//          selectFromMenu(DETECTOR5);
//    } else if(key=='p'){
//          selectFromMenu(DETECTOR6);
//    } else if(key=='['){
//          selectFromMenu(DETECTOR7);
//    } else if(key==']'){
//          selectFromMenu(DETECTOR8);
//    } else if(key=='\\'){
//          selectFromMenu(DETECTOR9);
//    } else if(key=='T'){
//          selectFromMenu(DETECTOR10);
//    } else if(key=='Y'){
//          selectFromMenu(DETECTOR11);
//    } else if(key=='U'){
//          selectFromMenu(DETECTOR12);
//    } else if(key=='I'){
//          selectFromMenu(DETECTOR13);
//    } else if(key=='O'){
//          selectFromMenu(DETECTOR14);
//    } else if(key=='P'){
//          selectFromMenu(DETECTOR15);
//    } else if(key=='{'){
//          selectFromMenu(DETECTOR16);
//    } else if(key=='}'){
//          selectFromMenu(DETECTOR17);
//    } else if(key=='|'){
//          selectFromMenu(DETECTOR18);
    } else if(key=='~'){
          selectFromMenu(DETECTOR_ALL);
    } else if(key == '+'|| key == '='){
          selectFromMenu(VIEW_ZOOM_IN);
    } else if(key == '-'|| key == '_'){
          selectFromMenu(VIEW_ZOOM_OUT);
    }else if (key == 26 || key == 'x'){ //ctrl+z
       selectFromMenu(UNDO);
    } else if(key == 'z'){

       // if (glutGetModifiers() == GLUT_ACTIVE_CTRL) {
        //if (glutGetModifiers() & GLUT_ACTIVE_CTRL){

            if(last_selected_layer > 0){
              if(setting.detector_cut_z[last_selected_layer - NUMBER_DATA_LAYER] < 7000){ 
                        setting.detector_cut_z[last_selected_layer - NUMBER_DATA_LAYER]+=100; 
              }
            }else{
                 for(int i = 0; i<NUMBER_DETECTOR_LAYER;i++){
                      if(setting.detector_cut_z[0] < 7000){
                          setting.detector_cut_z[i]+=100; 
                      }
                  }         
            }

        glutPostRedisplay();
    } else if(key == 'Z'){
            if(last_selected_layer > 0){
              if(setting.detector_cut_z[last_selected_layer - NUMBER_DATA_LAYER] > -7000){ 
                    setting.detector_cut_z[last_selected_layer - NUMBER_DATA_LAYER]-=100; 
              }
            }else{
                 for(int i = 0; i<NUMBER_DETECTOR_LAYER;i++){
                      if(setting.detector_cut_z[i] > -7000){
                          setting.detector_cut_z[i]-=100; 
                      }
                  }
            }         
        glutPostRedisplay();
    } else if(key == '<'){
         if(last_selected_layer > 0){
            if(setting.detector_trans[last_selected_layer - NUMBER_DATA_LAYER] > 0.005){
              setting.detector_trans[last_selected_layer - NUMBER_DATA_LAYER]-=0.005;
            }else{
              setting.detector_trans[last_selected_layer - NUMBER_DATA_LAYER]=0;
            }
        }else{
            for(int i = 0; i<NUMBER_DETECTOR_LAYER;i++){
                if(setting.detector_trans[i] > 0.005){
                    setting.detector_trans[i]-=0.005;
                }else{
                    setting.detector_trans[i]=0;
                }
            }

        }
         glutPostRedisplay();
    }else if(key == '>'){
          if(last_selected_layer > 0){
                if(setting.detector_trans[last_selected_layer - NUMBER_DATA_LAYER] < 1-0.005){
                  setting.detector_trans[last_selected_layer - NUMBER_DATA_LAYER]+=0.005;
                }else{
                  setting.detector_trans[last_selected_layer - NUMBER_DATA_LAYER]=1.;
                }
            }else{
               for(int i = 0; i<NUMBER_DETECTOR_LAYER;i++){
                    if(setting.detector_trans[i] < 1-0.005){
                        setting.detector_trans[i]+=0.005;
                    }else{
                      setting.detector_trans[i]=1.;
                    }
                }
            }

          glutPostRedisplay();

    }else if(key == 'm'){
         if(last_selected_layer > 0){
            if( setting.detector_cut_angle[last_selected_layer - NUMBER_DATA_LAYER] > 0){
              setting.detector_cut_angle[last_selected_layer - NUMBER_DATA_LAYER]-=0.5;
            }
          }else{
               for(int i = 0; i<NUMBER_DETECTOR_LAYER;i++){
                   if( setting.detector_cut_angle[i] > 0){
                        setting.detector_cut_angle[i]-=0.5;
                    }
            }
        }
        glutPostRedisplay();
    } else if(key == 'M'){
         if(last_selected_layer > 0){
          if(setting.detector_cut_angle[last_selected_layer - NUMBER_DATA_LAYER] < 360){
            setting.detector_cut_angle[last_selected_layer - NUMBER_DATA_LAYER]+=0.5;
            }
          }
         else{
          for(int i = 0; i<NUMBER_DETECTOR_LAYER;i++){
                if(setting.detector_cut_angle[i] < 360){
                  setting.detector_cut_angle[i]+=0.5;
                }
          }
        }

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
   case GLUT_KEY_RIGHT:
       mm.mv.z+=50.;
      break;
   case GLUT_KEY_LEFT:
       mm.mv.z-=50.;
      break;

   case GLUT_KEY_UP:
       mm.mv.y+=50.;
      break;
   case GLUT_KEY_DOWN:
       mm.mv.y-=50.;
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
        //cout << "move" << endl;
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


//void subDisplay(void){
//    char label[CED_MAX_LAYER_CHAR];
//    int i;
//
//    glutSetWindow(subWindow);
//    //glClearColor(0.5, 0.5, 0.5, 100);
//    glClearColor(0.5, 0.5, 0.5, 0.5);
//
//
//    //std::cout << glutGet(GLUT_WINDOW_WIDTH) << " vs " << window_width << std::endl; 
//    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//    float line = 45/window_height; //height of one line
//    //float column = 200/window_width;
//    float column = 200/window_width; //width of one line
//
//    const int ITEMS_PER_COLUMN=int(window_height/60.0); //how many lines per column?
//    //const int MAX_COLUMN= window_width/100;
//    //border
//    glColor3f(0,0.9,.9);
//    glBegin(GL_LINE_LOOP);
//    glVertex2f(0.001, 0.01);
//    glVertex2f(0.001, 0.99);
//    glVertex2f(0.999, 0.99);
//    glVertex2f(0.999, 0.01);
//    glEnd();
//
//    glColor3f(1.0, 1.0, 1.0); //white
//
//    //printf("window_height %f\nwindow width %f\n", window_height, window_width);
//
//    vector<string> shortcuts;
//    shortcuts.push_back( "[h] Toggle shortcut frame" );
//    shortcuts.push_back( "[r] Reset view" );
//    shortcuts.push_back( "[f] Font view" );
//    shortcuts.push_back( "[s] Side view" );
//    shortcuts.push_back( "[F] Front projection" );
//    shortcuts.push_back( "[S] Side projection" );
//    shortcuts.push_back( "[v] Fisheye projection" );
//    shortcuts.push_back( "[b] Change background color" );
//    shortcuts.push_back( "[+] Zoom in" );
//    shortcuts.push_back( "[-] Zoom out" );
//    shortcuts.push_back( "[c] Center" );
//    shortcuts.push_back( "[Z] Cut in z-axe direction" );
//    shortcuts.push_back( "[z] Cut in -z-axe direction" );
//    shortcuts.push_back( "[>] Increase transparency" );
//    shortcuts.push_back( "[<] Decrease transparency" );
//    shortcuts.push_back( "[`] Toggle all data layers" );
//    shortcuts.push_back( "[~] Toggle all detector layers" );
//    shortcuts.push_back( "[Esc] Quit CED" );
//
//    glColor3f(1.0, 1.0, 1.0);
//    sprintf (label, "Control keys");
//    glRasterPos2f(((int)(0/ITEMS_PER_COLUMN))*column+0.02, 0.80F);
//    drawStringBig(label);
//
//    //for(i=0;(unsigned) i<sizeof(shortcuts)/sizeof(shortcuts[0]);i++){
//
//    for(i=0;(unsigned) i<shortcuts.size();i++){
//       //if((i/ITEMS_PER_COLUMN) > MAX_COLUMN) break;
//       //sprintf(label,"%s", shortcuts[i]);
//       //glRasterPos2f(((int)(i/ITEMS_PER_COLUMN))*column+0.02,(ITEMS_PER_COLUMN-(i%ITEMS_PER_COLUMN))*line);
//       //printf("i=%i  lineposition=%i\n",i, (ITEMS_PER_COLUMN-(i%ITEMS_PER_COLUMN)));
//       drawHelpString(shortcuts[i], ((int)(i/ITEMS_PER_COLUMN))*column+0.02, (ITEMS_PER_COLUMN-(i%ITEMS_PER_COLUMN))*line );
//        //drawString(label);
//    }
//
//    int actual_column=(int)((i-1)/ITEMS_PER_COLUMN)+1;
//
//    int aline=0;
//    int j=0;
//    char tmp[CED_MAX_LAYER_CHAR];
//    int jj=0;
//
//    glColor3f(1.0, 1.0, 1.0);
//    sprintf (label, "Layers");
//    glRasterPos2f(((int)(aline/ITEMS_PER_COLUMN)+actual_column)*column, 0.80F);
//    drawStringBig(label);
//
//    for(i=0;i<NUMBER_DATA_LAYER;i++){
//        for(j=0;j<CED_MAX_LAYER_CHAR-1;j++){
//            if(layerDescription[i][j] != ','){
//                tmp[j]=layerDescription[i][j];
//            }else{
//                tmp[j]=0;
//                j+=2;
//                break;
//            }
//        }
//        
//       //sprintf(label,"[%c] %s%i: %s", layer_keys[i], (i<10)?"0":"", i, layerDescription[i]);
//        sprintf(label,"[%c] %s%i: %s", layer_keys[i], (i<10)?"0":"", i, tmp);
//        drawHelpString(label, ((int)(aline/ITEMS_PER_COLUMN)+actual_column)*column,(ITEMS_PER_COLUMN-(aline%ITEMS_PER_COLUMN))*line);
//        aline++;
//
//        jj=j;
//
//        for(;j<CED_MAX_LAYER_CHAR-1;j++){
//            if(layerDescription[i][j] == ',' || layerDescription[i][j] == 0){
//                tmp[j-jj]=0;
//                j++; //scrip ", "
//                jj=j+1;
//                //drawHelpString(tmp, ((int)(aline/ITEMS_PER_COLUMN)+actual_column+.18)*column,(ITEMS_PER_COLUMN-(aline%ITEMS_PER_COLUMN))*line);
//                sprintf(label,"[%c] %s%i: %s", layer_keys[i], (i<10)?"0":"", i, tmp);
//                drawHelpString(label, ((int)(aline/ITEMS_PER_COLUMN)+actual_column)*column,(ITEMS_PER_COLUMN-(aline%ITEMS_PER_COLUMN))*line);
//
//                aline++;
//                if(layerDescription[i][j] == 0){ break; }
//            }else{
//                tmp[j-jj]=layerDescription[i][j];
//            }
//        }
//    }
//    glutSwapBuffers ();
//}

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
    if(showHelp == 1){
        showHelp=0;
    }else{
        showHelp=1;
    }
    glutPostRedisplay();
//    mainWindow=glutGetWindow();
//    
//    if(showHelp == 1){
//        glutDestroyWindow(subWindow);
//        showHelp=0;
//    }else if(showHelp == 0){
//        subWindow=glutCreateSubWindow(mainWindow,5,5,int(window_width-10),int(window_height/4.0));
//
//        glutDisplayFunc(subDisplay);
//        glutReshapeFunc(subReshape);
//            
//        glutKeyboardFunc(keypressed);
//        glutSpecialFunc(SpecialKey);
//
//        glutPostRedisplay();
//
//        glutSetWindow(mainWindow);
//        showHelp=1;
//    }    
//    glutSetWindow(mainWindow);
}

void updateLayerEntryInPopupMenu(int id){ //id is layer id, not menu id!
//    char string[200];
//    char tmp[41];
//    if(id < 0 || id > NUMBER_POPUP_LAYER-1){
//        return;
//    }
//    strncpy(tmp, layerDescription[id], 40); 
//    tmp[40]=0;
//    
//    sprintf(string,"[%s] Layer %s%i [%c]: %s%s",isLayerVisible(id)?"X":"   ", (id < 10)?"0":"" ,id, layer_keys[id], tmp, (strlen(layerDescription[id]) > 40)?"...":"");
//    glutSetMenu(layerMenu);
//    glutChangeToMenuEntry(id+2,string, id+LAYER_0);                     
}

void updateScreenshotMenu(void){
//    char tmp[200];
//
//    glutSetMenu(subscreenshot);
//    window_width=  glutGet(GLUT_WINDOW_WIDTH);
//    window_height= glutGet(GLUT_WINDOW_HEIGHT);
//
//    sprintf(tmp,"Screenshot small (%.0f x %.0f) (~ %.2f MB)",window_width, window_height, window_width*window_height*3./1000000.);
//    glutChangeToMenuEntry(1,tmp,SAVE_IMAGE1);
//
//    sprintf(tmp,"Screenshot medium (%.0f x %.0f) (~ %.2f MB)",window_width*4, window_height*4, 4*4*window_width*window_height*3./1000000.);
//    glutChangeToMenuEntry(2,tmp,SAVE_IMAGE4);
//
//    sprintf(tmp,"Screenshot large (%.0f x %.0f) (~ %.2f MB)",window_width*10, window_height*10, 10*10*window_width*window_height*3./1000000.);
//    glutChangeToMenuEntry(3,tmp,SAVE_IMAGE10);
//
//    sprintf(tmp,"Screenshot extra large (%.0f x %.0f) (~ %.2f MB)",window_width*20, window_height*20, 20*20*window_width*window_height*3./1000000.);
//    glutChangeToMenuEntry(4,tmp,SAVE_IMAGE20);
//
//    sprintf(tmp,"Screenshot too large (%.0f x %.0f) (~ %.2f MB)",window_width*100, window_height*100, 100*100*window_width*window_height*3./1000000.);
//    glutChangeToMenuEntry(5,tmp,SAVE_IMAGE100);
//
}
void updateSaveLoadMenu(int id){ //id is save id, not menu id!
//    struct stat s;
//
//
//    const char *home = getenv("HOME");
//    char filename[1000];
//    char menuStr[1000];
//    snprintf(filename, 1000, "%s/.glced_cfg/settings%i", home, id);
//    if(stat(filename,&s) == 0){
//        snprintf(menuStr,1000,"Slot %i, created: %s",id,ctime(&s.st_mtime));
//    }else{
//        snprintf(menuStr,1000,"Slot %i, %s",id,"Empty");
//    }
//
//    glutSetMenu(subSave);
//    glutChangeToMenuEntry(id,menuStr, SAVE1+id-1);                     
//    
//    glutSetMenu(subLoad);
//    glutChangeToMenuEntry(id,menuStr, LOAD1+id-1);                     
//
//    //std::cout << menuStr << std::endl;
}


void updateLayerEntryDetector(int id){ //id is layer id, not menu id!
//    char string[200];
//    char tmp[101];
//    if(id < NUMBER_DATA_LAYER || id > NUMBER_DETECTOR_LAYER+NUMBER_DATA_LAYER-1 || id > CED_MAX_LAYER-1 || id < 0){
//        return;
//    }
//    strncpy(tmp, layerDescription[id], 100); 
//    tmp[100]=0;
//    
//    //sprintf(string,"[%s] Layer %s%i [%c]: %s%s",isLayerVisible(id)?"X":"   ", (id < 10)?"0":"" ,id, layer_keys[id], tmp, (strlen(layerDescription[id]) > 40)?"...":"");
//    sprintf(string,"[%s] Layer %s%i [%c]: %s%s",isLayerVisible(id)?"X":"   ", (id < 10)?"0":"" ,id,detec_layer_keys[id-NUMBER_DATA_LAYER],tmp, (strlen(layerDescription[id]) > 100)?"...":"");
//
//    glutSetMenu(detectorMenu);
//    glutChangeToMenuEntry(id-NUMBER_DATA_LAYER+2,string, id-NUMBER_DATA_LAYER+DETECTOR1);                     
}


void addLayerDescriptionToMenu(int id, char * str){
    if(id < 0 || id >= CED_MAX_LAYER){
        printf("Warning: Layer id out of range\n");
        return;
    }
    strncpy(layerDescription[id], str,CED_MAX_LAYER_CHAR-1);
    updateLayerEntryInPopupMenu(id);
    updateLayerEntryDetector(id);

}


void update_cut_angle_menu(void){
    return;
    //char str[200];

    //int i;

    //glutSetMenu(subsubMenu2);

    //for(i=0; (unsigned)i < sizeof(available_cutangles)/sizeof(available_cutangles[0]); i++){

    //    if(available_cutangles[i] == setting.cut_angle){
    //        sprintf(str,"[X] %i", available_cutangles[i]);
    //        glutChangeToMenuEntry(i+1, str,  CUT_ANGLE0+i);
    //    }else{
    //        sprintf(str,"[  ] %i", available_cutangles[i]);
    //        glutChangeToMenuEntry(i+1, str,  CUT_ANGLE0+i);
    //    }
    //}

}

void copySetting(CEDsettings &dest, CEDsettings &source, const char *name){
    if(strcmp(name,"trans")==0){
        for(int i=0;i<NUMBER_DETECTOR_LAYER;i++){
            dest.detector_trans[i]=source.detector_trans[i];
        }
    }

    else if(strcmp(name,"cut z")==0){
        for(int i=0;i<NUMBER_DETECTOR_LAYER;i++){
            dest.detector_cut_z[i]=source.detector_cut_z[i];
        }
    }
    
    else if(strcmp(name,"cut angle")==0){
        for(int i=0;i<NUMBER_DETECTOR_LAYER;i++){
            dest.detector_cut_angle[i]=source.detector_cut_angle[i];
        }
    }
    else{
        std::cout << "WARNING: unknown settingtype: " << name << std::endl;
    } 

}

void selectFromMenu(int id){ //hauke
    int anz;
    static CEDsettings backup_setting;
    //static float z_cutting_backup;
    //static float cut_angle_backup;
    static float mm_ha_backup; 
    static float mm_va_backup;
    static int graphic_2_backup;
    //static int fullscreen=false;
    double z_cut;


    glutSetWindow(mainWindow); //hauke

    if(id != UNDO){
        setting_old[4]=setting_old[3];
        setting_old[3]=setting_old[2];
        setting_old[2]=setting_old[1];
        setting_old[1]=setting_old[0];
        setting_old[0]=setting;
    }

    switch(id){
        case PICK_HIT:

            //cout << "TODO: pick with: " << popupmenu->x_click << " , " << popupmenu->y_click << endl;
            if(!ced_picking(popupmenu->x_click,popupmenu->y_click ,&mm.mv.x,&mm.mv.y,&mm.mv.z)){
               struct __glutSocketList *sock;
               sock=__glutSockets;
               int id = SELECTED_ID;
               //printf(" ced_get_selected : socket connected: %d", sock->fd );	
               if(client_connected){
                    send( sock->fd , &id , sizeof(int) , 0 );
                }
            }
            break;

        case CENTER_HIT:
            if(!ced_get_selected(popupmenu->x_start,popupmenu->y_start,&mm.mv.x,&mm.mv.y,&mm.mv.z)) glutPostRedisplay();
            break;

        case BGCOLOR_OPTION1:
            set_bg_color(CED_BGCOLOR_OPTION1_COLORCODE); 
            break;

        case BGCOLOR_OPTION2:
            set_bg_color(CED_BGCOLOR_OPTION2_COLORCODE); 
            break;

        case BGCOLOR_OPTION3:
            set_bg_color(CED_BGCOLOR_OPTION3_COLORCODE); 
            break;

        case BGCOLOR_OPTION4:
            set_bg_color(CED_BGCOLOR_OPTION4_COLORCODE); 
            break;

        case BGCOLOR_OPTION5:
            set_bg_color(CED_BGCOLOR_OPTION5_COLORCODE); 
            break;

        case BGCOLOR_OPTION6:
            set_bg_color(CED_BGCOLOR_OPTION6_COLORCODE); 
            break;

        case BGCOLOR_OPTION7:
            set_bg_color(CED_BGCOLOR_OPTION7_COLORCODE); 
            break;

        case BGCOLOR_OPTION8:
            set_bg_color(CED_BGCOLOR_OPTION8_COLORCODE); 
            break;

        case BGCOLOR_OPTION9:
            set_bg_color(CED_BGCOLOR_OPTION9_COLORCODE); 
            break;

        case BGCOLOR_OPTION10:
            set_bg_color(CED_BGCOLOR_OPTION10_COLORCODE); 
            break;

        case BGCOLOR_OPTION11:
            set_bg_color(CED_BGCOLOR_OPTION11_COLORCODE); 
            break;

        case BGCOLOR_OPTION12:
            set_bg_color(CED_BGCOLOR_OPTION12_COLORCODE); 
            break;

        case BGCOLOR_OPTION13:
            set_bg_color(CED_BGCOLOR_OPTION13_COLORCODE); 
            break;

        case BGCOLOR_OPTION14:
            set_bg_color(CED_BGCOLOR_OPTION14_COLORCODE); 
            break;

        case BGCOLOR_OPTION15:
            set_bg_color(CED_BGCOLOR_OPTION15_COLORCODE); 
            break;






//        case BGCOLOR_GAINSBORO:
//            set_bg_color(0.862745,0.862745,0.862745,0); 
//            //set_bg_color(0.862745,0.862745,0.862745,0);
//            break;
//
//        case BGCOLOR_LIGHTGREY:
//            set_bg_color(0.827451,0.827451,0.827451,0);
//            break;
//
//        case BGCOLOR_DARKGRAY:
//            set_bg_color(0.662745,0.662745,0.662745,0);
//            break;
//
//        case BGCOLOR_GRAY:
//            set_bg_color(0.501961,0.501961,0.501961,0);
//            break;
//
//        case BGCOLOR_SILVER:
//            set_bg_color(0.7529,0.7529,0.7529,0);
//            break;
//
//        case BGCOLOR_DIMGRAY:
//            set_bg_color(0.4118,0.4118,0.4118,0);
//            break;
//
//        case BGCOLOR_LIGHTSTEELBLUE:
//            set_bg_color(0.6902,0.7686 ,0.8706,0);
//            break;
//
//        case BGCOLOR_STEELBLUE:
//            set_bg_color(0.2745,0.5098,0.70588,0);
//            break;
//
//        case BGCOLOR_SEAGREEN:
//            set_bg_color(0.18039,0.54509,0.34117,0);
//            break;
//
//        case BGCOLOR_ORANGE:
//            set_bg_color(1,0.647,0,0);
//            break;
//
//        case BGCOLOR_YELLOW:
//            set_bg_color(1,1,0,0);
//            break;
//
//        case BGCOLOR_VIOLET:
//            set_bg_color(0.9333,0.5098,0.9333,0);
//            break;
//
//        case BGCOLOR_BLACK:
//            set_bg_color(0,0,0,0);
//            break;
//
//        case BGCOLOR_BLUE:
//            set_bg_color(0,0.2,0.4,0);
//            break;
//
//        case BGCOLOR_WHITE:
//            set_bg_color(1,1,1,0);
//            break;

        case BGCOLOR_USER:
            set_bg_color(userDefinedBGColor[0],userDefinedBGColor[1], userDefinedBGColor[2], userDefinedBGColor[3]);


        case VIEW_RESET:
            setting.phi_projection = false; // no phi projection
            setting.z_projection=false; // no phi projection;
            mm=mm_reset;
            //mm.sf = fisheye_alpha > 0 ? mm.sf*8.0: mm.sf;
            fisheye_alpha=0;
            setting.fixed_view=false;
            //update_cut_angle_menu();
            set_world_size(DEFAULT_WORLD_SIZE ); 
            break;

        case CED_RESET:
            //if(graphic[2] == 0){selectFromMenu(GRAFIC_PERSP); }
            if((setting.trans == true && setting.persp == false) || (setting.trans == false && setting.persp == true)){
                selectFromMenu(GRAFIC_PERSP); //switch persp on in new view, switch persp off in classic view
            }
            //setting.z_cutting=7000; //no z cutting
            //setting.cut_angle=0;    // no detector cutting
            for(int i = 0; i<NUMBER_DETECTOR_LAYER;i++){
                setting.detector_trans[i]=0.8;
                setting.detector_cut_angle[i]=0;
                setting.detector_cut_z[i]=7000;
            }

            for(int i = 0; i<CED_MAX_LAYER;i++){
                setting.layer[i]=true;
            }
            setting.phi_projection = false; // no phi projection
            setting.z_projection=false; // no phi projection;
            mm=mm_reset;
            //mm.sf = fisheye_alpha > 0 ? mm.sf*8.0: mm.sf;
            fisheye_alpha=0;
            setting.fixed_view=false;
            //update_cut_angle_menu();
            set_world_size(DEFAULT_WORLD_SIZE ); 
            //std::cout << "DEFAULT_WORLD_SIZE "  << DEFAULT_WORLD_SIZE << "zoom: " << mm.sf << std::endl;
            setting.light=false;

            setting.show_axes=true;
            break;


        case VIEW_FISHEYE:
            if(fisheye_alpha==0.0){
                mm.sf *= FISHEYE_ZOOM; //zoom in to hold the same detector size
                fisheye_alpha = FISHEYE_ALPHA;
                FISHEYE_WORLD_SIZE = WORLD_SIZE/(WORLD_SIZE*fisheye_alpha); //<-- new
                set_world_size(WORLD_SIZE); // <-- old
            }
            else{
                mm.sf *= 1.0/FISHEYE_ZOOM; //zoom out for the same look
                fisheye_alpha = 0.0;
                set_world_size(FISHEYE_WORLD_SIZE); //<-- old
            }
            break;

        case VIEW_FRONT:
            //mm=mm_reset;
            //mm.sf = fisheye_alpha > 0 ? mm.sf*8.0: mm.sf;

            if(setting.fixed_view){ break;}

            mm.ha=180.;
            mm.va=0.;
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
                //setting.z_cutting=z_cutting_backup;
                //copySetting(setting, backup_setting, "cut z");
                //setting.detector_cut_z=backup_setting.detector_cut_z;
                //setting.cut_angle=cut_angle_backup;


                copySetting(setting, backup_setting, "cut angle");
                //setting.detector_cut_angle=backup_setting.detector_cut_angle;
                //if(graphic_2_backup != graphic[2]){selectFromMenu(GRAFIC_PERSP); } //restore persp setting
                if(graphic_2_backup != setting.persp){selectFromMenu(GRAFIC_PERSP); } //restore persp setting

                mm.ha = mm_ha_backup;
                mm.va = mm_va_backup;

                setting.fixed_view=false;

            }else{ //turn projection on
                if(setting.z_projection){
                    selectFromMenu(TOGGLE_Z_PROJECTION);
                }

                //z_cutting_backup=setting.z_cutting;
                //cut_angle_backup=setting.cut_angle;

                copySetting(backup_setting, setting, "cut angle");
                //copySetting(backup_setting, setting, "cut z");
                //backup_setting.detector_cut_z=setting.detector_cut_z;
                //backup_setting.detector_cut_angle=setting.detector_cut_angle;

                setting.phi_projection=true;

                //graphic_2_backup=graphic[2];
                graphic_2_backup=setting.persp;

                //if(graphic[2]==1){selectFromMenu(GRAFIC_PERSP); }
                if(setting.persp==1){selectFromMenu(GRAFIC_PERSP); }


                for(int i=0;i<NUMBER_DETECTOR_LAYER;i++){
                    setting.detector_cut_angle[i]=180;
                //    setting.detector_cut_z[i]=7000;
                }
                //setting.cut_angle=180;
                //setting.z_cutting=7000;
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
                //setting.z_cutting=z_cutting_backup;

                copySetting(setting, backup_setting, "cut z");
                //copySetting(setting, backup_setting, "cut angle");
                //setting.detector_cut_z = backup_setting.detector_cut_z;
                //setting.cut_angle=cut_angle_backup;
                //setting.detector_cut_angle = backup_setting.detector_cut_angle;
                //if(graphic[2]==0){selectFromMenu(GRAFIC_PERSP); }
                //if(graphic_2_backup != graphic[2]){selectFromMenu(GRAFIC_PERSP); } //restore persp setting
                if(graphic_2_backup != setting.persp){selectFromMenu(GRAFIC_PERSP); } //restore persp setting


                mm.ha = mm_ha_backup;
                mm.va = mm_va_backup;

                setting.fixed_view=false;
            }else{ //turn projection on

                if(setting.phi_projection){selectFromMenu(TOGGLE_PHI_PROJECTION);}

                //z_cutting_backup=setting.z_cutting;

                copySetting(backup_setting, setting, "cut z");
                //cout << "toggle z projection!:" << endl;
                //for(int i = 0; i<NUMBER_DETECTOR_LAYER;i++){
                //    cout << backup_setting.detector_cut_z[i] << endl;
                //}



                //copySetting(backup_setting, setting, "cut angle");



                //backup_setting.detector_cut_z=setting.detector_cut_z;
                //cut_angle_backup=setting.cut_angle;
                //backup_setting.detector_cut_angle=setting.detector_cut_angle;

                setting.z_projection=true;

                for(int i=0;i<NUMBER_DETECTOR_LAYER;i++){
                    //setting.cut_angle=0;
                    //setting.z_cutting=-10;
                    setting.detector_cut_z[i]=-10;
                }

                //graphic_2_backup=graphic[2];
                graphic_2_backup=setting.persp;

                //if(graphic[2]==1){selectFromMenu(GRAFIC_PERSP); }
                if(setting.persp==true){selectFromMenu(GRAFIC_PERSP); }

               
               //side view
                mm_ha_backup=mm.ha;
                mm_va_backup = mm.va;

                mm.ha=180.;
                mm.va=0.;

                setting.fixed_view=true;
            }

            update_cut_angle_menu();
            break;

        case FONT0:
            setting.font=0;
            //buildMainMenu();
            break;

        case FONT1:
            setting.font=1;
            //buildMainMenu();
            break;

        case FONT2:
            setting.font=2;
            //buildMainMenu();
            break;


        case UNDO:
            setting=setting_old[0];
            setting_old[0]=setting_old[1];
            setting_old[1]=setting_old[2];
            setting_old[2]=setting_old[3];
            setting_old[3]=setting_old[4];
            setting_old[4]=setting_old[4];
            break;


        case VIEW_ZOOM_IN:
            mm.sf += mm.sf*50.0/window_height;
            //cout << "mm.sf: " << mm.sf << endl;
            //reshape((int)window_width, (int)window_height);

            //if(mm.sf>50){ mm.sf=50; }
            break;

        case VIEW_ZOOM_OUT:
            mm.sf -= mm.sf*50.0/window_height;
            //cout << "mm.sf: " << mm.sf << endl;

            //if(mm.sf > 0.05){
            //    mm.sf -= 0.05;
            //}else{
            //    mm.sf=0.00001;
            //}
            //reshape((int)window_width, (int)window_height);

            //if(mm.sf<0.01){ mm.sf=0.01; }
            break;

        case VIEW_CENTER:
            //ced_get_selected(x,y,&mm.mv.x,&mm.mv.y,&mm.mv.z);
            break;

        case LAYER_ALL:
            glutSetMenu(layerMenu);
            anz=0;
            //for(i=0;i<NUMBER_POPUP_LAYER;i++){ //try to turn all layers on

            for(int i=0;i<NUMBER_DATA_LAYER;i++){ //try to turn all layers on
                if(!isLayerVisible(i)){
                   //sprintf(string,"[X] Layer %s%i [%c]: %s", (i < 10)?"0":"" ,i, layer_keys[i], layerDescription[i]);
                   //glutChangeToMenuEntry(i+2,string, LAYER_0+i);                     
                   toggle_layer(i);
                   //updateLayerEntryInPopupMenu(i);
                   anz++;
                }
            }
            if(anz == 0){ //turn all layers off
                //for(i=0;i<NUMBER_POPUP_LAYER;i++){

                for(int i=0;i<NUMBER_DATA_LAYER;i++){
                   //sprintf(string,"[   ] Layer %s%i [%c]: %s",(i < 10)?"0":"" ,i, layer_keys[i], layerDescription[i]);
                   //glutChangeToMenuEntry(i+2,string, LAYER_0+i);                     
                   toggle_layer(i);
                   //updateLayerEntryInPopupMenu(id);
                }
            }
            break;

        case DETECTOR_ALL:
            glutSetMenu(detectorMenu);
            anz=0;
            for(int i=NUMBER_DATA_LAYER;i<NUMBER_DETECTOR_LAYER+NUMBER_DATA_LAYER;i++){ //try to turn all layers on
                if(!isLayerVisible(i)){
                   //sprintf(string,"[X] Layer %s%i [%c]: %s", (i < 10)?"0":"" ,i, layer_keys[i], layerDescription[i]);
                   //glutChangeToMenuEntry(i+2,string, LAYER_0+i);                     
                   toggle_layer(i);
                   //updateLayerEntryDetector(i);
                   anz++;
                }
            }
            if(anz == 0){ //turn all layers off
                for(int i=NUMBER_DATA_LAYER;i<NUMBER_DETECTOR_LAYER+NUMBER_DATA_LAYER;i++){
                   //sprintf(string,"[   ] Layer %s%i [%c]: %s",(i < 10)?"0":"" ,i, layer_keys[i], layerDescription[i]);
                   //glutChangeToMenuEntry(i+2,string, LAYER_0+i);                     
                   toggle_layer(i);
                   //updateLayerEntryDetector(id);
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
            //updateLayerEntryDetector(id-DETECTOR1+NUMBER_DATA_LAYER);
            
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
        case LAYER_20:
        case LAYER_21:
        case LAYER_22:
        case LAYER_23:
        case LAYER_24:
            glutSetMenu(layerMenu);
            toggle_layer(id-LAYER_0);
            //std::cout << "toogle layer " << id-LAYER_0 << std::endl;
            //updateLayerEntryInPopupMenu(id-LAYER_0);
            //buildLayerMenus();
            //buildMainMenu();

            break;

        case CUT_ANGLE0  :
        case CUT_ANGLE30 :
        case CUT_ANGLE90 :
        case CUT_ANGLE135:
        case CUT_ANGLE180:
        case CUT_ANGLE200:
        case CUT_ANGLE220:
        case CUT_ANGLE240:
        case CUT_ANGLE260:
        case CUT_ANGLE270:
        case CUT_ANGLE280:
        case CUT_ANGLE290:
        case CUT_ANGLE310:
        case CUT_ANGLE330:
        case CUT_ANGLE340:
        case CUT_ANGLE350:
        case CUT_ANGLE45 :
        case CUT_ANGLE100:
        case CUT_ANGLE120:
        case CUT_ANGLE150:
        case CUT_ANGLE170:
        case CUT_ANGLE190:
            for(int i=0;i<NUMBER_DETECTOR_LAYER;i++)
               setting.detector_cut_angle[i]=id-CUT_ANGLE0;
            break;




        case LAYER_CUT_ANGLE0  :
        case LAYER_CUT_ANGLE30 :
        case LAYER_CUT_ANGLE90 :
        case LAYER_CUT_ANGLE135:
        case LAYER_CUT_ANGLE180:
        case LAYER_CUT_ANGLE200:
        case LAYER_CUT_ANGLE220:
        case LAYER_CUT_ANGLE240:
        case LAYER_CUT_ANGLE260:
        case LAYER_CUT_ANGLE270:
        case LAYER_CUT_ANGLE280:
        case LAYER_CUT_ANGLE290:
        case LAYER_CUT_ANGLE310:
        case LAYER_CUT_ANGLE330:
        case LAYER_CUT_ANGLE340:
        case LAYER_CUT_ANGLE350:
        case LAYER_CUT_ANGLE45 :
        case LAYER_CUT_ANGLE100:
        case LAYER_CUT_ANGLE120:
        case LAYER_CUT_ANGLE150:
        case LAYER_CUT_ANGLE170:
        case LAYER_CUT_ANGLE190:
            setting.detector_cut_angle[last_selected_layer- NUMBER_DATA_LAYER]=id-LAYER_CUT_ANGLE0;
            break;



        

        case CUT_Z_7000:z_cut=7000;
            for(int i=0;i<NUMBER_DETECTOR_LAYER;i++)
               setting.detector_cut_z[i]=z_cut;
            break;

        case CUT_Z_M6000:z_cut=-6000;
            for(int i=0;i<NUMBER_DETECTOR_LAYER;i++)
               setting.detector_cut_z[i]=z_cut;
            break;

        case CUT_Z_M4000:z_cut=-4000;
            for(int i=0;i<NUMBER_DETECTOR_LAYER;i++)
               setting.detector_cut_z[i]=z_cut;
            break;

        case CUT_Z_M2000:z_cut=-2000;
            for(int i=0;i<NUMBER_DETECTOR_LAYER;i++)
               setting.detector_cut_z[i]=z_cut;
            break;

        case CUT_Z_0000 :z_cut=0 ;
            for(int i=0;i<NUMBER_DETECTOR_LAYER;i++)
               setting.detector_cut_z[i]=z_cut;
            break;

        case CUT_Z_2000 :z_cut=2000 ;
            for(int i=0;i<NUMBER_DETECTOR_LAYER;i++)
               setting.detector_cut_z[i]=z_cut;
            break;

        case CUT_Z_4000 :z_cut=4000 ;
            for(int i=0;i<NUMBER_DETECTOR_LAYER;i++)
               setting.detector_cut_z[i]=z_cut;
            break;

        case CUT_Z_6000 :z_cut=6000 ;
            for(int i=0;i<NUMBER_DETECTOR_LAYER;i++)
               setting.detector_cut_z[i]=z_cut;
            break;

        case LAYER_CUT_Z_7000:z_cut=7000;
            setting.detector_cut_z[last_selected_layer- NUMBER_DATA_LAYER]=z_cut;
            break;

        case LAYER_CUT_Z_M4000:z_cut=-4000;
            setting.detector_cut_z[last_selected_layer- NUMBER_DATA_LAYER]=z_cut;
            break;

        case LAYER_CUT_Z_M2000:z_cut=-2000;
            setting.detector_cut_z[last_selected_layer- NUMBER_DATA_LAYER]=z_cut;
            break;

        case LAYER_CUT_Z_0000 :z_cut=0 ;
            setting.detector_cut_z[last_selected_layer- NUMBER_DATA_LAYER]=z_cut;
            break;

        case LAYER_CUT_Z_2000 :z_cut=2000 ;
            setting.detector_cut_z[last_selected_layer- NUMBER_DATA_LAYER]=z_cut;
            break;

        case LAYER_CUT_Z_4000 :z_cut=4000 ;
            setting.detector_cut_z[last_selected_layer- NUMBER_DATA_LAYER]=z_cut;
            break;

        case LAYER_CUT_Z_6000 :z_cut=6000 ;
            setting.detector_cut_z[last_selected_layer- NUMBER_DATA_LAYER]=z_cut;
            break;



        case TRANS0:
            for(int i=0;i<NUMBER_DETECTOR_LAYER;i++)
                setting.detector_trans[i]=0;
            //setting.trans_value=0;
            break;
        case TRANS40:
            for(int i=0;i<NUMBER_DETECTOR_LAYER;i++)
                setting.detector_trans[i]=0.4;
            //setting.trans_value=0.4;
            break;
        case TRANS60:
             for(int i=0;i<NUMBER_DETECTOR_LAYER;i++)
                setting.detector_trans[i]=0.6;
            //setting.trans_value=0.6;
            break;
        case TRANS70:
             for(int i=0;i<NUMBER_DETECTOR_LAYER;i++)
                setting.detector_trans[i]=0.7;
            //setting.trans_value=0.7;
            break;
        case TRANS80:
            for(int i=0;i<NUMBER_DETECTOR_LAYER;i++)
                setting.detector_trans[i]=0.8;
            //setting.trans_value=0.8;
            break;
        case TRANS90:
            for(int i=0;i<NUMBER_DETECTOR_LAYER;i++)
                setting.detector_trans[i]=0.9;
            //setting.trans_value=0.9;
            break;
        case TRANS95:
            for(int i=0;i<NUMBER_DETECTOR_LAYER;i++)
                setting.detector_trans[i]=0.95;
            //setting.trans_value=0.95;
            break;
        case TRANS100:
            for(int i=0;i<NUMBER_DETECTOR_LAYER;i++)
                setting.detector_trans[i]=1.0;
            //setting.trans_value=1.0;
            break;

        case LAYER_TRANS0:
            setting.detector_trans[last_selected_layer- NUMBER_DATA_LAYER]=0;
            break;
        case LAYER_TRANS40:
            setting.detector_trans[last_selected_layer- NUMBER_DATA_LAYER]=0.4;
            break;
        case LAYER_TRANS60:
            setting.detector_trans[last_selected_layer- NUMBER_DATA_LAYER]=0.6;
            break;
        case LAYER_TRANS70:
            setting.detector_trans[last_selected_layer- NUMBER_DATA_LAYER]=0.7;
            break;
        case LAYER_TRANS80:
            setting.detector_trans[last_selected_layer- NUMBER_DATA_LAYER]=0.8;
            break;
        case LAYER_TRANS90:
            setting.detector_trans[last_selected_layer- NUMBER_DATA_LAYER]=0.9;
            break;
        case LAYER_TRANS95:
            setting.detector_trans[last_selected_layer- NUMBER_DATA_LAYER]=0.95;
            break;
        case LAYER_TRANS100:
            setting.detector_trans[last_selected_layer- NUMBER_DATA_LAYER]=1.0;
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
            //cout << "call fps" << endl;
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

                // break; //do nothing...

                 //TODO: CHANGE IT
                 GLfloat light0_spec[] = {1, 1, 1, 0.5};
                 GLfloat light0_ambi[] = {1, 1, 1, 0.5};
                 GLfloat light0_diff[] = {1, 1, 1, 0.5};
//      mm.ha=mm.ha_start+(x-mouse_x)*180./window_width;
//      mm.va=mm.va_start+(y-mouse_y)*180./window_height;
 
                
                 GLfloat light0_pos[] = {20000, 20000, 20000};
                 
            glBegin(GL_QUADS);
            glVertex3d(2000,2000,20000);
            glVertex3d(2500,2000,20000);
            glVertex3d(2000,2500,20000);
            glVertex3d(2000,2000,20500);
            glEnd();

                 //GLfloat light0_dir[] = {-1, -1, 0};

                 //GLfloat angle[] = {30};
                 //GLfloat light0_ambi[]= {0.5, 0.5, 0.5, 0.5};     

/////////////////



///////////////////


                 glLightfv(GL_LIGHT0, GL_SPECULAR, light0_spec);
                 glLightfv(GL_LIGHT1, GL_DIFFUSE, light0_ambi);
                 glLightfv(GL_LIGHT2, GL_AMBIENT, light0_diff);

                 //glLightfv(GL_LIGHT0, GL_SPOT_CUTOFF, angle);
                 //glLightfv(GL_LIGHT1, GL_SPOT_CUTOFF, angle);
                 //glLightfv(GL_LIGHT2, GL_SPOT_CUTOFF, angle);



                 glLightfv(GL_LIGHT0, GL_POSITION, light0_pos);
                 glLightfv(GL_LIGHT1, GL_POSITION, light0_pos);
                 glLightfv(GL_LIGHT2, GL_POSITION, light0_pos);

                 //glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, light0_dir);
                 //glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, light0_dir);
                 //glLightfv(GL_LIGHT2, GL_SPOT_DIRECTION, light0_dir);
 
                 
                 ////glClearColor (0.0, 0.0, 0.0, 0.0);
                 //glShadeModel (GL_SMOOTH);



                 //glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
                 //glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

                 glColorMaterial ( GL_FRONT_AND_BACK, GL_EMISSION ) ;
                 glColorMaterial (GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE) ;
                 glEnable (GL_COLOR_MATERIAL) ;


                 glEnable(GL_NORMALIZE);

                 glEnable(GL_LIGHTING); 
                 //glEnable(GL_LIGHT0);
                 glEnable(GL_LIGHT0);

                 glEnable(GL_DEPTH_TEST);

                 glMatrixMode(GL_MODELVIEW);
            }
            break;

        case GRAFIC_ALIAS:
            //if(graphic[3] == 1){
            if(setting.antia == true){
                //printf("Anti aliasing is off\n");
                //graphic[3] = 0;
                setting.antia = false;
                reshape((int)window_width, (int)window_height);
            }else{
                //printf("Anti aliasing is on\n");
                //graphic[3] = 1;
                setting.antia=true;
                reshape((int)window_width, (int)window_height);
            }
            break;

        case PICKING_MARKER:
            if(setting.picking_highlight==true){
                setting.picking_highlight=false;
            }else{
                setting.picking_highlight=true;
            }
            break;

        case TOGGLE_DETECTOR_PICKING:
            setting.detector_picking = abs(setting.detector_picking-1);
            break; 

        case GRAFIC_FOG:
                glGetDoublev(GL_COLOR_CLEAR_VALUE, setting.bgcolor);
                //GLfloat fogcolor[4]={setting.bgcolor[0],setting.bgcolor[1],setting.bgcolor[2],1.0};   
                GLfloat fogcolor[4];
                fogcolor[0]=setting.bgcolor[0];
                fogcolor[1]=setting.bgcolor[1];
                fogcolor[2]=setting.bgcolor[2];
                fogcolor[3]=0.5;


                glFogfv(GL_FOG_COLOR,fogcolor);          
                glFogf(GL_FOG_DENSITY,0.5);                 
                //glFogi(GL_FOG_MODE,GL_EXP);             

                glFogi(GL_FOG_MODE,GL_LINEAR);            
                glFogf(GL_FOG_START,500.0);              
                glFogf(GL_FOG_END,3000.0);                
                glHint(GL_FOG_HINT, GL_FASTEST);          
                glEnable(GL_FOG);
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
        case SAVE1:
        case SAVE2:
        case SAVE3:
        case SAVE4:
        case SAVE5:
            saveSettings(id-SAVE1+1); 
            updateSaveLoadMenu(id-SAVE1+1);
            break;

        case LOAD1:
        case LOAD2:
        case LOAD3:
        case LOAD4:
        case LOAD5:
            loadSettings(id-LOAD1+1); 
            set_bg_color(setting.bgcolor[0],setting.bgcolor[1],setting.bgcolor[2],setting.bgcolor[3]); 
            break;

        case SAVE_IMAGE1:
            screenshot("/tmp/glced.tga",1);
            reshape((int)window_width, (int)window_height);
            break;
        case SAVE_IMAGE4:
            screenshot("/tmp/glced.tga",4);
            reshape((int)window_width, (int)window_height);
            break;
        case SAVE_IMAGE10:
            screenshot("/tmp/glced.tga",10);
            reshape((int)window_width, (int)window_height);
            break;
        case SAVE_IMAGE20:
            screenshot("/tmp/glced.tga",20);
            reshape((int)window_width, (int)window_height);
            break;
        case SAVE_IMAGE100:
            screenshot("/tmp/glced.tga",100);
            reshape((int)window_width, (int)window_height);
            break;
    }

    //reshape((int)window_width, (int)window_height);


    if(id != 0){
    //buildMainMenu();
        buildLayerMenus();
    }

    glutPostRedisplay();
    //printf("bgcolor = %f %f %f %f\n",setting.bgcolor[0],setting.bgcolor[1],setting.bgcolor[2],setting.bgcolor[2]); 



}

void buildPopUpMenu(int x, int y){
    static GLfloat p_pre_x=0, p_pre_y=0, p_pre_z=0;
    char tmp[200];

    GLfloat p_x, p_y, p_z;
    int id, layer, type;
    
    //delete the old one first!!!

    popupmenu=new CED_PopUpMenu("");
    
    buildLayerMenus();
    popupmenu->addItem(datalayermenu);
    popupmenu->addItem(detectorlayermenu);
    popupmenu->addItem(new CED_SubSubMenu("---",0));
    

    if(!find_selected_object(x,y,&p_x,&p_y,&p_z, &id, &layer, &type)){ //if ==1 found hit, else clicked on background
        //cout << "TODO: ID: " << id << endl;
        //cout << "PICK_HIT" << endl;
        //find_selected_object(popupmenu->x_click,popupmenu->y_click,&pick_point.x,&pick_point.y,&pick_point.z, NULL, NULL, NULL);
        select_nothing=false;
        pick_point.x=p_x;pick_point.y=p_y;pick_point.z=p_z;

        if(type == 0){
            last_selected_layer=layer;
            //popupmenu=new CED_PopUpMenu("Select datapoint");

            popupmenu->addItem(new CED_SubSubMenu("Selected datapoint:",0));
            sprintf(tmp,"Coordinates: (%.1f, %.1f, %1.f)",p_x,p_y,p_z);
            popupmenu->addItem(new CED_SubSubMenu(tmp,0));
            sprintf(tmp,"ID: %i",id);
            popupmenu->addItem(new CED_SubSubMenu(tmp,0));
            sprintf(tmp,"Distance previous selected hit: %.2f",pow(pow(p_pre_x-p_x,2)+pow(p_pre_y-p_y,2)+pow(p_pre_z-p_z,2),0.5));
            popupmenu->addItem(new CED_SubSubMenu(tmp,0));
            sprintf(tmp,"Center object");
            popupmenu->addItem(new CED_SubSubMenu(tmp,CENTER_HIT));
            sprintf(tmp,"Pick object");
            popupmenu->addItem(new CED_SubSubMenu(tmp,PICK_HIT));
            sprintf(tmp,"Hide layer %i: %s)",layer,layerDescription[layer] );
            popupmenu->addItem(new CED_SubSubMenu(tmp,LAYER_0+layer));

            p_pre_x=p_x; 
            p_pre_y=p_y; 
            p_pre_z=p_z;
        }else if(type == 1){
            //popupmenu=new CED_PopUpMenu("Select detector component");

            last_selected_layer=layer;
            sprintf(tmp, "Selected detector: %s (Layer: %i)", layerDescription[layer], layer);
            popupmenu->addItem(new CED_SubSubMenu(tmp,0));

            sprintf(tmp,"Hide layer %i",layer);
            popupmenu->addItem(new CED_SubSubMenu(tmp, layer-NUMBER_DATA_LAYER+DETECTOR1));

            //sprintf(tmp,"Cut detector at this layer",layer, layerDescription[layer]);
            //popupmenu->addItem(new CED_SubSubMenu(tmp, layer-NUMBER_DATA_LAYER+DETECTOR1));
 

            sprintf(tmp,"Coordinates: (%.1f, %.1f, %1.f)",p_x,p_y,p_z);
            popupmenu->addItem(new CED_SubSubMenu(tmp,0));
            sprintf(tmp,"ID: %i",id);
            popupmenu->addItem(new CED_SubSubMenu(tmp,0));
            //sprintf(tmp,"Distance previous selected object: %.2f",pow(pow(p_pre_x-p_x,2)+pow(p_pre_y-p_y,2)+pow(p_pre_z-p_z,2),0.5));
            //popupmenu->addItem(new CED_SubSubMenu(tmp,0));
            sprintf(tmp,"Center object");
            popupmenu->addItem(new CED_SubSubMenu(tmp,CENTER_HIT));
            sprintf(tmp,"Pick object");
            popupmenu->addItem(new CED_SubSubMenu(tmp,PICK_HIT));
            //sprintf(tmp,"Hide layer %i: %s",layer, layerDescription[layer]);
            //popupmenu->addItem(new CED_SubSubMenu(tmp, layer-NUMBER_DATA_LAYER+DETECTOR1));


            snprintf(tmp,199,"Phi cut (%.0f)",setting.detector_cut_angle[layer-NUMBER_DATA_LAYER]);
            CED_SubSubMenu *phicuts=new CED_SubSubMenu(tmp);

            unsigned i;
            char str[200];
            for(i=0; (unsigned)i < sizeof(available_cutangles)/sizeof(available_cutangles[0]); i++){
                    sprintf(str,"Cut of %i degree in phi", available_cutangles[i]);
                    phicuts->addItem(new CED_SubSubMenu(str,  LAYER_CUT_ANGLE0+available_cutangles[i]));
                    //glutChangeToMenuEntry(i+1, str,  CUT_ANGLE0+i);
            }
            popupmenu->addItem(phicuts);

            char tmp[200];
            snprintf(tmp,199,"Z-cut (%.0f)",setting.detector_cut_z[layer-NUMBER_DATA_LAYER]);
            CED_SubSubMenu *zcuts=new CED_SubSubMenu(tmp);
            zcuts->addItem(new CED_SubSubMenu("Cut at z=-6000", LAYER_CUT_Z_M6000));
            zcuts->addItem(new CED_SubSubMenu("Cut at z=-4000", LAYER_CUT_Z_M4000));
            zcuts->addItem(new CED_SubSubMenu("Cut at z=-2000", LAYER_CUT_Z_M2000));
            zcuts->addItem(new CED_SubSubMenu("Cut at z=0",     LAYER_CUT_Z_0000));
            zcuts->addItem(new CED_SubSubMenu("Cut at z=2000",  LAYER_CUT_Z_2000));
            zcuts->addItem(new CED_SubSubMenu("Cut at z=4000",  LAYER_CUT_Z_4000));
            zcuts->addItem(new CED_SubSubMenu("Cut at z=6000",  LAYER_CUT_Z_6000));
            zcuts->addItem(new CED_SubSubMenu("Cut at z=7000",  LAYER_CUT_Z_7000));
            popupmenu->addItem(zcuts);


            //CED_SubMenu *phicuts=new CED_SubMenu("Cuts");
            //cuts->addItem(new CED_SubSubMenu("Cut at z=0",  0));
            //cuts->addItem(new CED_SubSubMenu("Cut at z=3000",  0));
            //cuts->addItem(new CED_SubSubMenu("Cut at z=5000",  0));


            

            snprintf(tmp,199,"Transparency (%.0f)",100*setting.detector_trans[layer-NUMBER_DATA_LAYER]);
            CED_SubSubMenu *trans=new CED_SubSubMenu(tmp);
            trans->addItem(new CED_SubSubMenu("    0%",LAYER_TRANS0));
            trans->addItem(new CED_SubSubMenu("  40%", LAYER_TRANS40));
            trans->addItem(new CED_SubSubMenu("  60%", LAYER_TRANS60));
            trans->addItem(new CED_SubSubMenu("  70%", LAYER_TRANS70));
            trans->addItem(new CED_SubSubMenu("  80%", LAYER_TRANS80));
            trans->addItem(new CED_SubSubMenu("  90%", LAYER_TRANS90));
            trans->addItem(new CED_SubSubMenu("  95%", LAYER_TRANS95));
            trans->addItem(new CED_SubSubMenu("100%",  LAYER_TRANS100));
            popupmenu->addItem(trans);



            p_pre_x=p_x; 
            p_pre_y=p_y; 
            p_pre_z=p_z;
        }
    }else{
           select_nothing=true;


            last_selected_layer=-1;
        //popupmenu=new CED_PopUpMenu("Change background color to:");

        popupmenu->addItem(new CED_SubSubMenu("Change background color to:",0));
        sprintf(tmp,"%s",CED_BGCOLOR_OPTION1_NAME);
        popupmenu->addItem(new CED_SubSubMenu(tmp,BGCOLOR_OPTION1));

        sprintf(tmp,"%s",CED_BGCOLOR_OPTION2_NAME);
        popupmenu->addItem(new CED_SubSubMenu(tmp,BGCOLOR_OPTION2));

        sprintf(tmp,"%s",CED_BGCOLOR_OPTION3_NAME);
        popupmenu->addItem(new CED_SubSubMenu(tmp,BGCOLOR_OPTION3));

        sprintf(tmp,"%s",CED_BGCOLOR_OPTION4_NAME);
        popupmenu->addItem(new CED_SubSubMenu(tmp,BGCOLOR_OPTION4));

        sprintf(tmp,"%s",CED_BGCOLOR_OPTION5_NAME);
        popupmenu->addItem(new CED_SubSubMenu(tmp,BGCOLOR_OPTION5));

        sprintf(tmp,"%s",CED_BGCOLOR_OPTION6_NAME);
        popupmenu->addItem(new CED_SubSubMenu(tmp,BGCOLOR_OPTION6));

        sprintf(tmp,"%s",CED_BGCOLOR_OPTION7_NAME);
        popupmenu->addItem(new CED_SubSubMenu(tmp,BGCOLOR_OPTION7));

        sprintf(tmp,"%s",CED_BGCOLOR_OPTION8_NAME);
        popupmenu->addItem(new CED_SubSubMenu(tmp,BGCOLOR_OPTION8));

        sprintf(tmp,"%s",CED_BGCOLOR_OPTION9_NAME);
        popupmenu->addItem(new CED_SubSubMenu(tmp,BGCOLOR_OPTION9));

        sprintf(tmp,"%s",CED_BGCOLOR_OPTION10_NAME);
        popupmenu->addItem(new CED_SubSubMenu(tmp,BGCOLOR_OPTION10));

        sprintf(tmp,"%s",CED_BGCOLOR_OPTION11_NAME);
        popupmenu->addItem(new CED_SubSubMenu(tmp,BGCOLOR_OPTION11));

        sprintf(tmp,"%s",CED_BGCOLOR_OPTION12_NAME);
        popupmenu->addItem(new CED_SubSubMenu(tmp,BGCOLOR_OPTION12));

        sprintf(tmp,"%s",CED_BGCOLOR_OPTION13_NAME);
        popupmenu->addItem(new CED_SubSubMenu(tmp,BGCOLOR_OPTION13));

        sprintf(tmp,"%s",CED_BGCOLOR_OPTION14_NAME);
        popupmenu->addItem(new CED_SubSubMenu(tmp,BGCOLOR_OPTION14));

        sprintf(tmp,"%s",CED_BGCOLOR_OPTION15_NAME);
        popupmenu->addItem(new CED_SubSubMenu(tmp,BGCOLOR_OPTION15));

//
//
//      if(userDefinedBGColor[0] >= 0){ //is set
//          settings->addItem(new CED_SubSubMenu("User defined",BGCOLOR_USER));
//      }

    }

    int height=10;
    int width=200;
    if(setting.font==0){
        height=10;
        width=150;
    }
    if(setting.font==1){
        height=12;
        width=200;
    }
    if(setting.font==2){
        height=20;
        width=300;
    }

    int pos_y=popupmenu->size()*height;

    popupmenu->isExtend=true;
    
    if( (x + width+10) > window_width){
        popupmenu->x_start=x-width-10;
        popupmenu->x_end=x-10;
    }else{
        popupmenu->x_start=x;
        popupmenu->x_end=x+width;
    }

    if( (y+pos_y) > window_height){
        popupmenu->y_start=y-pos_y;
        popupmenu->y_end=y-pos_y+height+1;
    }else{
        popupmenu->y_start=y;
        popupmenu->y_end=y+height+1;
    }

    popupmenu->y_click=y;
    popupmenu->x_click=x;
   // cout << "TODO: x,y: " << x << ", " << y << endl;
}
void buildLayerMenus(void){
    //std::cout << "enter buildLayerMenus" << std::endl;
    //if(ced_menu != NULL && detectorlayermenu != NULL && datalayermenu != NULL){
    //    delete ced_menu;
    //}

    detectorlayermenu=new CED_SubSubMenu("Detector layers",0); 
    datalayermenu=new CED_SubSubMenu("Data layers",0);
    int i;
    char str[2000];
    unsigned max=150;
    char tmp[max+1];
    for(i=0;i<NUMBER_POPUP_LAYER;i++){
        //std::cout << "description: " << layerDescription[i] << std::endl;
        if(strlen(layerDescription[i]) > max-1){
            snprintf(tmp,max-3,"%s",layerDescription[i]);
            sprintf(tmp,"%s...",tmp);
        }else{
            sprintf(tmp,"%s",layerDescription[i]);
        }

        sprintf(str,"%s %s%i [%c]: %s", isLayerVisible(i)?"[X]":"[ ]", (i < 10)?"  ":"" ,i, layer_keys[i], tmp);
        //std::cout << str << std::endl;
        datalayermenu->addItem(new CED_SubSubMenu(str,LAYER_0+i));
    }
    for(i=NUMBER_DATA_LAYER;i<NUMBER_DETECTOR_LAYER+NUMBER_DATA_LAYER;i++){
        //sprintf(str,"Detector Layer %s%i [%c]: %s", (i < 10)?"  ":"" ,i, layer_keys[i], layerDescription[i]);
        if(strlen(layerDescription[i]) > max){
            snprintf(tmp,max-3,"%s",layerDescription[i]);
            sprintf(tmp,"%s...",tmp);
        }else{
            sprintf(tmp,"%s",layerDescription[i]);
        }

        sprintf(str,"%s %s%i: %s", isLayerVisible(i)?"[X]":"[ ]",(i < 10)?"  ":"" ,i, layerDescription[i]);
        detectorlayermenu->addItem(new CED_SubSubMenu(str,DETECTOR1+i-NUMBER_DATA_LAYER));
    }


    //std::cout << "leave buildLayerMenus" << std::endl;
    buildMainMenu();
}
void buildMainMenu(void){
    //std::cout << "build main menu" << std::endl;
    ced_menu=new CED_Menu();
    //buildLayerMenus();


    char str[200];
    unsigned i;

    //layers
    CED_SubMenu *layers=new CED_SubMenu("Layers");

    //buildLayerMenus();

    if(setting.show_axes == true){
        layers->addItem(new CED_SubSubMenu("[X] Axis", AXES));
    }else{
        layers->addItem(new CED_SubSubMenu("[ ] Axis", AXES));
    }    
        
    layers->addItem(new CED_SubSubMenu("---", AXES));
    bool result=true;
    for(int i=0;i<NUMBER_DATA_LAYER;i++){
        if(setting.layer[i] == false){
            result=false;
            break;
        }
    }
    if(result){
        layers->addItem(new CED_SubSubMenu("[X] Show/Hide all data Layers [`]", LAYER_ALL));
    }else{
        layers->addItem(new CED_SubSubMenu("[ ] Show/Hide all data Layers [`]", LAYER_ALL));
    }

    layers->addItem(datalayermenu);

    layers->addItem(new CED_SubSubMenu("---", AXES));

    result=true;
    for(i=NUMBER_DATA_LAYER;i<NUMBER_DETECTOR_LAYER+NUMBER_DATA_LAYER;i++){
        if(setting.layer[i] == false){
            result=false;
            break;
        }
    }
    if(result){
        layers->addItem(new CED_SubSubMenu("[X] Show/Hide complete detector", DETECTOR_ALL));
    }else{
        layers->addItem(new CED_SubSubMenu("[ ] Show/Hide complete detector", DETECTOR_ALL));
    }

    layers->addItem(detectorlayermenu);
    ced_menu->addSubMenu(layers);

    double tmptrans=setting.detector_trans[0];
    for(i=1;i<NUMBER_DETECTOR_LAYER;i++){
        if(setting.detector_trans[i] != tmptrans){
            tmptrans=-100;
            break;
        }
    }

    CED_SubMenu *trans=new CED_SubMenu("Transparency");
    if(tmptrans == 0){
        trans->addItem(new CED_SubSubMenu("[X]   0%",TRANS0));
    }else{
        trans->addItem(new CED_SubSubMenu("[ ]   0%",TRANS0));
    }

    if(tmptrans == 0.40){
        trans->addItem(new CED_SubSubMenu("[X]  40%",TRANS40));
    }else{
        trans->addItem(new CED_SubSubMenu("[ ]  40%",TRANS40));
    }

    if(tmptrans == 0.60){
        trans->addItem(new CED_SubSubMenu("[X]  60%",TRANS60));
    }else{
        trans->addItem(new CED_SubSubMenu("[ ]  60%",TRANS60));
    }

    if(tmptrans == 0.70){
        trans->addItem(new CED_SubSubMenu("[X]  70%",TRANS70));
    }else{
        trans->addItem(new CED_SubSubMenu("[ ]  70%",TRANS70));
    }

    if(tmptrans == 0.80){
        trans->addItem(new CED_SubSubMenu("[X]  80%",TRANS80));
    }else{
        trans->addItem(new CED_SubSubMenu("[ ]  80%",TRANS80));
    }


    if(tmptrans == 0.90){
        trans->addItem(new CED_SubSubMenu("[X]  90%",TRANS90));
    }else{
        trans->addItem(new CED_SubSubMenu("[ ]  90%",TRANS90));
    }


    if(tmptrans == 0.95){
        trans->addItem(new CED_SubSubMenu("[X]  95%",TRANS95));
    }else{
        trans->addItem(new CED_SubSubMenu("[ ]  95%",TRANS95));
    }


    if(tmptrans == 1.00){
        trans->addItem(new CED_SubSubMenu("[X] 100%",TRANS100));
    }else{
        trans->addItem(new CED_SubSubMenu("[ ] 100%",TRANS100));
    }
    ced_menu->addSubMenu(trans);

    CED_SubMenu *camera=new CED_SubMenu("Camera");
    camera->addItem(new CED_SubSubMenu("Reset view [r]", VIEW_RESET));
    camera->addItem(new CED_SubSubMenu("Reset CED  [R]", CED_RESET));
    camera->addItem(new CED_SubSubMenu("Front view [f]", VIEW_FRONT));
    camera->addItem(new CED_SubSubMenu("Side view [s]",  VIEW_SIDE));

    camera->addItem(new CED_SubSubMenu("---", 0));
    if(setting.phi_projection==true){
        camera->addItem(new CED_SubSubMenu("[X] Toggle side view projection [S]", TOGGLE_PHI_PROJECTION));
    }else{
        camera->addItem(new CED_SubSubMenu("[ ] Toggle side view projection [S]", TOGGLE_PHI_PROJECTION));
    }
    if(setting.z_projection==true){
        camera->addItem(new CED_SubSubMenu("[X] Toggle front view projection [F]", TOGGLE_Z_PROJECTION));
    }else{
        camera->addItem(new CED_SubSubMenu("[ ] Toggle front view projection [F]", TOGGLE_Z_PROJECTION));
    }
    if(fisheye_alpha > 0){
        camera->addItem(new CED_SubSubMenu("[X] Toggle fisheye projection [v]",VIEW_FISHEYE));
    }else{
        camera->addItem(new CED_SubSubMenu("[ ] Toggle fisheye projection [v]",VIEW_FISHEYE));
    }

    camera->addItem(new CED_SubSubMenu("---", 0));
    camera->addItem(new CED_SubSubMenu("Zoom in [+]", VIEW_ZOOM_IN));
    camera->addItem(new CED_SubSubMenu("Zoom out [-]", VIEW_ZOOM_OUT));
    ced_menu->addSubMenu(camera);




    double tmpcut=setting.detector_cut_angle[0];
    for(i=1;i<NUMBER_DETECTOR_LAYER;i++){
        if(setting.detector_cut_angle[i] != tmpcut){
            tmpcut=-100;
            break;
        }
    }

    CED_SubMenu *cuts=new CED_SubMenu("Cuts");
    for(i=0; (unsigned)i < sizeof(available_cutangles)/sizeof(available_cutangles[0]); i++){
            if(available_cutangles[i] == tmpcut){
                sprintf(str,"[X] Cut of %i degree in phi", available_cutangles[i]);
            }else{
                sprintf(str,"[ ] Cut of %i degree in phi", available_cutangles[i]);
            }
            cuts->addItem(new CED_SubSubMenu(str,  CUT_ANGLE0+available_cutangles[i]));
            //glutChangeToMenuEntry(i+1, str,  CUT_ANGLE0+i);
    }

    //CED_SubSubMenu *zcuts=new CED_SubSubMenu("Z cut");

    tmpcut=setting.detector_cut_z[0];
    for(i=1;i<NUMBER_DETECTOR_LAYER;i++){
        if(setting.detector_cut_z[i] != tmpcut){
            tmpcut=-99999999;
            break;
        }
    }

    cuts->addItem(new CED_SubSubMenu("---", 0));
    if(tmpcut == -6000){
        cuts->addItem(new CED_SubSubMenu("[X] Cut at z=-6000", CUT_Z_M6000));
    }else{
        cuts->addItem(new CED_SubSubMenu("[ ] Cut at z=-6000", CUT_Z_M6000));
    }

    if(tmpcut == -4000){
        cuts->addItem(new CED_SubSubMenu("[X] Cut at z=-4000", CUT_Z_M4000));
    }else{
        cuts->addItem(new CED_SubSubMenu("[ ] Cut at z=-4000", CUT_Z_M4000));
    }

    if(tmpcut == -2000){
        cuts->addItem(new CED_SubSubMenu("[X] Cut at z=-2000", CUT_Z_M2000));
    }else{
        cuts->addItem(new CED_SubSubMenu("[ ] Cut at z=-2000", CUT_Z_M2000));
    }

    if(tmpcut == 0){
        cuts->addItem(new CED_SubSubMenu("[X] Cut at z=0", CUT_Z_0000));
    }else{
        cuts->addItem(new CED_SubSubMenu("[ ] Cut at z=0", CUT_Z_0000));
    }
    if(tmpcut == 2000){
        cuts->addItem(new CED_SubSubMenu("[X] Cut at z=2000", CUT_Z_2000));
    }else{
        cuts->addItem(new CED_SubSubMenu("[ ] Cut at z=2000", CUT_Z_2000));
    }
    if(tmpcut == 4000){
        cuts->addItem(new CED_SubSubMenu("[X] Cut at z=4000", CUT_Z_4000));
    }else{
        cuts->addItem(new CED_SubSubMenu("[ ] Cut at z=4000", CUT_Z_4000));
    }

    if(tmpcut == 6000){
        cuts->addItem(new CED_SubSubMenu("[X] Cut at z=6000", CUT_Z_6000));
    }else{
        cuts->addItem(new CED_SubSubMenu("[ ] Cut at z=6000", CUT_Z_6000));
    }

    if(tmpcut == 7000){
        cuts->addItem(new CED_SubSubMenu("[X] Cut at z=7000", CUT_Z_7000));
    }else{
        cuts->addItem(new CED_SubSubMenu("[ ] Cut at z=7000", CUT_Z_7000));
    }

    ced_menu->addSubMenu(cuts);


    CED_SubMenu *settings=new CED_SubMenu("Graphic");
    if(setting.trans==false && setting.persp==false){
        settings->addItem(new CED_SubSubMenu("[X] Graphic low",GRAFIC_LOW));
    }else{
        settings->addItem(new CED_SubSubMenu("[ ] Graphic low",GRAFIC_LOW));
    }


    if(setting.trans==true && setting.persp==true){
        settings->addItem(new CED_SubSubMenu("[X] Graphic high",GRAFIC_HIGH));
    }else{
        settings->addItem(new CED_SubSubMenu("[ ] Graphic high",GRAFIC_HIGH));
    }

    settings->addItem(new CED_SubSubMenu("---",0));
    if(setting.persp){
        settings->addItem(new CED_SubSubMenu("[X] Toggle perspective",GRAFIC_PERSP));
    }else{
        settings->addItem(new CED_SubSubMenu("[ ] Toggle perspective",GRAFIC_PERSP));
    }
    if(setting.trans){
        settings->addItem(new CED_SubSubMenu("[X] Toggle wireframe",GRAFIC_TRANS));
    }else{
        settings->addItem(new CED_SubSubMenu("[ ] Toggle wireframe",GRAFIC_TRANS));
    }
    if(setting.light){
        settings->addItem(new CED_SubSubMenu("[X] Light", GRAFIC_LIGHT));
    }else{
        settings->addItem(new CED_SubSubMenu("[ ] Light", GRAFIC_LIGHT));
    }
    if(setting.antia){
        settings->addItem(new CED_SubSubMenu("[X] Anti Aliasing", GRAFIC_ALIAS));
    }else{
        settings->addItem(new CED_SubSubMenu("[ ] Anti Aliasing", GRAFIC_ALIAS));
    }

    if(setting.picking_highlight){
        settings->addItem(new CED_SubSubMenu("[X] Picking marker", PICKING_MARKER));
    }else{
        settings->addItem(new CED_SubSubMenu("[ ] Picking marker", PICKING_MARKER));
    }


    settings->addItem(new CED_SubSubMenu("Fade far objects",GRAFIC_FOG));
    settings->addItem(new CED_SubSubMenu("Deepbuffer", GRAFIC_BUFFER));

    settings->addItem(new CED_SubSubMenu("---",0));
    if(setting.detector_picking){
        settings->addItem(new CED_SubSubMenu("[X] Detector picking", TOGGLE_DETECTOR_PICKING));
    }else{
        settings->addItem(new CED_SubSubMenu("[ ] Detector picking", TOGGLE_DETECTOR_PICKING));
    }
    settings->addItem(new CED_SubSubMenu("---",0));

    CED_SubSubMenu *font=new CED_SubSubMenu("Text font size ");
    if(setting.font == 0){
        font->addItem(new CED_SubSubMenu("[X] Tiny",FONT0));
    }else{
        font->addItem(new CED_SubSubMenu("[ ] Tiny",FONT0));
    }
    if(setting.font == 1){
        font->addItem(new CED_SubSubMenu("[X] Normal",FONT1));
    }else{
        font->addItem(new CED_SubSubMenu("[ ] Normal",FONT1));
    }
    if(setting.font == 2){
        font->addItem(new CED_SubSubMenu("[X] Big",FONT2));
    }else{
        font->addItem(new CED_SubSubMenu("[ ] Big",FONT2));
    }
    settings->addItem(font);




    CED_SubSubMenu *background=new CED_SubSubMenu("Change background color");
    //sprintf(str,"Change background color to: %s",CED_BGCOLOR_OPTION1_NAME);
    //background->addItem(new CED_SubSubMenu(str,BGCOLOR_OPTION1));
    sprintf(str,"%s",CED_BGCOLOR_OPTION2_NAME);
    background->addItem(new CED_SubSubMenu(str,BGCOLOR_OPTION2));
    sprintf(str,"%s",CED_BGCOLOR_OPTION3_NAME);
    background->addItem(new CED_SubSubMenu(str,BGCOLOR_OPTION3));
    sprintf(str,"%s",CED_BGCOLOR_OPTION4_NAME);
    background->addItem(new CED_SubSubMenu(str,BGCOLOR_OPTION4));
    sprintf(str,"%s",CED_BGCOLOR_OPTION5_NAME);
    background->addItem(new CED_SubSubMenu(str,BGCOLOR_OPTION5));
    sprintf(str,"%s",CED_BGCOLOR_OPTION6_NAME);
    background->addItem(new CED_SubSubMenu(str,BGCOLOR_OPTION6));
    sprintf(str,"%s",CED_BGCOLOR_OPTION7_NAME);
    background->addItem(new CED_SubSubMenu(str,BGCOLOR_OPTION7));
    sprintf(str,"%s",CED_BGCOLOR_OPTION8_NAME);
    background->addItem(new CED_SubSubMenu(str,BGCOLOR_OPTION8));
    sprintf(str,"%s",CED_BGCOLOR_OPTION9_NAME);
    background->addItem(new CED_SubSubMenu(str,BGCOLOR_OPTION9));
    sprintf(str,"%s",CED_BGCOLOR_OPTION10_NAME);
    background->addItem(new CED_SubSubMenu(str,BGCOLOR_OPTION10));
    sprintf(str,"%s",CED_BGCOLOR_OPTION11_NAME);
    background->addItem(new CED_SubSubMenu(str,BGCOLOR_OPTION11));
    sprintf(str,"%s",CED_BGCOLOR_OPTION12_NAME);
    background->addItem(new CED_SubSubMenu(str,BGCOLOR_OPTION12));
    sprintf(str,"%s",CED_BGCOLOR_OPTION13_NAME);
    background->addItem(new CED_SubSubMenu(str,BGCOLOR_OPTION13));
    sprintf(str,"%s",CED_BGCOLOR_OPTION14_NAME);
    background->addItem(new CED_SubSubMenu(str,BGCOLOR_OPTION14));
    sprintf(str,"%s",CED_BGCOLOR_OPTION15_NAME);
    background->addItem(new CED_SubSubMenu(str,BGCOLOR_OPTION15));


    if(userDefinedBGColor[0] >= 0){ //is set
        background->addItem(new CED_SubSubMenu("User defined",BGCOLOR_USER));
    }
    

    settings->addItem(new CED_SubSubMenu("---",0));
    settings->addItem(background);
    settings->addItem(new CED_SubSubMenu("---",0));

    CED_SubSubMenu *save=new CED_SubSubMenu("Save settings");
    CED_SubSubMenu *load=new CED_SubSubMenu("Load settings");
    save->addItem(new CED_SubSubMenu("Save into slot 1",SAVE1));
    save->addItem(new CED_SubSubMenu("Save into slot 2",SAVE2));
    save->addItem(new CED_SubSubMenu("Save into slot 3",SAVE3));
    save->addItem(new CED_SubSubMenu("Save into slot 4",SAVE4));
    save->addItem(new CED_SubSubMenu("Save into slot 5",SAVE5));

    load->addItem(new CED_SubSubMenu("Load settings 1",LOAD1));
    load->addItem(new CED_SubSubMenu("Load settings 2",LOAD2));
    load->addItem(new CED_SubSubMenu("Load settings 3",LOAD3));
    load->addItem(new CED_SubSubMenu("Load settings 4",LOAD4));
    load->addItem(new CED_SubSubMenu("Load settings 5",LOAD5));


    settings->addItem(load);
    settings->addItem(save);

    ced_menu->addSubMenu(settings);

    char tmp[200];
    CED_SubSubMenu *screenshot=new CED_SubSubMenu("Save screenshot");
    sprintf(tmp,"original size (%i x %i)", int(setting.win_w), int(setting.win_h));
    screenshot->addItem(new CED_SubSubMenu(tmp,SAVE_IMAGE1));

    sprintf(tmp,"large (%i x %i)", int(4*setting.win_w), int(4*setting.win_h));
    screenshot->addItem(new CED_SubSubMenu(tmp,SAVE_IMAGE4));

    sprintf(tmp,"very large (%i x %i)", int(10*setting.win_w), int(10*setting.win_h));
    screenshot->addItem(new CED_SubSubMenu(tmp,SAVE_IMAGE10));

    sprintf(tmp,"very very large (%i x %i)", int(20*setting.win_w), int(20*setting.win_h));
    screenshot->addItem(new CED_SubSubMenu(tmp,SAVE_IMAGE20));

    sprintf(tmp,"extrem large (%i x %i)", int(100*setting.win_w), int(100*setting.win_h));
    screenshot->addItem(new CED_SubSubMenu(tmp,SAVE_IMAGE100));

    CED_SubMenu *tools=new CED_SubMenu("Tools");
    tools->addItem(screenshot);
    tools->addItem(new CED_SubSubMenu("---",0));
    if(setting.fps){
        tools->addItem(new CED_SubSubMenu("[X] Show FPS",FPS));
    }else{
        tools->addItem(new CED_SubSubMenu("[ ] Show FPS",FPS));
    }
    ced_menu->addSubMenu(tools);


    CED_SubMenu *help=new CED_SubMenu("Help");
    if(showHelp){
        help->addItem(new CED_SubSubMenu("[X] Show keyboard shortcuts",HELP));
    }else{
        help->addItem(new CED_SubSubMenu("[ ] Show keyboard shortcuts",HELP));
    }
    help->addItem(new CED_SubSubMenu("---",0));
    help->addItem(new CED_SubSubMenu("Contact CED team (hauke.hoelbe@desy.de)",0));
    ced_menu->addSubMenu(help);
}


int buildMenuPopup(void){ //hauke
    int subMenu3;
    int DetectorComponents;
    int bgColorMenu = glutCreateMenu(selectFromMenu);

    glutAddMenuEntry(CED_BGCOLOR_OPTION1_NAME,BGCOLOR_OPTION1);
    glutAddMenuEntry(CED_BGCOLOR_OPTION2_NAME,BGCOLOR_OPTION2);
    glutAddMenuEntry(CED_BGCOLOR_OPTION3_NAME,BGCOLOR_OPTION3);
    glutAddMenuEntry(CED_BGCOLOR_OPTION4_NAME,BGCOLOR_OPTION4);
    glutAddMenuEntry(CED_BGCOLOR_OPTION5_NAME,BGCOLOR_OPTION5);
    glutAddMenuEntry(CED_BGCOLOR_OPTION6_NAME,BGCOLOR_OPTION6);
    glutAddMenuEntry(CED_BGCOLOR_OPTION7_NAME,BGCOLOR_OPTION7);
    glutAddMenuEntry(CED_BGCOLOR_OPTION8_NAME,BGCOLOR_OPTION8);
    glutAddMenuEntry(CED_BGCOLOR_OPTION9_NAME,BGCOLOR_OPTION9);
    glutAddMenuEntry(CED_BGCOLOR_OPTION10_NAME,BGCOLOR_OPTION10);
    glutAddMenuEntry(CED_BGCOLOR_OPTION11_NAME,BGCOLOR_OPTION11);
    glutAddMenuEntry(CED_BGCOLOR_OPTION12_NAME,BGCOLOR_OPTION12);
    glutAddMenuEntry(CED_BGCOLOR_OPTION13_NAME,BGCOLOR_OPTION13);
    glutAddMenuEntry(CED_BGCOLOR_OPTION14_NAME,BGCOLOR_OPTION14);
    glutAddMenuEntry(CED_BGCOLOR_OPTION15_NAME,BGCOLOR_OPTION15);


    if(userDefinedBGColor[0] >= 0){ //is set
        glutAddMenuEntry("User defined",BGCOLOR_USER);
    }

    int cameraMenu = glutCreateMenu(selectFromMenu);
    glutAddMenuEntry("Reset view [r]", VIEW_RESET);
    glutAddMenuEntry("Front view [f]", VIEW_FRONT);
    glutAddMenuEntry("Side view [s]", VIEW_SIDE);
    glutAddMenuEntry("Toggle side view projection [S]", TOGGLE_PHI_PROJECTION);
    glutAddMenuEntry("Toggle front view projection [F]", TOGGLE_Z_PROJECTION);
    glutAddMenuEntry("Toggle fisheye projection [v]",VIEW_FISHEYE);
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


    subsubMenu2 = glutCreateMenu(selectFromMenu);
    for(i=0; (unsigned) i < sizeof(available_cutangles) / sizeof(available_cutangles[0]); i++){
        glutAddMenuEntry(" ",  CUT_ANGLE0+i);
    }

    update_cut_angle_menu();


    int transMenu=glutCreateMenu(selectFromMenu);
    glutAddMenuEntry("  0%",TRANS0);
    glutAddMenuEntry("40%",TRANS40);
    glutAddMenuEntry("60%",TRANS60);
    glutAddMenuEntry("70%",TRANS70);
    glutAddMenuEntry("80%",TRANS80);
    glutAddMenuEntry("90%",TRANS90);
    glutAddMenuEntry("95%",TRANS95);
    glutAddMenuEntry("100%",TRANS100);


    subSave=glutCreateMenu(selectFromMenu);
    glutAddMenuEntry("Slot 1",SAVE1);
    glutAddMenuEntry("Slot 2",SAVE2);
    glutAddMenuEntry("Slot 3",SAVE3);
    glutAddMenuEntry("Slot 4",SAVE4);
    glutAddMenuEntry("Slot 5",SAVE5);


    subLoad=glutCreateMenu(selectFromMenu);
    glutAddMenuEntry("Slot 1",LOAD1);
    glutAddMenuEntry("Slot 2",LOAD2);
    glutAddMenuEntry("Slot 3",LOAD3);
    glutAddMenuEntry("Slot 4",LOAD4);
    glutAddMenuEntry("Slot 5",LOAD5);

    for(int i=1;i<=5;i++){
        updateSaveLoadMenu(i);
    }


    subscreenshot=glutCreateMenu(selectFromMenu);
    glutAddMenuEntry("...",SAVE_IMAGE1);
    glutAddMenuEntry("...",SAVE_IMAGE4);
    glutAddMenuEntry("...",SAVE_IMAGE10);
    glutAddMenuEntry("...",SAVE_IMAGE20);
    glutAddMenuEntry("...",SAVE_IMAGE100);
    updateScreenshotMenu();


    int graphicDetailsMenu=glutCreateMenu(selectFromMenu);
    glutAddMenuEntry("Toggle perspective",GRAFIC_PERSP);
    glutAddMenuEntry("Toggle wireframe",GRAFIC_TRANS);
    glutAddMenuEntry("Fade far objects into background color",GRAFIC_FOG);
    glutAddMenuEntry("Deepbuffer", GRAFIC_BUFFER);
    glutAddMenuEntry("Transparency/mesh", GRAFIC_TRANS);
    glutAddMenuEntry("Light", GRAFIC_LIGHT);
    glutAddMenuEntry("Anti Aliasing", GRAFIC_ALIAS);




    int graphicMenu=glutCreateMenu(selectFromMenu);
    glutAddMenuEntry("Graphic low",GRAFIC_LOW);
    glutAddMenuEntry("Graphic high",GRAFIC_HIGH);
    glutAddSubMenu("Details",graphicDetailsMenu);
    glutAddSubMenu("Change background color",bgColorMenu);



    int toolMenu=glutCreateMenu(selectFromMenu);
    glutAddSubMenu("Screenshot",subscreenshot);
    glutAddMenuEntry("Show FPS",FPS);




    int menu=glutCreateMenu(selectFromMenu);

    //int visiMenu=glutCreateMenu(selectFromMenu);
    glutAddSubMenu("Data layer",subMenu3);
    glutAddSubMenu("Detector components",detectorMenu);
    glutAddSubMenu("Detector cuts",subsubMenu2);
    glutAddSubMenu("Detector transparency",transMenu);
    glutAddMenuEntry("Toggle axes",AXES);
    glutAddSubMenu("Graphic settings",graphicMenu);
    glutAddSubMenu("Save current settings",subSave);
    glutAddSubMenu("Load settings",subLoad);
    glutAddSubMenu("Camera",cameraMenu);
    glutAddMenuEntry("Show Keybinding [h]", HELP);
    glutAddSubMenu("Tools",toolMenu);

    return menu;
}


int main(int argc,char *argv[]){
    bool geometry = false;

    mm_reset=mm;
    WORLD_SIZE = DEFAULT_WORLD_SIZE ;

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

    loadSettings(1);  
    setting.screenshot_sections=1;


    //set_bg_color(setting.bgcolor[0],setting.bgcolor[1],setting.bgcolor[2],setting.bgcolor[2]); //set to default (black)=0;

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
          } else {
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



    set_bg_color(setting.bgcolor[0],setting.bgcolor[1],setting.bgcolor[2],setting.bgcolor[2]); //set to default (black)
    //glClearColor(BG_COLOR[0],BG_COLOR[1], BG_COLOR[2], BG_COLOR[3]);
    init();

    #ifndef __APPLE__
    //glutMouseWheelFunc(mouseWheel); //dont works under mac os!
    #endif
  
    glutMouseFunc(mouse);
    glutPassiveMotionFunc(mouse_passive);

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
  
    //workaraound for franks mac
    buildMenuPopup(); 


    buildLayerMenus();
    buildMainMenu(); 
    popupmenu=new CED_PopUpMenu("");

    //glutAttachMenu(GLUT_RIGHT_BUTTON); 
    //for(i=0;i<NUMBER_POPUP_LAYER;i++){ //fill the layer section
    //  updateLayerEntryInPopupMenu(i);
    //}
    //for(i=NUMBER_DATA_LAYER;i<NUMBER_DETECTOR_LAYER+NUMBER_DATA_LAYER;i++){ //fill the layer section
    //  updateLayerEntryDetector(i);
    //}

    glutTimerFunc(500,timer,1);
  
    //glDisable(GL_BLEND);
    if(setting.light == true){
        setting.light=false;
        selectFromMenu(GRAFIC_LIGHT); 
    }


    setting_old[0]=setting;
    setting_old[1]=setting;
    setting_old[2]=setting;
    setting_old[3]=setting;
    setting_old[4]=setting;




    glutMainLoop();
    return 0;
}

int save_pixmap_as_tga(unsigned char *buffer_all,char *name,int wi, int hi){
    //based on: http://www.opengl.org/discussion_boards/ubbthreads.php?ubb=showflat&Number=44286

    int header_size=24;
    int mem_size = wi*hi*3;
    //unsigned char tmp;
    FILE *out_file;
    unsigned char *header;

    if (!(header = (unsigned char *) calloc(1, header_size))) { return(-1); }

    //write header
    header[2] = 2;  // uncompressed
    header[12] = wi & 255;
    header[13] = wi >> 8;
    header[14] = hi & 255;
    header[15] = hi >> 8;
    header[16] = 24;    // 24 bits per pix

    if (!(out_file = fopen(name, "wb"))) { return(-2); }

    fwrite(header, sizeof(unsigned char), header_size, out_file);
    fwrite(buffer_all, sizeof(unsigned char), mem_size, out_file);

    fclose(out_file);
    return(0);
}


int save_pixmap_as_bmp(unsigned char *buffer_all,char *name,unsigned int wi, unsigned int hi){
    unsigned int mem_size = wi*hi*3;
    FILE *out_file;
    unsigned char *header;

    unsigned int header_size=26;

    cout << endl << "               bmp screenshot width: " << wi << " height: " << hi << endl;
    if (!(header = (unsigned char *) calloc(1, header_size))) { return(-1); }

    header[0]  = 'B';
    header[1]  = 'M';
    header[2]  = (mem_size+header_size)          & 255;
    header[3]  = ((mem_size+header_size)  >> 8)  & 255;
    header[4]  = ((mem_size+header_size)  >> 16) & 255;
    header[5]  = ((mem_size+header_size)  >> 24) & 255;
    header[6]  = 0;
    header[7]  = 0;
    header[8]  = 0;
    header[9]  = 0;
    header[10] = header_size;
    header[11] = 0;
    header[12] = 0;
    header[13] = 0;
    header[14] = 12;
    header[15] = 0;
    header[16] = 0;
    header[17] = 0;
    header[18] = (unsigned char)(wi & 255);      
    header[19] = (unsigned char)((wi >> 8) & 255);
    header[20] = (unsigned char)(hi & 255);      
    header[21] = (unsigned char)((hi >> 8) & 255);
    header[22] = 1;
    header[23] = 0;
    header[24] = 24;
    header[25] = 0;

    std::cout << "height: " << int((header[21] << 8) + header[20]) << endl;
    std::cout << "wight: " << int((header[19] << 8)+  header[18]) << endl;

    if (!(out_file = fopen(name, "wb"))) { return(-2); }

    fwrite(header, sizeof(unsigned char), header_size, out_file);
    fwrite(buffer_all, sizeof(unsigned char), mem_size, out_file);

    fclose(out_file);
    return(0);
}

void screenshot(char *name, int times)
{
    if(times > 100){
        std::cout << "Sorry 100x100 are the max value" << std::cout;
        return;
    }

    setting.screenshot_sections=times;


    //int HEADER_SIZE=24;
    unsigned char *buffer_all;
    unsigned char *buffer[100*100];

    //char filename[100];

    int w=glutGet(GLUT_WINDOW_WIDTH);
    int h=glutGet(GLUT_WINDOW_HEIGHT);

    window_width=w;
    window_height=h;

    int buf_size = (w*h*3);

    std::cout << "Generating screenshot (" << w*times << "x" << h*times << "):" << std::endl;
    

    //int buf_size_all = HEADER_SIZE + w*h*3 *times*times;

    int buf_size_all = w*h*3 *times*times;


    std::cout << "    Requesting memory ";
    for(int i=0;i<times*times;i++){
        std::cout << ".";
        std::cout.flush();
        if (!(buffer[i] = (unsigned char *) calloc(1, buf_size)))
        {
            return;
        }
    }
    if (!(buffer_all = (unsigned char *) calloc(1, buf_size_all)))
    {
        return;
    }
    std::cout << " Done" << std::endl;
    

    std::cout << "    Generating image ";
    if(setting.persp == true){
        glTranslatef(0.0, 0.0, +2000); //HOTFIX!!! TODO: find the place where this translation is made 
        double near_plane=200.;
        for(int i=0;i<times;i++){
            for(int j=0;j<times;j++){

                std::cout << ".";
                std::cout.flush();

                glMatrixMode(GL_PROJECTION);
                glLoadIdentity();

                glViewport(0,0,w,h);

                if(w >= h){
                glFrustum((-1*near_plane/2.      + 2*i*(near_plane/2.)/times)*w*1.0/h,
                          (-1*near_plane/2.      +(i+1)*2*(near_plane/2.)/times)*w*1.0/h,
                          (-1*(near_plane/2.)    + 2*j*(near_plane/2.)/times), 
                          (-1*(near_plane/2.)    +(j+1)*2*(near_plane/2.)/times),
                          near_plane ,50000.0*mm.sf*2+50000/(mm.sf*2));
                }else{
                glFrustum((-1*near_plane/2.      + 2*i*(near_plane/2.)/times),
                          (-1*near_plane/2.      +(i+1)*2*(near_plane/2.)/times),
                          (-1*(near_plane/2.)    + 2*j*(near_plane/2.)/times)*h*1.0/w, 
                          (-1*(near_plane/2.)    +(j+1)*2*(near_plane/2.)/times)*h*1.0/w,
                          near_plane*h*1./w ,50000.0*mm.sf*2+50000/(mm.sf*2));

                }

                glViewport(0,0,w,h);
                gluLookAt  (0,0,2000,    0,0,0,    0,1,0);
                glViewport(0,0,w,h);
 
                glMatrixMode(GL_MODELVIEW);
                write_world_into_front_buffer();

                glViewport(0,0,w,h);

                glPixelStorei( GL_PACK_ALIGNMENT, 1 );
                glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, buffer[i+j*times]);

                glMatrixMode(GL_MODELVIEW);
            }
        }
     }else{


        double near_plane=200./(WORLD_SIZE/10);

        for(int i=0;i<times;i++){
            for(int j=0;j<times;j++){

                std::cout << ".";
                std::cout.flush();

                glMatrixMode(GL_PROJECTION);
                glLoadIdentity();

                glViewport(0,0,w,h);

                if(w >= h){


                glOrtho((-1*near_plane/2.      + 2*i*(near_plane/2.)/times)*w*1.0/h*WORLD_SIZE,
                          (-1*near_plane/2.      +(i+1)*2*(near_plane/2.)/times)*w*1.0/h*WORLD_SIZE,
                          (-1*(near_plane/2.)    + 2*j*(near_plane/2.)/times)*WORLD_SIZE, 
                          (-1*(near_plane/2.)    +(j+1)*2*(near_plane/2.)/times)*WORLD_SIZE,
                          near_plane ,(50000.0*mm.sf*2+50000/(mm.sf*2)));
                }else{

                //near_plane*=1./(h*1./w);
                double tmp2=WORLD_SIZE;
                WORLD_SIZE*=w*1./h;
                glOrtho((-1*near_plane/2.      + 2*i*(near_plane/2.)/times)*WORLD_SIZE,
                          (-1*near_plane/2.      +(i+1)*2*(near_plane/2.)/times)*WORLD_SIZE,
                          (-1*(near_plane/2.)    + 2*j*(near_plane/2.)/times)*h*1.0/w*WORLD_SIZE, 
                          (-1*(near_plane/2.)    +(j+1)*2*(near_plane/2.)/times)*h*1.0/w*WORLD_SIZE,
                          near_plane,(50000.0*mm.sf*2+50000/(mm.sf*2)));

                WORLD_SIZE=tmp2;

                }

                glViewport(0,0,w,h);
                gluLookAt  (0,0,2000,    0,0,0,    0,1,0);
                glViewport(0,0,w,h);
 
                glMatrixMode(GL_MODELVIEW);
                write_world_into_front_buffer();

                glViewport(0,0,w,h);

                glPixelStorei( GL_PACK_ALIGNMENT, 1 );
                glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, buffer[i+j*times]);

                glMatrixMode(GL_MODELVIEW);
            }
        }
    }
    std::cout << " Done" << std::endl;

    for(int k=0;k<times;k++){
        for(int l=0;l<h;l++){
            for(int j=0;j<times;j++){
                for(int i=0; i < w*3; i++){
                    buffer_all[i+w*3*j+l*w*3*times+k*times*w*h*3]=buffer[j+times*k][i+l*w*3];
                }
            }
        }
    }

    //RGB -> BGR
    char tmp;
    for(int j=0;j<buf_size_all;j+=3){
        tmp = buffer_all[j];
        buffer_all[j] = buffer_all[j+2];
        buffer_all[j+2] = tmp;
    }

    std::cout << "    Save screenshot as: " << name ;
    cout.flush();
    //save_pixmap_as_tga(buffer_all, name, w*times, h*times);
    //save_pixmap_as_bmp(buffer_all, "/tmp/glced.bmp", w*times, h*times);
    save_pixmap_as_tga(buffer_all, "/tmp/glced.tga", w*times, h*times);
    std::cout << " Done" << endl ;

    std::cout << "    Clean memory ";
    for(int i=0;i<times*times;i++){
        std::cout << ".";
        std::cout.flush();
        free(buffer[i]);
    }
    free(buffer_all);
    std::cout << " Done" << std::endl;

    setting.screenshot_sections=1;
}

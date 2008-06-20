/* "C" event display.
 * Server side elements definitions.
 *
 * Alexey Zhelezov, DESY/ITEP, 2005 */

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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <ced_cli.h>
#include <ced.h>

// what is visible
unsigned ced_visible_layers=0xffffffff;

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
  return 0;
}

/*
 * To be called from drawing functions
 */
static void ced_add_objmap(CED_Point *p,int max_dxy, unsigned int ID){
  GLdouble winx,winy,winz;
  if(omap_count==omap_alloced){
    omap_alloced+=256;
    omap=realloc(omap,omap_alloced*sizeof(CED_ObjMap));
  }
  if(gluProject((GLdouble)p->x,(GLdouble)p->y,(GLdouble)p->z,
		modelM,projM,viewport,&winx,&winy,&winz)!=GL_TRUE)
    return;
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
	      glVertex3f(h->p.x-d,h->p.y-d,h->p.z+d);
	      glVertex3f(h->p.x+d,h->p.y+d,h->p.z-d);

	      glVertex3f(h->p.x+d,h->p.y-d,h->p.z+d);
	      glVertex3f(h->p.x-d,h->p.y+d,h->p.z-d);

	      glVertex3f(h->p.x+d,h->p.y+d,h->p.z+d);
	      glVertex3f(h->p.x-d,h->p.y-d,h->p.z-d);

	      glVertex3f(h->p.x-d,h->p.y+d,h->p.z+d);
	      glVertex3f(h->p.x+d,h->p.y-d,h->p.z-d);

	    } else {
	      //	      printf("star type == %d \n",(h->type & CED_HIT_STAR));
	      d=h->size/2.;
	      glVertex3f(h->p.x-d,h->p.y,h->p.z);
	      glVertex3f(h->p.x+d,h->p.y,h->p.z);
	      glVertex3f(h->p.x,h->p.y-d,h->p.z);
	      glVertex3f(h->p.x,h->p.y+d,h->p.z);
	      glVertex3f(h->p.x,h->p.y,h->p.z-d);
	      glVertex3f(h->p.x,h->p.y,h->p.z+d);
	    }
	    break;
	default:
	    glPointSize((GLfloat)h->size);
	    glBegin(GL_POINTS);
	    glVertex3fv(&h->p.x);
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
  if(!IS_VISIBLE(h->type))
    return;
  ced_color(h->color);
  glLineWidth(h->width);
  glBegin(GL_LINES);
  glVertex3fv(&h->p0.x);
  glVertex3fv(&h->p1.x);
  glEnd();
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

  glTranslatef(0.0, 0.0, c->shift);
  if(c->rotate > 0.01 )
      glRotatef(c->rotate, 0, 0, 1);
  gluQuadricNormals(q1, GL_SMOOTH);
  gluQuadricTexture(q1, GL_TRUE);
  gluCylinder(q1, c->d, c->d, c->z*2, c->sides, 1);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  gluDeleteQuadric(q1);

  glPopMatrix();
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
  //  drawing the first face
  glVertex3f( (float) face[0][0][0], (float) face[0][0][1],  (float) face[0][0][2] );
  glVertex3f( (float) face[0][1][0], (float) face[0][1][1],  (float) face[0][1][2] );

  glVertex3f( (float) face[0][1][0], (float) face[0][1][1],  (float) face[0][1][2] );
  glVertex3f( (float) face[0][2][0], (float) face[0][2][1],  (float) face[0][2][2] );

  glVertex3f( (float) face[0][2][0], (float) face[0][2][1],  (float) face[0][2][2] );
  glVertex3f( (float) face[0][3][0], (float) face[0][3][1],  (float) face[0][3][2] );

  glVertex3f( (float) face[0][3][0], (float) face[0][3][1],  (float) face[0][3][2] );
  glVertex3f( (float) face[0][0][0], (float) face[0][0][1],  (float) face[0][0][2] );


  // drawing the sencod face
  glVertex3f( (float) face[1][0][0], (float) face[1][0][1],  (float) face[1][0][2] );
  glVertex3f( (float) face[1][1][0], (float) face[1][1][1],  (float) face[1][1][2] );

  glVertex3f( (float) face[1][1][0], (float) face[1][1][1],  (float) face[1][1][2] );
  glVertex3f( (float) face[1][2][0], (float) face[1][2][1],  (float) face[1][2][2] );

  glVertex3f( (float) face[1][2][0], (float) face[1][2][1],  (float) face[1][2][2] );
  glVertex3f( (float) face[1][3][0], (float) face[1][3][1],  (float) face[1][3][2] );

  glVertex3f( (float) face[1][3][0], (float) face[1][3][1],  (float) face[1][3][2] );
  glVertex3f( (float) face[1][0][0], (float) face[1][0][1],  (float) face[1][0][2] );
  

  // drawing the connections
  glVertex3f( (float) face[0][0][0], (float) face[0][0][1],  (float) face[0][0][2] );
  glVertex3f( (float) face[1][0][0], (float) face[1][0][1],  (float) face[1][0][2] );

  glVertex3f( (float) face[0][1][0], (float) face[0][1][1],  (float) face[0][1][2] );
  glVertex3f( (float) face[1][1][0], (float) face[1][1][1],  (float) face[1][1][2] );

  glVertex3f( (float) face[0][2][0], (float) face[0][2][1],  (float) face[0][2][2] );
  glVertex3f( (float) face[1][2][0], (float) face[1][2][1],  (float) face[1][2][2] );

  glVertex3f( (float) face[0][3][0], (float) face[0][3][1],  (float) face[0][3][2] );
  glVertex3f( (float) face[1][3][0], (float) face[1][3][1],  (float) face[1][3][2] );

  glEnd();






}

/*
 * GeoBoxR 
 */
static unsigned GEOBR_ID = 0;

static void ced_draw_geobox_r(CED_GeoBoxR * box )  {

  if(!IS_VISIBLE(box->layer)){
  // a box has 8 vertices, four belonging to the first surface facing
  // the beam, the other four from the second surface
  const unsigned int nPoint = 4;
  const unsigned int nDim   = 3;
  const unsigned int nFace  = 2;
  double face[nFace][nPoint][nDim];
  //  unsigned int iDim, iPoint, iFace;


  ced_color(box->color);
  glLineWidth(0.5);

  glPushMatrix(); // push the matrix onto the matrix stack and pop it off (preserving the original matrix)

  glTranslatef(box->center[0],box->center[1],box->center[2]);

  glRotatef(box->rotate,0.0, 0.0, 1.0);

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

  glBegin(GL_LINES);
  //  drawing the first face
  glVertex3f( (float) face[0][0][0], (float) face[0][0][1],  (float) face[0][0][2] );
  glVertex3f( (float) face[0][1][0], (float) face[0][1][1],  (float) face[0][1][2] );

  glVertex3f( (float) face[0][1][0], (float) face[0][1][1],  (float) face[0][1][2] );
  glVertex3f( (float) face[0][2][0], (float) face[0][2][1],  (float) face[0][2][2] );

  glVertex3f( (float) face[0][2][0], (float) face[0][2][1],  (float) face[0][2][2] );
  glVertex3f( (float) face[0][3][0], (float) face[0][3][1],  (float) face[0][3][2] );

  glVertex3f( (float) face[0][3][0], (float) face[0][3][1],  (float) face[0][3][2] );
  glVertex3f( (float) face[0][0][0], (float) face[0][0][1],  (float) face[0][0][2] );


  // drawing the sencod face
  glVertex3f( (float) face[1][0][0], (float) face[1][0][1],  (float) face[1][0][2] );
  glVertex3f( (float) face[1][1][0], (float) face[1][1][1],  (float) face[1][1][2] );

  glVertex3f( (float) face[1][1][0], (float) face[1][1][1],  (float) face[1][1][2] );
  glVertex3f( (float) face[1][2][0], (float) face[1][2][1],  (float) face[1][2][2] );

  glVertex3f( (float) face[1][2][0], (float) face[1][2][1],  (float) face[1][2][2] );
  glVertex3f( (float) face[1][3][0], (float) face[1][3][1],  (float) face[1][3][2] );

  glVertex3f( (float) face[1][3][0], (float) face[1][3][1],  (float) face[1][3][2] );
  glVertex3f( (float) face[1][0][0], (float) face[1][0][1],  (float) face[1][0][2] );
  

  // drawing the connections
  glVertex3f( (float) face[0][0][0], (float) face[0][0][1],  (float) face[0][0][2] );
  glVertex3f( (float) face[1][0][0], (float) face[1][0][1],  (float) face[1][0][2] );

  glVertex3f( (float) face[0][1][0], (float) face[0][1][1],  (float) face[0][1][2] );
  glVertex3f( (float) face[1][1][0], (float) face[1][1][1],  (float) face[1][1][2] );

  glVertex3f( (float) face[0][2][0], (float) face[0][2][1],  (float) face[0][2][2] );
  glVertex3f( (float) face[1][2][0], (float) face[1][2][1],  (float) face[1][2][2] );

  glVertex3f( (float) face[0][3][0], (float) face[0][3][1],  (float) face[0][3][2] );
  glVertex3f( (float) face[1][3][0], (float) face[1][3][1],  (float) face[1][3][2] );

  glEnd();


  glPopMatrix();
  }


}


void ced_register_elements(void){
  GEOC_ID  =ced_register_element(sizeof(CED_GeoCylinder),(ced_draw_cb)ced_draw_geocylinder);
  LINE_ID  =ced_register_element(sizeof(CED_Line),(ced_draw_cb)ced_draw_line);
  HIT_ID   =ced_register_element(sizeof(CED_Hit),(ced_draw_cb)ced_draw_hit);
  GEOB_ID  =ced_register_element(sizeof(CED_GeoBox),(ced_draw_cb)ced_draw_geobox);
  GEOBR_ID  =ced_register_element(sizeof(CED_GeoBoxR),(ced_draw_cb)ced_draw_geobox_r);
}



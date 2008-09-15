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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <ced_cli.h>
#include <ced.h>

/** This defines what is visible */
unsigned ced_visible_layers=0x00000FFF;

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
   	
   	//glEnable(GL_BLEND);
  	ced_color(h->color);
  	glLineWidth(h->width);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  	glBegin(GL_LINES);
  	glVertex3fv(&h->p0.x);
  	glVertex3fv(&h->p1.x);
  	//glDisable(GL_BLEND);
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
 * GeoCylinder
 */
static unsigned GEOCR_ID=0;

static void ced_draw_geocylinder_r(CED_GeoCylinderR *c){
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

	//glEnable(GL_BLEND);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);  
      
  gluQuadricNormals(q1, GL_SMOOTH);
  gluQuadricTexture(q1, GL_TRUE);
  gluCylinder(q1, c->d, c->d, c->z, c->sides, 1);
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

    glDisable(GL_BLEND);
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
    glDisable(GL_BLEND);
    glPopMatrix();
  	glEndList();	
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

static unsigned LEGEND_ID=0;


/**
 * Draws the energy spectrum legend 
 * @author: SD
 * @date: 1.09.09
 * */
static void ced_draw_legend(CED_Legend *legend){
	
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
	char string[3];
	int x_offset = 34;
	int y_offset = 5;
	float num;
	
	/** Legend header */
	char* header = "GeV";
	char* footer = "LOG";
	int x_offset_legend = 60;
	int y_offset_legend = 20;
	
	int font=(int)GLUT_BITMAP_TIMES_ROMAN_10; //default font                           //draw into back right buffer
  	glLoadIdentity(); //load an 'identity projection' matrix
  	int tick_size = 10;
	
	/**
	 *  Legend header: GeV */
	glColor3f(1.0,1.0,1.0);
	renderBitmapString(x_min-x_offset_legend,y_min+stripeThickness*color_steps-y_offset_legend, (void*)font, header);
	glEnd();
	//glPopMatrix();
	
	/**
	 *  Legend footer: LOG or LIN */
	switch(scale){
		case 'a': default:
			renderBitmapString(x_min-x_offset_legend,y_min-y_offset_legend, (void*)font, footer);
			glEnd();
		break;
		/** LIN */
		case 'b':
			footer = "LIN";	
			renderBitmapString(x_min-x_offset_legend,y_min-y_offset_legend, (void*)font, footer);
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
				sprintf(string, "%.1f", ene_min);
				renderBitmapString(x_min+x_offset,y_min+y_offset, (void*)font, string);
			}
			else if (i==(color_steps-1)){
				//printf("top\n");
				sprintf(string, "%.1f", ene_max);
				renderBitmapString(x_min+x_offset,y_min+stripeThickness*i+y_offset, (void*)font, string);
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
			
			sprintf(string, "%.1f", num);
			renderBitmapString(x_min+x_offset,y_min+stripeThickness*pos+y_offset, (void*)font, string);

			++tickNumber;
		}
	}
	glEnd();
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
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(cone->RGBAcolor[0], cone->RGBAcolor[1], cone->RGBAcolor[2], cone->RGBAcolor[3]);
	glutSolidCone(base, height, slices, stacks);
	glDisable(GL_BLEND);
	
	glEnd();
	glPopMatrix();
  	
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


void ced_register_elements(void){
  GEOC_ID  =ced_register_element(sizeof(CED_GeoCylinder),(ced_draw_cb)ced_draw_geocylinder);
  GEOCR_ID  =ced_register_element(sizeof(CED_GeoCylinderR),(ced_draw_cb)ced_draw_geocylinder_r);
  LINE_ID  =ced_register_element(sizeof(CED_Line),(ced_draw_cb)ced_draw_line);
  HIT_ID   =ced_register_element(sizeof(CED_Hit),(ced_draw_cb)ced_draw_hit);
  GEOB_ID  =ced_register_element(sizeof(CED_GeoBox),(ced_draw_cb)ced_draw_geobox);
  GEOBR_ID  =ced_register_element(sizeof(CED_GeoBoxR),(ced_draw_cb)ced_draw_geobox_r);
  CONER_ID  =ced_register_element(sizeof(CED_ConeR),(ced_draw_cb)ced_draw_cone_r);
  ELLIPSOID_ID = ced_register_element(sizeof(CED_EllipsoidR),(ced_draw_cb)ced_draw_ellipsoid_r);
  CLUELLIPSE_ID = ced_register_element(sizeof(CED_CluEllipseR),(ced_draw_cb)ced_draw_cluellipse_r);
  /** due to an issue w/ drawing the legend (in 2D) this has to come last ! */
  LEGEND_ID  =ced_register_element(sizeof(CED_Legend),(ced_draw_cb)ced_draw_legend);
}




/* "C" event display.
 * Enduser accessable API.
 *
 * Alexey Zhelezov, DESY/ITEP, 2005 */

#ifndef __CED_CLI_H
#define __CED_CLI_H

#ifdef __cplusplus
extern "C" {
#endif
		

/*
 * This is the first function to call (before any other).
 *
 *  host - host with CED (must be "localhost")
 *  port - server port number (let say 7285 :)
 *
 * NOTE: ced_register_elements() must be called
 *       separately.
 */
void ced_client_init(const char *host,unsigned short port);

/*
 * Cancel current event output. So, all elements
 * queued will be discarded.
 *
 * Good to call at the begining of every event processing.
 */
void ced_new_event(void);


/*
 * This function really attempt to display event in CED.
 * When CED is not available, this function discard
 * current event information.
 *
 * NOTE: between ced_new_event() and ced_draw_event()
 *       must be some element creation calls.
 */
void ced_draw_event(void);

/*
 * This function really attempt to display event in CED.
 * Unlike ced_draw_event() does not reset the event.
 *
 * NOTE: between ced_new_event() and ced_draw_event()
 *       must be some element creation calls.
 */
void ced_send_event(void);


/*********************************************
 *
 * The following is elements API.
 *
 *********************************************/

void ced_register_elements(void);

typedef enum {
  CED_TYPE_SHIFT=0x0,
  CED_LAYER_SHIFT=0x8
} CED_TYPE_BITS;

typedef struct {
  float x;
  float y;
  float z;
} CED_Point;

/*
 * Hit element
 */

typedef enum {
    CED_HIT_POINT=0,
    CED_HIT_CROSS,
    CED_HIT_STAR
} CED_HIT_TYPE;

typedef struct {
  CED_Point p;
  unsigned type;  // not yet defined...
  unsigned color; // in ARGB form (so, 0xff0000 is RED)
  unsigned size;  // size of point/size of cross
  unsigned lcioID; // unique id of LICO object
} CED_Hit;

void ced_hit(float x,float y,float z,unsigned type,unsigned size,unsigned color);
void ced_hit_ID(float x,float y,float z,unsigned type,unsigned size,unsigned color, unsigned lcioID);

/*
 * Line element
 */

typedef struct {
  CED_Point p0;
  CED_Point p1;
  unsigned type;  // not yet defined...
  unsigned width; // not yet defined...
  unsigned color; // in ARGB form (so, 0xff0000 is RED)
} CED_Line;

void ced_line(float x0,float y0,float z0,
	      float x1,float y1,float z1,
	      unsigned type,unsigned width,unsigned color);

/*
 * GeoCylinder
 */
typedef struct {
  float d;       // radius
  unsigned  sides;   // poligon order
  float rotate;  // angle degree
  float z;       // 1/2 length
  float shift;   // in z
  unsigned color;
} CED_GeoCylinder;

void ced_geocylinder(float d,unsigned sides,float rotate,float z,float shift,
		     unsigned color);

void ced_geocylinders(unsigned n,CED_GeoCylinder *all);
		   
/*
 * GeoCylinder rotatable
 * @author: S.Daraszewicz (UoE)
 * @date: 01.09.09
 */
typedef struct {
  float d;       	// radius
  unsigned sides; 	// poligon order
  float center[3];  // cylinder centre z,y,z
  float rotate[3];  // rotation angles wrt x,y,z axis
  float z;       	// length
  unsigned color;	// colour
  unsigned layer; 	// layer the Cylinder to be displayed onto
} CED_GeoCylinderR;
		   
		     
void ced_geocylinder_r(float d, double z, double * center, double * rotate, unsigned sides, 
		     unsigned int color, int layer);
		     

  /** GeoBox structure
   */
  typedef struct {
    /** The three box sizes in mm */
    double sizes[3];
    /** position of the center of the box*/
    double center[3];
    /** box color */
    unsigned int color;
  } CED_GeoBox;

  /** Send/Draw a box at position center (x,y,z in mm) with lengths along the 
   * axes specified in sizes.
   * 
   * @author A.Bulgheroni, INFN
   */
  void ced_geobox(double * sizes, double * center, unsigned int color );

    /** Send/Draw several boxes.
   * 
   * @author A.Bulgheroni, INFN
   */
  void ced_geoboxes( unsigned int nBox, CED_GeoBox * allBoxes);

  typedef struct {
    /** The three box sizes in mm */
    double sizes[3];
    /** position of the center of the box*/
    double center[3];
    /** box color */
    unsigned int color;
    /** rotation angle in degrees */
    double rotate[3];
    /** layer for toggling display */
    unsigned int layer;
  } CED_GeoBoxR;

void ced_geobox_r(double * sizes, double * center, double * rotate, unsigned int color, unsigned int layer);

/*
 * Energy spectrum colour map legend.
 * @author: S.Daraszewicz (UoE)
 * @date: 01.09.09
 */
  typedef struct {  
  	/** min energy on the legend */	
  	float ene_max;
  	/** max energy on the legend */
  	float ene_min;
  	/** number of ticks on the legend */
  	unsigned int ticks;
  	/** spectrum colour steps */
  	unsigned int color_steps; 
  	/** spectrum colour matrix */
  	unsigned int rgb_matrix[512][3]; //FIX ME: 512 size not changed with color_steps
  	/** LOG or LIN */
  	char scale;
  } CED_Legend;


void ced_legend(float ene_min, float ene_max, unsigned int color_steps, unsigned int ** rgb_matrix, unsigned int ticks, char scale);

  typedef struct {  
  	/** position of the centre of the base */	
  	double center[3];
  	/** rotation matrix */
  	double rotate[3];
    /** layer for toggling display */
    unsigned int layer;
    /** base radius */
    float base;
    /** height */
    float height;
    /** RGBA color */
    float RGBAcolor[4];
  } CED_ConeR;


void ced_cone_r(float base, float height, double *center, double *rotate, unsigned int layer, float *RGBAcolor);

  typedef struct {  
  	/** position of the centre of the base */	
  	double center[3];
  	/** rotation matrix */
  	double rotate[3];
    /** layer for toggling display */
    unsigned int layer;
    /** xyz size */
	double size[3];
    /** RGBA color */
   	int color;
  } CED_EllipsoidR;


void ced_ellipsoid_r(double *size, double *center, double *rotate, unsigned int layer, int color);

  typedef struct {  
  	/** position of the centre of the base */	
  	double center[3];
  	/** rotation matrix */
  	double rotate[3];
    /** layer for toggling display */
    unsigned int layer;
    /** base radius */
	float radius;
	/** half height */
	float height;
    /** RGBA color */
    int color;
  } CED_CluEllipseR;


void ced_cluellipse_r(float radius, float height, float *center, double *rotate, unsigned int layer, int color);

#ifdef __cplusplus
 }
#endif
	

#endif /* __CED_CLI_H */

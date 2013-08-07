#include <ced_cli.h>
#include <time.h>
#include <stdlib.h>
#include <iostream>

/** Simple test program to draw a million hits
 *  @author Jan Engels - Desy - IT
 */

int main(){
  
  /*
   * Initialisation */
  ced_client_init("localhost",7286);
  ced_register_elements();
  
  time_t t;
  srand((unsigned) time(&t));
  unsigned i;


  unsigned nPts = 4000 ;

  int boxL = 1000 ;

  unsigned marker = 3 ;
  unsigned layer =  7 ;
  unsigned type = ( marker ) | ( layer << CED_LAYER_SHIFT ) ;
  unsigned size = 80 ; 


  // ced_line_ID( -boxL , 0. , 0. ,  +boxL , 0. , 0. , layer, 1 , 0x009900 , 0 ) ; 
  // ced_line_ID( 0, -boxL , 0. ,  0, +boxL     , 0. , layer, 1 , 0x009900 , 0 ) ; 
  // ced_line_ID( 0, 0, -boxL ,  0, 0, +boxL    , layer, 1 , 0x009900 , 0 ) ; 


  //red
  for( i=0; i<nPts ; i++){
    ced_hit( rand()%boxL, rand()%boxL, rand()%boxL, type, size, 0x990000 );
  }
    
  //green
  for( i=0; i<nPts ; i++){
    ced_hit( (rand()%boxL), -(rand()%boxL), -(rand()%boxL), type, size, 0x009900 );
  }

  //cyan
  for( i=0; i<nPts ; i++){
    ced_hit( -(rand()%boxL), rand()%boxL, -(rand()%boxL), type, size, 0x009999 );
  }

  //blue
  for( i=0; i<nPts ; i++){
    ced_hit( -rand()%boxL, -(rand()%boxL), rand()%boxL, type, size, 0x0000ff );
  }

  ced_send_event();    // Does not clean up screen


  return 0;
}

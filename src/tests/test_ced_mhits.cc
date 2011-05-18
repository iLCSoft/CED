#include <ced_cli.h>
#include <time.h>
#include <stdlib.h>

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

    //red
    for( i=0; i<250000 ; i++){
        ced_hit( rand()%5000, rand()%5000, rand()%5000, 0, 1, 0x990000 );
    }

  	//ced_send_event();    // Does not clean up screen
    
    //green
    for( i=0; i<250000 ; i++){
        ced_hit( 0-(rand()%5000), 0-(rand()%5000), 0-(rand()%5000), 0, 1, 0x009900 );
    }

  	//ced_send_event();    // Does not clean up screen
    
    //cyan
    for( i=0; i<250000 ; i++){
        ced_hit( 0-(rand()%5000), rand()%5000, 0-(rand()%5000), 0, 1, 0x009999 );
    }

  	//ced_send_event();    // Does not clean up screen
    
    //blue
    for( i=0; i<250000 ; i++){
        ced_hit( rand()%5000, 0-(rand()%5000), rand()%5000, 0, 1, 0x0000ff );
    }


/*****************************************************
 * This code used to crash CED before bug fix in r1559
 *****************************************************

    //red
    for( i=0; i<300000 ; i++){
        ced_hit( rand()%5000, rand()%5000, rand()%5000, 0, 1, 0x990000 );
    }

  	//ced_send_event();    // Does not clean up screen
    
    //green
    for( i=0; i<50000 ; i++){
        ced_hit( 0-(rand()%5000), 0-(rand()%5000), 0-(rand()%5000), 0, 1, 0x009900 );
    }

  	//ced_send_event();    // Does not clean up screen
    
    //cyan
    for( i=0; i<7000 ; i++){
        ced_hit( 0-(rand()%5000), rand()%5000, 0-(rand()%5000), 0, 1, 0x009999 );
    }

  	//ced_send_event();    // Does not clean up screen
    
    //white
    for( i=0; i<140 ; i++){
        ced_hit( rand()%5000, 0-(rand()%5000), rand()%5000, 0, 1, 0xffffff );
    }

  	ced_send_event();    // Does not clean up screen
  	//ced_draw_event();    // Make clean up screen before drawing

    ced_hit( 3000, -3000, 3000, 0, 5, 0xffffff );
  	ced_send_event();    // Does not clean up screen
    ced_hit( 2000, -2000, 2000, 0, 5, 0xffffff );
  	ced_send_event();    // Does not clean up screen

    // ced can only draw 357142 hits at a time
    // this hit will crash
    ced_hit( 1000, -1000, 1000, 0, 5, 0xffffff );

*/

  	ced_send_event();    // Does not clean up screen
  	//ced_draw_event();    // Make clean up screen before drawing

 return 0;
}

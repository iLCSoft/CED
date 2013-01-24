/****************************************************************
NAME: 
    ced_menu.h
AUTHOR: 
    Hauke Hoelbe
DATE:
    03.05.2012
USED BY: 
    glced.cc
DESCRIPTION: 
    provides definitions and classes for the CED main and popup menu.
****************************************************************/
#ifndef __CED_MENU
#define __CED_MENU
using namespace std;
extern CEDsettings setting;
extern GLfloat window_width;

void drawHelpString (const string & str, float x,float y){ //format help strings strings: "[<key>] <description>"
    unsigned int i;
//    float x1=x;
//    float y1=y;
//    if( x1 < 0.0){
//        x1=0.;
//    }  
//    if( y1 < 0.0){ 
//        y1=0.;
//    }  
//
//    glRasterPos2f(x1,y1);

    glRasterPos2f(x,y);

    int monospace = 0;
    for (i = 0; str[i]; i++){
        if(str[i] == '['){
            monospace = 1;
            if(setting.font == 0){
                glutBitmapCharacter (GLUT_BITMAP_HELVETICA_10, '[');
            }else if(setting.font == 1){
                glutBitmapCharacter (GLUT_BITMAP_HELVETICA_12, '[');
            }else if(setting.font == 2){
                glutBitmapCharacter (GLUT_BITMAP_HELVETICA_18, '[');
            }
            i++;
        }
        else if(str[i] == ']'){
             monospace = 0;
        }
        if(monospace){
            if(setting.font == 0){
                glutBitmapCharacter(GLUT_BITMAP_8_BY_13, str[i]);
            }else if(setting.font == 1){
                glutBitmapCharacter(GLUT_BITMAP_8_BY_13, str[i]);
            }else if(setting.font == 2){
                glutBitmapCharacter(GLUT_BITMAP_9_BY_15, str[i]);
            }
        }else{
            //glutBitmapCharacter (GLUT_BITMAP_HELVETICA_10, str[i]);
            //glutBitmapCharacter ( GLUT_BITMAP_HELVETICA_12 , str[i]);
            //glutBitmapCharacter ( GLUT_BITMAP_HELVETICA_18 , str[i]);
            if(setting.font == 0){
                glutBitmapCharacter (GLUT_BITMAP_HELVETICA_10, str[i]);
            }else if(setting.font == 1){
                glutBitmapCharacter (GLUT_BITMAP_HELVETICA_12, str[i]);
            }else if(setting.font == 2){
                glutBitmapCharacter (GLUT_BITMAP_HELVETICA_18, str[i]);
            }
        }
    }
}

void drawStringBig (char *s){
    unsigned int i;
    for (i = 0; i[s]; i++){
        glutBitmapCharacter (GLUT_BITMAP_HELVETICA_18, s[i]);
    }
}



class CED_SubSubMenu{
    public:
        void setTitle(string new_title){
            title=new_title;
        }
        void draw(){
                int height=10;
                int width=2;
                if(setting.font==0){
                    height=10;
                    width=6;
                }
                if(setting.font==1){
                    height=12;
                    width=8;
                }
                if(setting.font==2){
                    height=20;
                    width=11;
                }


                if(isExtend || isMouseOver){
                    isAktive=true;
                    glColor3f(0.662745,0.662745,0.662745);
                    glBegin(GL_QUADS);
                    glVertex3f(x_start,y_start,0);
                    glVertex3f(x_start,y_end,0);
                    glVertex3f(x_end,y_end,0);
                    glVertex3f(x_end,y_start,0);
                    glEnd();

                    glColor3f(0.662745,0.662745,0.662745);
                    glBegin(GL_LINES);

                    glVertex3f(x_start,y_start,0);
                    glVertex3f(x_start,y_end,0);

                    glVertex3f(x_end,y_end,0);
                    glVertex3f(x_end,y_start,0);

                    glVertex3f(x_start,y_end,0);
                    glVertex3f(x_end,y_end,0);

                    glEnd();

                    glColor3f(1,1,1);
                    unsigned i;


                    unsigned maxlength=0;
                    for(i=0;(unsigned) i<subsubMenus.size();i++){
                        if(subsubMenus.at(i)->title.length() > maxlength){
                            maxlength=subsubMenus.at(i)->title.length();
                        }
                    }
                    for(i=0;(unsigned) i<subsubMenus.size();i++){
                            if(window_width > x_end+maxlength*width || window_width-x_end  > x_start){
                                subsubMenus.at(i)->x_start=x_end;
                                subsubMenus.at(i)->x_end  =x_end+maxlength*width;
                            }else{
                                subsubMenus.at(i)->x_start=x_start-maxlength*width;
                                subsubMenus.at(i)->x_end  =x_start;
                            }

                            if(window_height < (y_start+height*subsubMenus.size()) ){
                                subsubMenus.at(i)->y_start=y_start-height*subsubMenus.size()+height+height*i;
                                subsubMenus.at(i)->y_end  =y_end-height*subsubMenus.size() +height+ height*i;
                            }else{
                                subsubMenus.at(i)->y_start=y_start+height*i;
                                subsubMenus.at(i)->y_end  =y_end + height*i;
                            }
                            subsubMenus.at(i)->draw();
                    }




                }else{
                    isAktive=false;
                    glColor4f(0.827451,0.827451,0.827451,1);

                    glBegin(GL_QUADS);
                    glVertex3f(x_start,y_start,0);
                    glVertex3f(x_start,y_end,0);
                    glVertex3f(x_end,y_end,0);
                    glVertex3f(x_end,y_start,0);
                    glEnd();

                    glColor3f(0.662745,0.662745,0.662745);
                    glBegin(GL_LINES);

                    glVertex3f(x_start,y_start,0);
                    glVertex3f(x_start,y_end,0);
                    glVertex3f(x_end,y_end,0);
                    glVertex3f(x_end,y_start,0);


                    //glVertex3f(x_start,y_end,0);
                    //glVertex3f(x_end,y_end,0);


                    //glVertex3f(x_end,y_start,0);
                    //glVertex3f(x_start,y_start,0);
                    glEnd();
                    glColor3f(0,0,0);
                    if(subsubMenus.size() > 0){
                        //cout << "draw triangle"  << endl;
                        glBegin(GL_TRIANGLES);
                        glVertex3f(x_end-8,y_start+1,0);
                        glVertex3f(x_end-8,y_end-1,0);
                        glVertex3f(x_end-1,y_start+(y_end-y_start)/2,0);
                        glEnd();
                    }


                }
                if(x_start < 0){
                    x_start=0;
                }
                //if(x_end > window_width){
                //    x_end=window_width;
                //}


                if(title == "---"){

                    glColor3f(0.662745,0.662745,0.662745);
                    glBegin(GL_LINES);
                    glVertex3f(x_start,y_start+0.5*(y_end - y_start),0);
                    glVertex3f(x_end,y_start+0.5*(y_end - y_start),0);
                    glEnd();
                    glColor3f(0,0,0);
                }else{
                    drawHelpString(title.substr(0,int(fabs((x_end-x_start)/width))), x_start+3, y_start+height-height/5);
                }
        }

        void clickAt(int x,int y){
            //cout << "x = " << x << " y == " << y << endl;
            //cout << "x_start = " << x_start << " y == " << y << endl;

            if(x_start < x && x_end > x && y_start < y && y_end > y){
                //cout << "submenu clicked!" << endl;

               if(isAktive){
                    selectFromMenu(optionNr);
                }
                if(isExtend){
                    isExtend=false;
                }else{
                    if(subsubMenus.size() > 0){
                        isExtend=true;
                    }else{
                        isExtend=false;
                        isMouseOver=false;
                    }

                }
            }else{
                isExtend=false;
            }
            unsigned i;
            for(i=0;(unsigned) i<subsubMenus.size();i++){
                if(subsubMenus.at(i)->isExtend || subsubMenus.at(i)->isMouseOver){
                    //cout << "send clickat to: " << subsubMenus.at(i)->title << endl;
                    subsubMenus.at(i)->clickAt(x,y);
                }
            }

        }

        int mouseMove(int x,int y){
            if(x_start < x && x_end > x && y_start < y && y_end > y){
                isMouseOver=true;
                glutPostRedisplay();
            } else if(isMouseOver==true){
                isMouseOver=false;
                glutPostRedisplay();
            }
            unsigned i;
            for(i=0;(unsigned) i<subsubMenus.size();i++){
                if( subsubMenus.at(i)->mouseMove(x,y)){
                   // if(selected_submenu != NULL){
                   //     selected_submenu->isExtend=false;
                   // }
                   // selected_submenu=subsubMenus.at(i);
                }
            }

            return(isMouseOver);
        }


        void addItem(CED_SubSubMenu *subsub){
           int height=10;
//           int width=6;
           if(setting.font==0){
               height=10;
//               width=6;
           }
           if(setting.font==1){
               height=12;
//               width=8;
           }
           if(setting.font==2){
               height=20;
//               width=11;
           }

            subsub->x_start=x_start;
            subsub->x_end  =x_start+50; //TODO
            subsub->y_start=y_start+height;
            subsub->y_end  =y_end + height;
            subsubMenus.push_back(subsub);
        }
        CED_SubSubMenu(string t, int nr=0){
            title=t;
            optionNr=nr;
            isExtend=false;
            isMouseOver=false;
            dead=false;
        }
        ~CED_SubSubMenu(){
            if(dead == true){
                return;
            }
            cout << "       delete subsubmenus (" << subsubMenus.size() << " submenus)" <<  endl;
            for(unsigned i=1;i<subsubMenus.size();i++){
                cout << "           delete: " << subsubMenus.at(i)->title <<   endl;
                delete subsubMenus.at(i);
                subsubMenus.at(i)=NULL;
                //subsubMenu.remove(i);
            }
            dead=true;
        }


        bool isAktive;
        string title;
        int optionNr;
        bool isExtend;
        bool isMouseOver;
        int x_start;
        int x_end;
        int y_start;
        int y_end;

        bool dead;

    private:
        vector<CED_SubSubMenu *> subsubMenus;
};


class CED_SubMenu{
    public:
        void setTitle(string new_title){
            title=new_title;
        }
        void draw(){
                int height=10;
                int width=6;
                if(setting.font==0){
                    height=10;
                    width=6;
                }
                if(setting.font==1){
                    height=12;
                    width=8;
                }
                if(setting.font==2){
                    height=20;
                    width=11;
                }


                if(isExtend || isMouseOver){
                    glColor4f(0.662745,0.662745,0.662745,1);
                    glBegin(GL_QUADS);
                    glVertex3f(x_start,y_start,0);
                    glVertex3f(x_start,y_end,0);
                    glVertex3f(x_end,y_end,0);
                    glVertex3f(x_end,y_start,0);
                    glEnd();

                    glColor3f(1,1,1);
                }else{
                    glColor3f(0,0,0);
                }
                //drawHelpString(title, x_start+3, y_start+8);

                if(title == "---"){

                    glColor3f(0.662745,0.662745,0.662745);
                    glBegin(GL_LINES);
                    glVertex3f(x_start,y_start+0.5*(y_end - y_start),0);
                    glVertex3f(x_end,y_start+0.5*(y_end - y_start),0);
                    glEnd();
                    glColor3f(0,0,0);
                }else{
                    drawHelpString(title, x_start+3, y_start+height-height/5);
                }

                //drawHelpString(title, x_start+3, y_start+height-height/5);
                if(isExtend){
                    unsigned i;
                    //cout << "draw " << subsubMenus.size() << " subsubmenu items" << endl;
                   if( selected_submenu != NULL){
                        selected_submenu->isExtend=true;
                        //selected_submenu->mouseOver();
                    }

                    unsigned maxlength=0;
                    for(i=0;(unsigned) i<subsubMenus.size();i++){
                        if(subsubMenus.at(i)->title.length() > maxlength){
                            maxlength=subsubMenus.at(i)->title.length();
                        }
                    }


                    for(i=0;(unsigned) i<subsubMenus.size();i++){
                        subsubMenus.at(i)->x_start=x_start;
                        subsubMenus.at(i)->x_end  =x_start+maxlength*width;
                        subsubMenus.at(i)->y_start=y_start+height+1+height*i;
                        subsubMenus.at(i)->y_end  =y_end + height+1+height*i;
                        if(subsubMenus.at(i)->x_end > window_width){
                            int tmp=subsubMenus.at(i)->x_end;
                            int tmp2=subsubMenus.at(i)->x_start;
                            subsubMenus.at(i)->x_end = x_end;
                            subsubMenus.at(i)->x_start=x_end-(tmp-tmp2);
                        }
                        subsubMenus.at(i)->draw();
                    }
                }
        }

        void clickAt(int x,int y){
            //cout << "x = " << x << " y == " << y << endl;
            //cout << "x_start = " << x_start << " y == " << y << endl;
            if(isExtend){
                unsigned i;
                for(i=0;(unsigned) i<subsubMenus.size();i++){
                    subsubMenus.at(i)->clickAt(x,y);
                }
            }

            if(x_start < x && x_end > x && y_start < y && y_end > y){
                //cout << "submenu clicked!" << endl;
                if(isExtend){
                    isExtend=false;
                }else{
                    isExtend=true;
                    selectFromMenu(optionNr);
                }
            }else{
                isExtend=false;
            }


            selected_submenu=NULL;
        }

        void mouseMove(int x,int y){
            if(isExtend){
                unsigned i;
                for(i=0;(unsigned) i<subsubMenus.size();i++){
                    if( subsubMenus.at(i)->mouseMove(x,y)){
                        if(selected_submenu != NULL){
                            selected_submenu->isExtend=false;
                        }
                        selected_submenu=subsubMenus.at(i);
                    }
                }
            }
            if(x_start < x && x_end > x && y_start < y && y_end > y){
                isMouseOver=true;
                glutPostRedisplay();
            }else if(isMouseOver==true){
                isMouseOver=false;
                glutPostRedisplay();
            }

        }


        void addItem(CED_SubSubMenu *subsub){
            subsubMenus.push_back(subsub);

        }
        CED_SubMenu(string t, int nr=0){
            title=t;
            optionNr=nr;
            isExtend=false;
            isMouseOver=false;
            selected_submenu=NULL;
        }
        ~CED_SubMenu(){
            for(int i=0;(unsigned) i<subsubMenus.size();i++){
                delete subsubMenus.at(i);
                subsubMenus.at(i)=NULL;
            }
        }



        string title;
        int optionNr;
        bool isExtend;
        bool isMouseOver;
        int x_start;
        int x_end;
        int y_start;
        int y_end;


    private:
        CED_SubSubMenu *selected_submenu;
        vector<CED_SubSubMenu *> subsubMenus;
};

class CED_Menu{
    public:
        void draw(void){
            //cout << "menu draw" << endl;
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

            glOrtho(0,w,h, 0,0,15*WORLD_SIZE);

            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();

            //glColor3f(0,0,0);

            glColor4f(0.827451,0.827451,0.827451,1);

            int height=10;
            if(setting.font==0){
                height=10;
            }
            if(setting.font==1){
                height=12;
            }
            if(setting.font==2){
                height=20;
            }



            glBegin(GL_QUADS);
            glVertex3f(0, 0,0);
            glVertex3f(0,height,0);
            glVertex3f(w,height,0);
            glVertex3f(w,0,0);
            glEnd();


            glColor3f(0.501961,0.501961,0.501961);
            glLineWidth(1.);
            glBegin(GL_LINES);

            //glVertex3f(1,1,0);
            //glVertex3f(1,10,0);

            glVertex3f(1,height+1,0);
            glVertex3f(w-1,height+1,0);

            //glVertex3f(w-1,8,0);
            //glVertex3f(w-1,1,0);


            //glVertex3f(w-1,1,0);
            //glVertex3f(1,1,0);
            glEnd();

            glColor3f(0,0,0);

            unsigned i;
            //unsigned x_offset=1;
            for(i=0;(unsigned) i<subMenus.size();i++){
               //drawHelpString(shortcuts[i],  R_COLUMN)*column+boarder_quad+5, (i%ITEMS_PER_COLUMN)*line+boarder_quad+10);

               subMenus.at(i)->draw();

            }

            glEnable(GL_DEPTH_TEST);
            glMatrixMode(GL_PROJECTION);
            glPopMatrix();
            glMatrixMode(GL_MODELVIEW);
            glPopMatrix();


        }
        void clickAt(int x, int y){
            //cout << "isClicked" << endl;
            unsigned i;
            for(i=0;(unsigned) i<subMenus.size();i++){
                subMenus.at(i)->clickAt(x,y);
            }

        }

        void mouseMove(int x, int y){
            unsigned i;
            for(i=0;(unsigned) i<subMenus.size();i++){
                subMenus.at(i)->mouseMove(x,y);
            }

        }

        void addSubMenu(CED_SubMenu *sub){
            double length=10;
            int height=10;
            if(setting.font==0){
                length=4.8;
                height=10;
            }
            if(setting.font==1){
                length=5.2;
                height=12;
            }
            if(setting.font==2){
                length=10.0;
                height=20;
            }
            //cout << "length: " << length << endl;

            sub->x_start=x_offset;
            sub->y_start=1;
            sub->y_end=height+1;
            x_offset+=(unsigned)(sub->title.length()*length+3*length);
            sub->x_end=x_offset;
            subMenus.push_back(sub);
            x_offset+=5;
        }
        CED_Menu(){
            x_offset=1;
        }
        ~CED_Menu(){
            cout << "delete ced menu" <<  endl;
            for(int i=0;(unsigned) i<subMenus.size();){
                delete subMenus.at(i);
            }
        }

    private:
        vector<CED_SubMenu *> subMenus;
        unsigned x_offset;
};

class CED_PopUpMenu{
    public:
        void setTitle(string new_title){
            title=new_title;
        }
        void draw(){
            int height=10;
            int width=6;
            if(setting.font==0){
                height=10;
                width=6;
            }
            if(setting.font==1){
                height=12;
                width=8;
            }
            if(setting.font==2){
                height=20;
                width=11;
            }

                                             
            unsigned  maxlength=title.length();
            unsigned i;
            for(i=0;(unsigned) i<subsubMenus.size();i++){
                if(subsubMenus.at(i)->title.length() > maxlength){
                    maxlength=subsubMenus.at(i)->title.length();
                }
            }
            x_end=x_start+maxlength*width;


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

            glOrtho(0,w,h, 0,0,15*WORLD_SIZE);

            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();

               // if(isExtend || isMouseOver){
               //     glColor4f(0.662745,0.662745,0.662745,1);
               //     glBegin(GL_QUADS);
               //     glVertex3f(x_start,y_start,0);
               //     glVertex3f(x_start,y_end,0);
               //     glVertex3f(x_end,y_end,0);
               //     glVertex3f(x_end,y_start,0);
               //     glEnd();

               //     glColor3f(1,1,1);
               // }else{
                    glColor3f(0,0,0);
               // }

                  if(isExtend){
                    //drawHelpString(title, x_start+3, y_start+height-height/5.);

                    if( selected_submenu != NULL){
                         selected_submenu->isExtend=true;
                         //selected_submenu->mouseOver();
                     }

                    unsigned i;
                    for(i=0;(unsigned) i<subsubMenus.size();i++){
                        subsubMenus.at(i)->x_start=x_start;
                        subsubMenus.at(i)->x_end  =x_end;
                        subsubMenus.at(i)->y_start=y_start/*+height */+1+height*i;
                        subsubMenus.at(i)->y_end  =y_end  /*+ height*/+1+height*i;
                        subsubMenus.at(i)->draw();
                    }
                }
            glEnable(GL_DEPTH_TEST);
            glMatrixMode(GL_PROJECTION);
            glPopMatrix();
            glMatrixMode(GL_MODELVIEW);
            glPopMatrix();

        }

        void clickAt(int x,int y){
            //cout << "get clickat " << endl;
            //cout << "x = " << x << " y == " << y << endl;
            //cout << "x_start = " << x_start << " y == " << y << endl;
            if(isExtend){
                unsigned i;
                for(i=0;(unsigned) i<subsubMenus.size();i++){
                    subsubMenus.at(i)->clickAt(x,y);
                }
            }

            if(x_start < x && x_end > x && y_start < y && y_end > y){
                //cout << "submenu clicked!" << endl;
                if(isExtend){
                    isExtend=false;
                }else{
                    isExtend=true;
                    selectFromMenu(optionNr);
                }

            }else{
                isExtend=false;
            }

            isExtend=false;
            selected_submenu=NULL;
            x_start=0; x_end=0;

            for(int i=0;(unsigned) i<subsubMenus.size();i++){
                subsubMenus.at(i)->x_end=0;
                subsubMenus.at(i)->x_start=0;
            }
        }
       void mouseMove(int x,int y){
            if(isExtend){
                unsigned i;
                for(i=0;(unsigned) i<subsubMenus.size();i++){
                    if( subsubMenus.at(i)->mouseMove(x,y)){
                        if(selected_submenu != NULL){
                            selected_submenu->isExtend=false;
                        }
                        selected_submenu=subsubMenus.at(i);
                    }

                    //subsubMenus.at(i)->mouseMove(x,y);
                }
                if(x_start < x && x_end > x && y_start < y && y_end > y){
                    isMouseOver=true;
                    glutPostRedisplay();
                }else if(isMouseOver==true){
                    isMouseOver=false;
                    glutPostRedisplay();
                }
            }

        }



        void addItem(CED_SubSubMenu *subsub){
            subsubMenus.push_back(subsub);

        }
        CED_PopUpMenu(string t, int nr=0){
            title=t;
            optionNr=nr;
            isExtend=false;
            isMouseOver=false;
            selected_submenu=NULL;
            //click_x=
            //click_y=
        }

        int size(void){
            return(subsubMenus.size());
        }
        string title;
        int optionNr;
        bool isExtend;
        bool isMouseOver;
        int x_start;
        int x_end;
        int y_start;
        int y_end;
        int x_click;
        int y_click;

    private:
        vector<CED_SubSubMenu *> subsubMenus;
        CED_SubSubMenu *selected_submenu;

};


//********** Definitions for selecting options from the menus *************//

#define PICK_HIT                    24974
#define CENTER_HIT                  24975

#define FONT0                        2010
#define FONT1                        2011
#define FONT2                        2012

#define UNDO                         2312

#define GRAFIC_HIGH                  2000
#define GRAFIC_LOW                   2001
#define GRAFIC_PERSP                 2002
#define GRAFIC_BUFFER                2003
#define GRAFIC_TRANS                 2004
#define GRAFIC_LIGHT                 2005
#define GRAFIC_ALIAS                 2006
#define GRAFIC_FOG                   2007

#define TOGGLE_DETECTOR_PICKING      2008


#define PICKING_MARKER 2009



#define CUT_Z_M6000                  140001
#define CUT_Z_M4000                  140002
#define CUT_Z_M2000                  140003
#define CUT_Z_0000                   140004
#define CUT_Z_2000                   140005
#define CUT_Z_4000                   140006
#define CUT_Z_6000                   140007
#define CUT_Z_7000                   140000


#define LAYER_CUT_Z_M6000             150001
#define LAYER_CUT_Z_M4000             150002
#define LAYER_CUT_Z_M2000             150003
#define LAYER_CUT_Z_0000              150004
#define LAYER_CUT_Z_2000              150005
#define LAYER_CUT_Z_4000              150006
#define LAYER_CUT_Z_6000              150007
#define LAYER_CUT_Z_7000             150000


#define CUT_ANGLE0              12000
#define CUT_ANGLE30             12030
#define CUT_ANGLE90             12090
#define CUT_ANGLE135            12135
#define CUT_ANGLE180            12180
#define CUT_ANGLE200            12200
#define CUT_ANGLE220            12220
#define CUT_ANGLE240            12240
#define CUT_ANGLE260            12260
#define CUT_ANGLE270            12270
#define CUT_ANGLE280            12280
#define CUT_ANGLE290            12290
#define CUT_ANGLE310            12310
#define CUT_ANGLE330            12330
#define CUT_ANGLE340            12340
#define CUT_ANGLE350            12350
#define CUT_ANGLE45             12045
#define CUT_ANGLE100            12100
#define CUT_ANGLE120            12120
#define CUT_ANGLE150            12150
#define CUT_ANGLE170            12170
#define CUT_ANGLE190            12190

#define LAYER_CUT_ANGLE0              13000
#define LAYER_CUT_ANGLE30             13030
#define LAYER_CUT_ANGLE90             13090
#define LAYER_CUT_ANGLE135            13135
#define LAYER_CUT_ANGLE180            13180
#define LAYER_CUT_ANGLE200            13200
#define LAYER_CUT_ANGLE220            13220
#define LAYER_CUT_ANGLE240            13240
#define LAYER_CUT_ANGLE260            13260
#define LAYER_CUT_ANGLE270            13270
#define LAYER_CUT_ANGLE280            13280
#define LAYER_CUT_ANGLE290            13290
#define LAYER_CUT_ANGLE310            13310
#define LAYER_CUT_ANGLE330            13330
#define LAYER_CUT_ANGLE340            13340
#define LAYER_CUT_ANGLE350            13350
#define LAYER_CUT_ANGLE45             13045
#define LAYER_CUT_ANGLE100            13100
#define LAYER_CUT_ANGLE120            13120
#define LAYER_CUT_ANGLE150            13150
#define LAYER_CUT_ANGLE170            13170
#define LAYER_CUT_ANGLE190            13190

#define TRANS0                  3100
#define TRANS40                 3101
#define TRANS60                 3102
#define TRANS70                 3103
#define TRANS80                 3104
#define TRANS90                 3105
#define TRANS95                 3106
#define TRANS100                3107

#define LAYER_TRANS0            3200
#define LAYER_TRANS40           3201
#define LAYER_TRANS60           3202
#define LAYER_TRANS70           3203
#define LAYER_TRANS80           3204
#define LAYER_TRANS90           3205
#define LAYER_TRANS95           3206
#define LAYER_TRANS100          3207


#define FULLSCREEN              6001
#define AXES                    6002
#define FPS                     6003
#define AUTOSHOT                6004

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

//#define BGCOLOR_GAINSBORO       1011
//#define BGCOLOR_LIGHTGREY       1012
//#define BGCOLOR_DARKGRAY        1013
//#define BGCOLOR_GRAY            1014
#define BGCOLOR_OPTION1         1015
#define BGCOLOR_OPTION2         1016
#define BGCOLOR_OPTION3         1017
#define BGCOLOR_OPTION4         1018
#define BGCOLOR_OPTION5         1019
#define BGCOLOR_OPTION6         1020
#define BGCOLOR_OPTION7         1021
#define BGCOLOR_OPTION8         1022
#define BGCOLOR_OPTION9         1023
#define BGCOLOR_OPTION10        1024
#define BGCOLOR_OPTION11        1025
#define BGCOLOR_OPTION12        1026
#define BGCOLOR_OPTION13        1027
#define BGCOLOR_OPTION14        1028
#define BGCOLOR_OPTION15        1029

#define BGCOLOR_USER            1100


#define VIEW_FISHEYE    20
#define VIEW_FRONT      21
#define VIEW_SIDE       22
#define VIEW_ZOOM_IN    23
#define VIEW_ZOOM_OUT   24
#define VIEW_RESET      25
#define VIEW_CENTER     26
#define CED_RESET       27

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
#define LAYER_20        50 
#define LAYER_21        51 
#define LAYER_22        52 
#define LAYER_23        53 
#define LAYER_24        54 
#define LAYER_ALL       60

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



#define HELP            100
#define SAVE1           101
#define SAVE2           102
#define SAVE3           103
#define SAVE4           104
#define SAVE5           105
#define LOAD1           131
#define LOAD2           132
#define LOAD3           133
#define LOAD4           134
#define LOAD5           135


#define SAVE_IMAGE      5555
#define SAVE_IMAGE1     5556
#define SAVE_IMAGE4     5557
#define SAVE_IMAGE10    5558
#define SAVE_IMAGE20    5559
#define SAVE_IMAGE100   5560

#define AUTOSHOT_IMAGE       5561
#define AUTOSHOT_IMAGE1      5562
#define AUTOSHOT_IMAGE4      5563
#define AUTOSHOT_IMAGE10     5564
#define AUTOSHOT_IMAGE20     5565
#define AUTOSHOT_IMAGE100    5566


#define TOGGLE_PHI_PROJECTION   5000
#define TOGGLE_Z_PROJECTION     5001


#endif

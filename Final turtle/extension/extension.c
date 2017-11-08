/* Code which takes an input set of instructions and parses them to decide if they conform to 
the definition of the turtle language - for use with extended formal grammar and new 'turtle' instructions */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <float.h>
#include "sdl2wrapper.h"

#define MAXNUMTOKENS 1000
#define MAXTOKENSIZE 30
#define ALPHABET 26
#define MAXDOLOOPS 15
#define MAXSTACKSIZE 100 /*for use in instruction SET - max number of polish operands+operations */
#define strsame(A,B) (strcmp(A, B)==0)  /* if A and B are the same it returns a 1*/
#define ERROR(PHRASE) {fprintf(stderr, "Fatal Error: %s occured in %s, line %d\n.", PHRASE, __FILE__, __LINE__); exit(2);}
#define ERRORTOKEN(PHRASE, cw) {fprintf(stderr, "Fatal Error: %s occurred for input token number %d of your program.\n", PHRASE, cw); exit(2);}
#define ERRORNEXT(X, Y, cw) {fprintf(stderr, "Fatal Error: Expecting %s following %s for input token number %d of your program.\n", X, Y, cw+1); exit(2);}
#define WARNING(PHRASE, cw) {printf("warning message: in token %s, output may not be as desired because %s", cw, PHRASE)}
#define DEGREESTORADIANS (2*M_PI)/360
#define FIRSTDOGAP 5 /*number of tokens from the end of a 'DO loop' call the first <VAR> is*/
#define SECONDDOGAP 3 /*number of tokens from the end of a 'DO loop' call the second <VARNUM> is*/
#define THIRDDOGAP 1 /*number of tokens from the end of a 'DO loop' call the third <VARNUM> is*/
#define MINBINARYOP 2
#define NUM_ARGS 2
#define STARTX WWIDTH/2
#define STARTY WHEIGHT/2
#define ARBDELAY 200       /*delay in milliseconds between instuction prints - gives the initial speed*/
#define PROGENDPAUSE 2500 /*delay in milliseconds at the end of the display of the program*/
#define RGBMAX 255 /*predefined maximum value for RGB ratios */
#define TRIANGLEHEIGHT 15 /*for drawning a location triangle*/
#define TRIANGLEWIDTH 5
#define MINSPEED 1 /*arbitrary minimum speed users can choose for instruction'SPEED'*/
#define MAXSPEED 10 /*arbitrary maximum speed users can choose for instruction'SPEED'*/
#define MINSPEEDDELAY 10 /*arbitrary delay in milliseconds for the fastest speed instruction 'SPEED' allows*/
#define MAXSPEEDDELAY 1810 /*arbitrary delay in milliseconds for the slowest speed instruction 'SPEED' allows*/

struct prog{
   char wds[MAXNUMTOKENS][MAXTOKENSIZE];
   int cw; /* Current word */
   double variabletracker[ALPHABET]; /*records the current value each of our variables A-Z is set to within a program. 0 means the variable is unset*/
   double variablezeroflag[ALPHABET]; /*if a variable is set to 0 then it throws up the relevant flag in this array*/  
   int doloopflag; /* keeps track of number of 'open' DO loops - the following 4 parts of 'prog' are for keeping track of DO loops */
   int cnt[MAXDOLOOPS];     /*each 'nth' value in this array signifies the number of repetitions of the nth open do loop*/
   int length[MAXDOLOOPS];   /*each 'nth' value in this array signifies the length of the nth open do loop*/
   int dovar[MAXDOLOOPS]; /*stores the key variable affected by any do loop*/
   double polishresult; /*necessary for instruction 'SET'*/
};
typedef struct prog Program;

struct turtpos{
   double current[2]; /* current cartesian coordinates of the turtle, stored as: (x,y) */ 
   double prev[2];
   double prevprev[2];
   double angle;  /*current angle that the turtle is facing in radians from -pi to pi: where 0 is due East, negative direction is clockwise and positive direction is anticlockwise */
   int triangle; /*1 or 0 - do we want a visual triangular representation of the turtle?*/
   int nopen; /*1 or 0 - if on then the pen doesnt draw*/
   int speed; /*speed of displayed lines :- between 1 and 10*/
};
typedef struct turtpos Turtleposition;

struct stack{
   double contents[MAXSTACKSIZE];
   int top;
};
typedef struct stack Stack;

void Initprog(Program *p); 
void Initturtle(Turtleposition *t);
void Checkfile(int argc);
void Prog(Turtleposition *t, Program *p, SDL_Simplewin *sw);
void Code(Turtleposition *t, Program *p, SDL_Simplewin *sw);
void drawline(Turtleposition *t, SDL_Simplewin *sw);
void drawturtle(Turtleposition *t, SDL_Simplewin *sw, int a);
int Statement(Turtleposition *t, Program *p);
double retrieve(Program *p, int VAR);
int numorcaps(char str[]);
int caps(char str[]);
void checkpolish(Program *p, Stack *s);
void polishcalc(char str[], Stack *s);
double pop(Stack *s);
void push(double a, Stack *s);
char op(char str[]);
void subseqfd(Turtleposition *t, Program *p);
void subseqturn(Turtleposition *t, Program *p);
void subseqdo(Program *p);
void extraintegercond(char str[]);
int subseqset(Program *p);
void executefd(Turtleposition *t, Program *p);
void executeturn(Turtleposition *t, Program *p);
double createvalidangle(double angle);
void executedo(Program *p);
void executeset(int character, double polishoutput, Program *p);
void executeturtle(Turtleposition *t, Program *p);
void subseqturtle(Turtleposition *t, Program *p);
void executenodraw(Turtleposition *t, Program *p);
void subseqnodraw(Turtleposition *t, Program *p);
int dblsame(double a, double b);
void executespeed(Turtleposition *t, Program *p);
void subseqspeed(Turtleposition *t, Program *p);

int main(int argc, char **argv){
   int i;
   FILE *fp;
   Program prog;
   Turtleposition t;
   SDL_Simplewin sw;
   Neill_SDL_Init(&sw);
   Neill_SDL_Events(&sw);    /*this isn't working - maybe we need to move it?*/
   Initprog(&prog);
   Initturtle(&t);
   if(!(fp = fopen(argv[1], "r"))){
      fprintf(stderr, "Cannot open file\n");
      exit(2);
   }
   Checkfile(argc);
   i=0;
   while(fscanf(fp, "%s", prog.wds[i++])==1 && i<MAXNUMTOKENS);
   assert(i<MAXNUMTOKENS);
   Prog(&t, &prog, &sw);
   SDL_Delay(PROGENDPAUSE);
   atexit(SDL_Quit);
   fclose(fp);
   return(0);
}

/*initialising all parts of an input Program structure 'p'*/
void Initprog(Program *p){
   int i;
   p->cw = 0;
   p->doloopflag=0;
   p->polishresult=0;
   for(i=0; i<MAXNUMTOKENS; i++){
      p->wds[i][0]='\0'; 
   }
   for(i=0; i<ALPHABET; i++){
      p->variabletracker[i]=0;
      p->variablezeroflag[i]=0;
   }        
   for(i=0; i<MAXDOLOOPS; i++){
      p->cnt[i]=0;
      p->length[i]=0;
      p->dovar[i]=0;
   }
}

/*initialising all parts of an input Turtle structure 't'*/
void Initturtle(Turtleposition *t){
   t->angle=M_PI/2;
   t->current[0]=STARTX;
   t->current[1]=STARTY;
   t->prev[0]=STARTX;
   t->prev[1]=STARTY;
   t->prevprev[0]=STARTX;
   t->prevprev[1]=STARTY; /*for the smooth function - might need to remove...*/
   t->triangle=0;
   t->nopen=0;
   t->speed=ARBDELAY;
}

/*Error printed & program exitted if there aren't 2 inputs. */
void Checkfile(int argc){
   if(argc!=NUM_ARGS){ 
      fprintf(stderr, "ERROR: Wrong number of file inputs\n");
      exit(1);
   }
}

/* The following function checks the first token to see if it is an opening brace. It then calls function
'code' which is a recursive loop for checking successive tokens */
void Prog(Turtleposition *t, Program *p, SDL_Simplewin *sw){
   if(strsame(p->wds[p->cw], "{")==0){
      ERROR("No opening '{'?");
   }
   p->cw = p->cw+1;
   Code(t, p, sw);
}

/* Code checks to see if a token is the close brace character/string. If it is not then it checks what that
token is via statement. If it is, then it returns and so ends itself. */ /*also performs do loop*/
void Code(Turtleposition *t, Program *p, SDL_Simplewin *sw){
   if(strsame(p->wds[p->cw], "}")){
      if(p->doloopflag==0){
         return;
      }
      else{
         if(p->cnt[p->doloopflag]!=0){                      /*creating a doloop if DO has been called as an instruction*/
            p->cw=(p->cw)-(p->length[p->doloopflag]);
            
            if(p->cnt[p->doloopflag]>0){
               p->cnt[p->doloopflag]=p->cnt[p->doloopflag]-1;
               p->variabletracker[p->dovar[p->doloopflag]]++;
            }
            else{
               p->cnt[p->doloopflag]=p->cnt[p->doloopflag]+1;
               p->variabletracker[p->dovar[p->doloopflag]]--;
            }
            if(dblsame(p->variabletracker[p->dovar[p->doloopflag]],0)){    /*setting the variable zero flag if we go through zero 
- since zero is our default meaning no set variable, we need an additional set of flags*/
               p->variablezeroflag[p->dovar[p->doloopflag]]=1; 
            }
            else{
               p->variablezeroflag[p->dovar[p->doloopflag]]=0; 
            }
         }
         else{
            p->doloopflag=p->doloopflag-1; 
         }
      }
   } 
   Statement(t, p);
   if(t->nopen==0){
      drawline(t, sw);
   }
   if(t->triangle==1){
      drawturtle(t, sw, RGBMAX);
   }
   /*put the smooth function here**************************************************/
   SDL_Delay(t->speed);
   if(t->triangle==1){
      drawturtle(t, sw, 0); /*deletes previous turtle pic so that we only ever have one representation on the screen at a time*/
   }
   p->cw = p->cw+1;
   Code(t, p, sw);
}

void drawline(Turtleposition *t, SDL_Simplewin *sw){
   Neill_SDL_SetDrawColour(sw, RGBMAX, RGBMAX, RGBMAX);
   Neill_SDL_RenderDrawLine(sw->renderer, (int)t->prev[0], (int)t->prev[1], (int)t->current[0], (int)t->current[1]);
   SDL_RenderPresent(sw->renderer);
   SDL_UpdateWindowSurface(sw->win);
}

void drawturtle(Turtleposition *t, SDL_Simplewin *sw, int toggledelete){
   int th=TRIANGLEHEIGHT; 
   int tw=TRIANGLEWIDTH; /*half triangle width*/
   Neill_SDL_SetDrawColour(sw, toggledelete, 0, 0);
   Neill_SDL_RenderDrawLine(sw->renderer, (int)(t->current[0]+(th*cos(t->angle))), (int)(t->current[1]+(th*sin(t->angle))), (int)(t->current[0]-(tw*sin(t->angle))), (int)(t->current[1]+(tw*cos(t->angle))));
   Neill_SDL_RenderDrawLine(sw->renderer, (int)(t->current[0]+(th*cos(t->angle))), (int)(t->current[1]+(th*sin(t->angle))), (int)(t->current[0]+(tw*sin(t->angle))), (int)(t->current[1]-(tw*cos(t->angle))));
   Neill_SDL_RenderDrawLine(sw->renderer, (int)(t->current[0]-(tw*sin(t->angle))), (int)(t->current[1]+(tw*cos(t->angle))), (int)(t->current[0]+(tw*sin(t->angle))), (int)(t->current[1]-(tw*cos(t->angle))));
   SDL_RenderPresent(sw->renderer);
   SDL_UpdateWindowSurface(sw->win);
   Neill_SDL_Events(sw);
}

/*the following function returns 1 if two doubles are the same to 9 decimal places */
int dblsame(double a, double b){
   if(fabs(a-b)<1/DBL_MAX){
      return(1);
   }
   return(0);
}

/* Checks a token to see if it is valid.*/
int Statement(Turtleposition *t, Program *p){
   double d=0;
   int i=0;
   Stack s;
   s.top=0;
   if(strsame(p->wds[p->cw], "FD")){
      subseqfd(t, p);
      return(0);
   }
   if(strsame(p->wds[p->cw], "LT") || (strsame(p->wds[p->cw], "RT"))){ 
      subseqturn(t, p);
      return(0);
   }
   if(strsame(p->wds[p->cw], "DO")){
      subseqdo(p);
      return(0);
   }
   if(strsame(p->wds[p->cw], "SET")){
      i=subseqset(p);
      checkpolish(p, &s);
      d=p->polishresult;
      executeset(i, d, p);
      return(0);
   }
   if(strsame(p->wds[p->cw], "TURTLE")){
      subseqturtle(t, p);
      return(0);
   }
   if(strsame(p->wds[p->cw], "NODRAW")){
      subseqnodraw(t, p);
      return(0);
   }
   if(strsame(p->wds[p->cw], "SPEED")){
      subseqspeed(t, p);
      return(0);
   }
/*
   if(strsame(p->wds[p->cw], "COLOUR")){
      subseqnocolour(t, p);
      return(0);
   }*/
/*
   if(strsame(p->wds[p->cw], "SMOOTH")){
      subseqnosmooth(t, p);
      return(0);
   }*/
   if(strsame(p->wds[p->cw], "}")){
      return(0);
   }
   ERROR("Expecting an FD, LT, RT, DO, SET or '}' instruction");
}

/*retrieve takes a variable and returns a real value if one has been assigned. If not, then it returns an ERROR message */
/*checks temporary array of 26 doubles and check if that is non-zero and return the value*/
double retrieve(Program *p, int VAR){ 
   double d=0;
   int letter=VAR-'A';
   d=p->variabletracker[letter];
   if(!dblsame(d, 0)){
      return(d);
   }
   if(dblsame(d, 0) && !dblsame(p->variablezeroflag[letter], 0)){
      return(0); /* if a variable has been set to 0 */
   }
   if(dblsame(d, 0) && dblsame(p->variablezeroflag[letter], 0)){
      ERRORTOKEN("Variable that was used has not previously been assigned", p->cw);
   }
   return(0);
}

/*to check all values subsequent to the instruction 'FD':*/
void subseqfd(Turtleposition *t, Program *p){
   p->cw++;
   if(!numorcaps(p->wds[p->cw])){
      ERRORNEXT("a number or letter variable", "'FD'", p->cw);
   }
   executefd(t, p);
}

/*executefd takes the instruction FD and the following number and implements it onto the turtle's position*/
void executefd(Turtleposition *t, Program *p){
   double dist=0;
   char num;
   if(caps(p->wds[p->cw])){
      sscanf(p->wds[p->cw], "%c", &num);
      dist=retrieve(p, (int)num);
   }
   else{
      sscanf(p->wds[p->cw], "%lf", &dist);
   }
   t->prevprev[0]=t->prev[0];
   t->prevprev[1]=t->prev[1];
   t->prev[0]=t->current[0];
   t->prev[1]=t->current[1];
   t->current[0]=t->current[0]+(dist*cos(t->angle));
   t->current[1]=t->current[1]+(dist*sin(t->angle));
}

/*to check all values following 'RT' or 'LT':*/
void subseqturn(Turtleposition *t, Program *p){
   p->cw++;
   if(!numorcaps(p->wds[p->cw])){
      ERRORNEXT("a number or letter variable", "turn instruction", p->cw);
   }
   executeturn(t, p);
}

/*executeturn takes the value following 'LT' or 'RT' and changes the 'angle' within the turtle structure by adding or subtracting
the input angle as approriate via polar coordinates convention */
void executeturn(Turtleposition *t, Program *p){
   double ang=0;
   char num;
   if(caps(p->wds[p->cw])){
      sscanf(p->wds[p->cw], "%c", &num);
      ang=retrieve(p, (int)num);
   }
   else{
      sscanf(p->wds[p->cw], "%lf", &ang);
   }
   ang=ang*DEGREESTORADIANS;
   if(strsame(p->wds[p->cw-1], "LT")){
      t->angle=t->angle+ang;
   }
   else{
      t->angle=t->angle-ang;
   }
   t->angle=createvalidangle(t->angle);
}

/*since we are adding/subtracting angles from each other we use the following function to ensure that the result of the angle sum
remains in our specified range for polar coordinates */
double createvalidangle(double angle){
   while(angle>=M_PI){
      angle=angle-(2*M_PI);
   }
   while(angle<(-M_PI)){
      angle=angle+(2*M_PI);
   }
   return(angle);
}

/* to check values subsequent to 'DO'. Subseqdo puts together 'DO' related error messages and calls the next stage of processing 
a 'DO' loop. */
void subseqdo(Program *p){
   p->cw++;
   if(!caps(p->wds[p->cw])){
      ERRORNEXT("a capital letter variable", "'DO'", p->cw);
   }
   p->cw++;
   if(!strsame(p->wds[p->cw], "FROM")){
      ERRORNEXT("'FROM'", "the preceding variable", p->cw);
   }
   p->cw++;
   if(!numorcaps(p->wds[p->cw])){
      ERRORNEXT("a number or letter variable", "'FROM'", p->cw);
   }
   extraintegercond(p->wds[p->cw]);
   p->cw++;
   if(!strsame(p->wds[p->cw], "TO")){
      ERRORNEXT("'TO'", "the preceding variable", p->cw);
   }
   p->cw++;
   if(!numorcaps(p->wds[p->cw])){
      ERRORNEXT("a number or letter variable", "'TO'", p->cw);
   }
   extraintegercond(p->wds[p->cw]);
   p->cw++;
   if(!strsame(p->wds[p->cw], "{")){
      ERRORNEXT("'{'", "the preceding variable", p->cw);
   }
   p->doloopflag++;
   executedo(p);
}

/*executedo following function is in charge of checking that the do loop is valid (only integers, not decimal numbers) putting the initial
 variables into the array of 26 in the structure program as well as setting the structure up for the loop itself. */
void executedo(Program *p){
   int i=0, start=0, end=0, key, flag=0;
   key=(int)p->wds[(p->cw)-FIRSTDOGAP][0]-'A';
   if(p->wds[p->cw-SECONDDOGAP][0]>='A'&&p->wds[p->cw-SECONDDOGAP][0]<='Z'){
      start=(int)retrieve(p, (int)p->wds[p->cw-SECONDDOGAP][0]);
   }
   else{
      sscanf(p->wds[p->cw-SECONDDOGAP], "%d", &start);
   }
   p->variabletracker[key]=(double)start;
   if(dblsame((double)start,0)){
      p->variablezeroflag[key]=1;
   }
   if(p->wds[p->cw-THIRDDOGAP][0]>='A'&&p->wds[p->cw-THIRDDOGAP][0]<='Z'){
      end=(int)retrieve(p, (int)p->wds[p->cw-THIRDDOGAP][0]);
   }
   else{
      sscanf(p->wds[p->cw-THIRDDOGAP], "%d", &end);
   }    
   p->dovar[p->doloopflag]=key;
   p->cnt[p->doloopflag]=(end-start);
   while(!strsame(p->wds[p->cw+i], "}") || flag!=0){
      if(strsame(p->wds[p->cw+i], "DO")){
         flag++;
      }
      if(strsame(p->wds[p->cw+i], "}")){
         flag=flag-1;
      }
      i++;
   }
   p->length[p->doloopflag]=i-1;           
}

/*for the Do loop we can only deal with integer inputs so need an extra condition*/
void extraintegercond(char str[]){
   int temp=0;
   int i=0, flag=0;
   char tempstring[MAXTOKENSIZE];
   for(i=0; i<MAXTOKENSIZE; i++){
      if(flag==1){
         tempstring[i]=str[i];
      }      
      if(str[i]=='.'){
         flag++;
      }
      else{
         tempstring[i]=0;
      }
      printf("%c", tempstring[i]);
   }
 
   sscanf(tempstring, "%d", &temp);
   /*if(caps(str)==0 && (temp==0)){
      ERROR("retrieved value in a 'DO' loop is not an integer but needs to be,");
   }*/
}

/* to check all values subsequent to 'SET': package together all things which must follow the instruction set - including error messages*/
/*should output the ascii code of the key variable that we are setting*/
int subseqset(Program *p){
   char temp;
   p->cw++;
   sscanf(p->wds[p->cw], "%c", &temp);
   if(!caps(p->wds[p->cw])){
      ERRORNEXT("a capital letter variable", "'SET'", p->cw);
   }
   p->cw++;
   if(!strsame(p->wds[p->cw], ":=")){
      ERRORNEXT("':='", "the preceding variable", p->cw);
   }
   p->cw++;
   return((int)temp);
}

/*The following function is in charge of checking that the polish in set is valid and produces a number for the relevant assignment, and if a number is allocated it 
changes the value in the array of 26 in the structure program */
void executeset(int character, double polishoutput, Program *p){
   p->variabletracker[character-'A']=polishoutput;
   if(dblsame(polishoutput,0)){
      p->variablezeroflag[character-'A']=1;
   }
}

/*following function produces an error message if the polish is not of the right format. otherwise it returns the
number of tokens used after ':=' in the instruction 'set'*/
void checkpolish(Program *p, Stack *s){ 
   double a=0, output=0;
   char num=0;
   if(strsame(p->wds[p->cw], ";")){
      if(s->top==1){
         output=pop(s);
         p->polishresult=output; 
      }
      else{
         ERROR("Need fewer operands in polish expression. Current expression has no unique output.");
      }
   }
   if(!op(p->wds[p->cw]) && numorcaps(p->wds[p->cw]) && !strsame(p->wds[p->cw], ";")){    
      if(caps(p->wds[p->cw])){
         sscanf(p->wds[p->cw], "%c", &num);
         a=retrieve(p, (int)num);
      }
      else{
         sscanf(p->wds[p->cw], "%lf", &a);
      }
      push(a, s);
      p->cw=p->cw+1;
      checkpolish(p, s);
   }
   if(op(p->wds[p->cw]) && !strsame(p->wds[p->cw], ";") && !numorcaps(p->wds[p->cw])){
      polishcalc(p->wds[p->cw], s);
      p->cw=p->cw+1;
      checkpolish(p, s);
   }
   if(!numorcaps(p->wds[p->cw]) && !op(p->wds[p->cw]) && !strsame(p->wds[p->cw], ";")){
      ERRORTOKEN("Expecting a Polish expression (operation, variable or ';')", p->cw+1);
   }

}

/*push function created specifically for putting an input value as a new top value of a stack and incrementing the counter 'top'*/
void push(double a, Stack *s){
   s->contents[s->top]=a;
   s->top=s->top+1;
}

/*pop function created to remove a function from the top of a stack and reduce the top counter by 1*/
double pop(Stack *s){
   double temp;
   s->top=s->top-1;
   temp=s->contents[s->top];
   s->contents[s->top]=0;
   return(temp);
}

void subseqturtle(Turtleposition *t, Program *p){
   p->cw++;
   if(!strsame(p->wds[p->cw], "ON") && !strsame(p->wds[p->cw], "OFF")){
      ERRORNEXT("'ON' or 'OFF'", "'TURTLE'", p->cw);
   }
   executeturtle(t, p);
}

void executeturtle(Turtleposition *t, Program *p){
   if(strsame(p->wds[p->cw], "ON")){
      t->triangle=1;
   }
   if(strsame(p->wds[p->cw], "OFF")){
      t->triangle=0;
   }
}

void subseqnodraw(Turtleposition *t, Program *p){
   p->cw++;
   if(!strsame(p->wds[p->cw], "ON") && !strsame(p->wds[p->cw], "OFF")){
      ERRORNEXT("'ON' or 'OFF'", "'NODRAW'", p->cw);
   }
   executenodraw(t, p);
}

void executenodraw(Turtleposition *t, Program *p){
   if(strsame(p->wds[p->cw], "ON")){
      t->nopen=1;
   }
   if(strsame(p->wds[p->cw], "OFF")){
      t->nopen=0;
   }
}

void subseqspeed(Turtleposition *t, Program *p){
   p->cw++;
   if(!numorcaps(p->wds[p->cw])){
      ERRORNEXT("a number or a predefined variable", "'SPEED'", p->cw);
   }
   executespeed(t, p);
}

void executespeed(Turtleposition *t, Program *p){
   double temp;
   char num;
   if(caps(p->wds[p->cw])){
      sscanf(p->wds[p->cw], "%c", &num);
      temp=retrieve(p, (int)num);
   }
   else{
      sscanf(p->wds[p->cw], "%lf", &temp);
   }
   if(temp<MINSPEED || temp>MAXSPEED){
      ERROR("Speed must be set between 1 and 10");
   }
   t->speed=((MAXSPEED-temp)*((MAXSPEEDDELAY-MINSPEEDDELAY)/(MAXSPEED-MINSPEED)))+MINSPEED; 
}
/*
void subseqcolour(Turtleposition *t, Program *p){
   p->cw++;
   if(!strsame(p->wds[p->cw], "GREEN") && !strsame(p->wds[p->cw], "WHITE")){
!colourname(p->wds[p->cw])
      ERROR("Token 'COLOUR' must be followed by a defined colour");
   }
   executecolour(t, p);
}

void executecolour(Turtleposition *t, Program *p){
   char colour;
   sscanf(p->wds[p->cw], "%c", &colour);
   if(strsame(p->wds[p->cw], "OFF")){
      
   }
}*/

/*
void subseqsmooth(Turtleposition *t, Program *p){
   p->cw++;
   if(!strsame(p->wds[p->cw], "ON") && !strsame(p->wds[p->cw], "OFF")){
      ERRORNEXT("'ON' or 'OFF'", "'NODRAW'", p->cw);
   }
   executenodraw(t, p);
}

void executesmooth(Turtleposition *t, Program *p){
   if(strsame(p->wds[p->cw], "ON")){
      t->nopen=1;
   }
   if(strsame(p->wds[p->cw], "OFF")){
      t->nopen=0;
   }
}*/

/*using a previously created stack, polishcalc takes in an operation in the type string. it pops the two values off a given stack 
and pushes the result back onto the stack*/
void polishcalc(char str[], Stack *s){
   double x=0, y=0;
   if((s->top)<MINBINARYOP){
      ERROR("Need fewer operations in polish expression. Current expression has no unique output.");
   }
   y=pop(s);
   x=pop(s);
   if(strsame(&str[0], "+")){
      push(x+y, s);
   }
   if(strsame(&str[0], "-")){
      push(x-y, s);
   }
   if(strsame(&str[0], "*")){
      push(x*y, s);
   }
   if(strsame(&str[0], "/")){
      if(dblsame(y, 0)){
         ERROR("Attempted division by 0 within polish!");
      }
      push(x/y, s);
   }   
}

/*function which returns 1 if a string is just an operation and 0 if it is not*/
char op(char str[]){
   if((strsame(&str[0], "+") || strsame(&str[0], "-") || strsame(&str[0], "*") || strsame(&str[0], "/")) && strlen(str)==1){
      return(str[0]);
   }
   return(0);
}

/* we want the following to return a 1 if a string is a valid number or capital letter according to the formal grammar
 and a 0 if not */
int numorcaps(char str[]){  
   int cnt=0, i=0, n=0;
   if(caps(str)){
      return(1);
   }
   if(str[0]=='.'){
      cnt++;
   }
   if(str[0]!='-'&&(isdigit((int)str[0])==0)&&str[0]!='.'){
      return(0);
   }
   n=strlen(str);
   for(i=1; i<n-1; i++){
      if(str[i]!='.'&&(isdigit((int)str[i])==0)&&str[i]!='\0'){
         return(0);
      }
      if(str[i]=='.'){
         cnt++;
      }
   }
   if((cnt!=0)&&(cnt!=1)){
      return(0);
   }
   return(1);
}

/*a function which returns 1 if a string is a single capital letter (variable) and 0 if it is not*/
int caps(char str[]){ 
   if(str[0]>='A'&&str[0]<='Z'&&str[1]=='\0'){
      return(1);
   }
   return(0);
}


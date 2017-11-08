/*This file is created for the completeness test (to check which lines of which functions we are using)*/
/*In order to do this we have redacted the SDL parts of the interpreter marked (commented out for the completeness test) */
/*N.B. we don't need to check the use of errors as this has been done separately*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <float.h>
/*#include "neillsdl2.h"*/ /*commented out for the completeness test*/

#define MAXNUMTOKENS 100
#define MAXTOKENSIZE 15
#define ALPHABET 26
#define MAXDOLOOPS 15
#define MAXSTACKSIZE 100
#define strsame(A,B) (strcmp(A, B)==0)  /* if A and B are the same it returns a 1*/
#define ERROR(PHRASE) {fprintf(stderr, "Fatal Error: %s occured in %s, line %d\n.", PHRASE, __FILE__, __LINE__); exit(2);}
#define ERRORTOKEN(PHRASE, cw) {fprintf(stderr, "Fatal Error: %s occurred for input token number %d of your program.\n", PHRASE, cw); exit(2);}
#define ERRORNEXT(X, Y, cw) {fprintf(stderr, "Fatal Error: Expecting %s following %s for input token number %d of your program.\n", X, Y, cw+1); exit(2);}
#define DEGREESTORADIANS (2*M_PI)/360
#define FIRSTDOGAP 5
#define SECONDDOGAP 3
#define THIRDDOGAP 1
#define MAXUSABLESTACKSPACE MAXSTACKSIZE-2
#define MINBINARYOP 2
#define NUM_ARGS 2
#define STARTX 0
#define STARTY 0
#define ARBDELAY 20
#define PROGENDPAUSE 2500
#define RGBMAX 255
#define NUMBERCOMPFLAG 85

struct prog{
   char wds[MAXNUMTOKENS][MAXTOKENSIZE];
   int cw; /* Current word */
   double variabletracker[ALPHABET]; /*records the current value each of our variables A-Z is set to within a program. 0 means the variable is unset*/
   double variablezeroflag[ALPHABET]; /*if a variable is set to 0 then it throws up the relevant flag in this array*/  
   int doloopflag; /* keeps track of number of 'open' DO loops - the following 4 parts of 'prog' are for keeping track of DO loops */
   int cnt[MAXDOLOOPS];     /*each 'nth' value in this array signifies the number of repetitions of the nth open do loop*/
   int length[MAXDOLOOPS];   /*each 'nth' value in this array signifies the length of the nth open do loop*/
   int dovar[MAXDOLOOPS]; /*stores the key variable affected by any do loop*/
   double polishresult; /*necessary for set polish*/
   int compflag[NUMBERCOMPFLAG];
};
typedef struct prog Program;

struct turtpos{
   double current[2]; /* current cartesian coordinates of the turtle, stored as: (x,y) */
   double prev[2];
   double angle;  /*current angle that the turtle is facing in radians from -pi to pi: where 0 is due East, negative direction is clockwise and 
               positive direction is anticlockwise */
};
typedef struct turtpos Turtleposition;

struct stack{
   double contents[MAXSTACKSIZE];
   int top;
};
typedef struct stack Stack;

void Initprog(Program *p); 
void Initturtle(Turtleposition *t, Program *p);
void Checkfile(int argc, Program *p);
void Prog(Turtleposition *t, Program *p/*, SDL_Simplewin *sw*/);
void Code(Turtleposition *t, Program *p/*, SDL_Simplewin *sw*/);
void drawline(Turtleposition *t/*, SDL_Simplewin *sw*/);
void drawturtle(Turtleposition *t/*, SDL_Simplewin *sw*/, int a);
int Statement(Turtleposition *t, Program *p);
double retrieve(Program *p, int VAR);
int numorcaps(char str[], Program *p);
int caps(char str[], Program *p);
void checkpolish(Program *p, Stack *s);
void polishcalc(char str[], Stack *s, Program *p);
double pop(Stack *s, Program *p);
void push(double a, Stack *s, Program *p);
char op(char str[], Program *p);
void subseqfd(Turtleposition *t, Program *p);
void subseqturn(Turtleposition *t, Program *p);
void subseqdo(Program *p);
void extraintegercond(char str[], Program *p);
int subseqset(Program *p);
void executefd(Turtleposition *t, Program *p);
void executeturn(Turtleposition *t, Program *p);
double createvalidangle(double angle, Program *p);
void executedo(Program *p);
void executeset(int character, double polishoutput, Program *p);
int dblsame(double a, double b, Program *p);
void printcoord(double prevx, double prevy, Turtleposition *t);

int main(int argc, char **argv){
   int i;
   FILE *fp;
   Program prog;
   Turtleposition t;
   /*SDL_Simplewin sw; */        /*commented out for the completeness test*/
   /*Neill_SDL_Init(&sw);*/      /*commented out for the completeness test*/
   /*Neill_SDL_Events(&sw);*/    /*commented out for the completeness test*/
   Initprog(&prog);
   Initturtle(&t, &prog);
   if(!(fp = fopen(argv[1], "r"))){
      fprintf(stderr, "Cannot open file\n");
      exit(2);
   }
   Checkfile(argc, &prog);
   i=0;
   while(fscanf(fp, "%s", prog.wds[i++])==1 && i<MAXNUMTOKENS);
   assert(i<MAXNUMTOKENS);
   Prog(&t, &prog/*, &sw*/);
/*   SDL_Delay(PROGENDPAUSE);*/ /*final delay*/  /*commented out for the completeness test*/
   /*atexit(SDL_Quit);*/
   fclose(fp);
   printf("COMPLETENESS ARRAY: {");
   for(i=0;i<NUMBERCOMPFLAG-1;i++){
      printf("%d,", prog.compflag[i]);
   }
   printf("%d}\n", prog.compflag[NUMBERCOMPFLAG-1]);
   return(0);
}

/*initialising all parts of an input Program structure 'p'*/
void Initprog(Program *p){
   int i;
   p->cw = 0;
   p->doloopflag=0;
   for(i=0; i<MAXNUMTOKENS; i++){
      p->compflag[0]=1;
      p->wds[i][0]='\0'; 
   }
   for(i=0; i<ALPHABET; i++){
      p->compflag[1]=1;
      p->variabletracker[i]=0;
      p->variablezeroflag[i]=0;
   }        
   for(i=0; i<MAXDOLOOPS; i++){
      p->compflag[2]=1;
      p->cnt[i]=0;
      p->length[i]=0;
      p->dovar[i]=0;
   }
   for(i=3;i<NUMBERCOMPFLAG;i++){
      p->compflag[i]=0;
      p->compflag[3]=1;
   }
}

/*initialising all parts of an input Turtle structure 't'*/
void Initturtle(Turtleposition *t, Program *p){
   p->compflag[4]=1;
   t->angle=M_PI/2;
   t->current[0]=STARTX;
   t->current[1]=STARTY;
   t->prev[0]=STARTX;
   t->prev[1]=STARTY;
}

/*Error printed & program exitted if there aren't 2 inputs. */
void Checkfile(int argc, Program *p){
   p->compflag[5]=1;
   if(argc!=NUM_ARGS){ 
      fprintf(stderr, "ERROR: Wrong number of file inputs\n");
      exit(1);
   }
}

/* The following function checks the first token to see if it is an opening brace. It then calls function
'code' which is a recursive loop for checking successive tokens */
void Prog(Turtleposition *t, Program *p/*, SDL_Simplewin *sw*/){
   p->compflag[6]=1;
   if(strsame(p->wds[p->cw], "{")==0){
      ERROR("No opening '{'?");
   }
   p->cw = p->cw+1;
   Code(t, p/*, sw*/);
}

/* Code checks to see if a token is the close brace character/string. If it is not then it checks what that
token is via statement. If it is, then it returns and so ends itself. */ /*also performs do loop*/
void Code(Turtleposition *t, Program *p/*, SDL_Simplewin *sw*/){
   p->compflag[7]=1;
   if(strsame(p->wds[p->cw], "}")){
      p->compflag[8]=1;
      if(p->doloopflag==0){
         p->compflag[9]=1;
         return;
      }
      else{
         p->compflag[10]=1;
         if(p->cnt[p->doloopflag]!=0){                      /*creating a doloop if DO has been called as an instruction*/
            p->compflag[11]=1;
            p->cw=(p->cw)-(p->length[p->doloopflag]);
            
            if(p->cnt[p->doloopflag]>0){
               p->compflag[12]=1;
               p->cnt[p->doloopflag]=p->cnt[p->doloopflag]-1;
               p->variabletracker[p->dovar[p->doloopflag]]++;
            }
            else{
               p->compflag[13]=1;
               p->cnt[p->doloopflag]=p->cnt[p->doloopflag]+1;
               p->variabletracker[p->dovar[p->doloopflag]]--;
            }
            if(dblsame(p->variabletracker[p->dovar[p->doloopflag]],0, p)){    /*setting the variable zero flag if we go through zero*/
               p->compflag[14]=1;
               p->variablezeroflag[p->dovar[p->doloopflag]]=1; 
            }
            else{
               p->compflag[15]=1;
               p->variablezeroflag[p->dovar[p->doloopflag]]=0;
            }
         }
         else{
            p->compflag[16]=1;
            p->doloopflag=p->doloopflag-1; 
         }
      }
   } /*put all the above in a function!!!!!!!!!!!!*/
   Statement(t, p);
   
   /*drawline(t, sw);*/    /*commented out for the completeness test*/
   /*SDL_Delay(ARBDELAY);*/   /*commented out for the completeness test*/
   p->cw = p->cw+1;
   Code(t, p/*, sw*/);
}

/*void drawline(Turtleposition *t, SDL_Simplewin *sw){
   Neill_SDL_SetDrawColour(sw, RGBMAX, RGBMAX, RGBMAX);
   Neill_SDL_RenderDrawLine(sw->renderer, (int)t->prev[0], (int)t->prev[1], (int)t->current[0], (int)t->current[1]);
   
   SDL_RenderPresent(sw->renderer);
   SDL_UpdateWindowSurface(sw->win);
   Neill_SDL_Events(sw);
}*/ /*commented out for the completeness test*/

/*the following function returns 1 if two doubles are the same to 9 decimal places */
int dblsame(double a, double b, Program *p){
   p->compflag[16]=1;
   if(fabs(a-b)<1/DBL_MAX){
      p->compflag[17]=1;
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
   p->compflag[18]=1;
   if(strsame(p->wds[p->cw], "FD")){
      p->compflag[19]=1;
      subseqfd(t, p);
      return(0);
   }
   if(strsame(p->wds[p->cw], "LT") || (strsame(p->wds[p->cw], "RT"))){ 
      p->compflag[20]=1;
      subseqturn(t, p);
      return(0);
   }
   if(strsame(p->wds[p->cw], "DO")){
      p->compflag[21]=1;
      subseqdo(p);
      return(0);
   }
   if(strsame(p->wds[p->cw], "SET")){
      p->compflag[22]=1;
      i=subseqset(p);
      checkpolish(p, &s);
      d=p->polishresult;
      executeset(i, d, p);
      return(0);
   }
   if(strsame(p->wds[p->cw], "}")){
      p->compflag[23]=1;
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
   p->compflag[24]=1;
   if(!dblsame(d, 0, p)){
      p->compflag[25]=1;
      return(d);
   }
   if(dblsame(d, 0, p) && !dblsame(p->variablezeroflag[letter], 0, p)){
      p->compflag[26]=1;
      return(0); /* if a variable has been set to 0 */
   }
   if(dblsame(d, 0, p) && dblsame(p->variablezeroflag[letter], 0, p)){
      ERRORTOKEN("Variable that was used has not previously been assigned", p->cw);
   }
   return(0);
}

/*to check all values subsequent to the instruction 'FD':*/
void subseqfd(Turtleposition *t, Program *p){
   p->compflag[27]=1;
   p->cw++;
   if(!numorcaps(p->wds[p->cw], p)){
      ERRORNEXT("a number or letter variable", "'FD'", p->cw);
   }
   executefd(t, p);
}

void printcoord(double prevx, double prevy, Turtleposition *t){
   printf("From (%f,%f) ", prevx, prevy);
   printf("to (%f,%f), ", t->current[0], t->current[1]);
   printf("with a facing of %f radians.\n", t->angle);
}

/*executefd takes the instruction FD and the following number and implements it onto the turtle's position*/
void executefd(Turtleposition *t, Program *p){
   double dist=0;
   char num;
   p->compflag[28]=1;
   if(caps(p->wds[p->cw], p)){
      p->compflag[29]=1;
      sscanf(p->wds[p->cw], "%c", &num);
      dist=retrieve(p, (int)num);
   }
   else{
      p->compflag[30]=1;
      sscanf(p->wds[p->cw], "%lf", &dist);
   }
   t->prev[0]=t->current[0];
   t->prev[1]=t->current[1];
   t->current[0]=t->current[0]+(dist*cos(t->angle));
   t->current[1]=t->current[1]+(dist*sin(t->angle));
   printcoord(t->prev[0], t->prev[1], t);
}

/*to check all values following 'RT' or 'LT':*/
void subseqturn(Turtleposition *t, Program *p){
   p->compflag[31]=1;
   p->cw++;
   if(!numorcaps(p->wds[p->cw], p)){
      ERRORNEXT("a number or letter variable", "turn instruction", p->cw);
   }
   executeturn(t, p);
}

/*executeturn takes the value following 'LT' or 'RT' and changes the 'angle' within the turtle structure by adding or subtracting
the input angle as approriate via polar coordinates convention */
void executeturn(Turtleposition *t, Program *p){
   double ang=0;
   char num;
   p->compflag[32]=1;
   if(caps(p->wds[p->cw], p)){
      p->compflag[33]=1;
      sscanf(p->wds[p->cw], "%c", &num);
      ang=retrieve(p, (int)num);
   }
   else{
      p->compflag[34]=1;
      sscanf(p->wds[p->cw], "%lf", &ang);
   }
   ang=ang*DEGREESTORADIANS;
   if(strsame(p->wds[p->cw-1], "LT")){
      p->compflag[35]=1;
      t->angle=t->angle+ang;
   }
   else{
      p->compflag[36]=1;
      t->angle=t->angle-ang;
   }
   t->angle=createvalidangle(t->angle, p);
}

/*since we are adding/subtracting angles from each other we use the following function to ensure that the result of the angle sum
remains in our specified range for polar coordinates */
double createvalidangle(double angle, Program *p){
   p->compflag[37]=1;
   while(angle>=M_PI){
      p->compflag[38]=1;
      angle=angle-(2*M_PI);
   }
   while(angle<(-M_PI)){
      p->compflag[39]=1;
      angle=angle+(2*M_PI);
   }
   return(angle);
}

/* to check values subsequent to 'DO'. Subseqdo puts together 'DO' related error messages and calls the next stage of processing 
a 'DO' loop. */
void subseqdo(Program *p){
   p->compflag[40]=1;
   p->cw++;
   if(!caps(p->wds[p->cw], p)){
      ERRORNEXT("a capital letter variable", "'DO'", p->cw);
   }
   p->cw++;
   if(!strsame(p->wds[p->cw], "FROM")){
      ERRORNEXT("'FROM'", "the preceding variable", p->cw);
   }
   p->cw++;
   if(!numorcaps(p->wds[p->cw], p)){
      ERRORNEXT("a number or letter variable", "'FROM'", p->cw);
   }
   extraintegercond(p->wds[p->cw], p);
   p->cw++;
   if(!strsame(p->wds[p->cw], "TO")){
      ERRORNEXT("'TO'", "the preceding variable", p->cw);
   }
   p->cw++;
   if(!numorcaps(p->wds[p->cw], p)){
      ERRORNEXT("a number or letter variable", "'TO'", p->cw);
   }
   extraintegercond(p->wds[p->cw], p);
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
   p->compflag[41]=1;
   key=(int)p->wds[(p->cw)-FIRSTDOGAP][0]-'A';
   if(p->wds[p->cw-SECONDDOGAP][0]>='A'&&p->wds[p->cw-SECONDDOGAP][0]<='Z'){
      p->compflag[42]=1;
      start=(int)retrieve(p, (int)p->wds[p->cw-SECONDDOGAP][0]);
   }
   else{
      p->compflag[43]=1;
      sscanf(p->wds[p->cw-SECONDDOGAP], "%d", &start);
   }
   p->variabletracker[key]=(double)start;
   if(dblsame((double)start,0, p)){
      p->compflag[44]=1;
      p->variablezeroflag[key]=1;
   }
   if(p->wds[p->cw-THIRDDOGAP][0]>='A'&&p->wds[p->cw-THIRDDOGAP][0]<='Z'){
      p->compflag[45]=1;
      end=(int)retrieve(p, (int)p->wds[p->cw-THIRDDOGAP][0]);
   }
   else{
      p->compflag[46]=1;
      sscanf(p->wds[p->cw-THIRDDOGAP], "%d", &end);
   }    
   p->dovar[p->doloopflag]=key;
   p->cnt[p->doloopflag]=(end-start);
   while(!strsame(p->wds[p->cw+i], "}") || flag!=0){
      p->compflag[47]=1;
      if(strsame(p->wds[p->cw+i], "DO")){
         p->compflag[48]=1;
         flag++;
      }
      if(strsame(p->wds[p->cw+i], "}")){
         p->compflag[49]=1;
         flag=flag-1;
      }
      i++;
   }
   p->length[p->doloopflag]=i-1;           
}

/*for the Do loop we can only deal with integer inputs so need an extra condition*/
void extraintegercond(char str[], Program *p){
   int temp=0;
   int i=0, flag=0;
   char tempstring[MAXTOKENSIZE];
   p->compflag[50]=1;
   for(i=0; i<MAXTOKENSIZE; i++){
      p->compflag[51]=1;
      if(flag==1){
         p->compflag[52]=1;
         tempstring[i]=str[i];
      }      
      if(str[i]=='.'){
         p->compflag[53]=1;
         flag++;
      }
      else{
         p->compflag[54]=1;
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
   p->compflag[55]=1;
   p->cw++;
   sscanf(p->wds[p->cw], "%c", &temp);
   if(!caps(p->wds[p->cw], p)){
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
   p->compflag[56]=1;
   p->variabletracker[character-'A']=polishoutput;
   if(dblsame(polishoutput,0, p)){
      p->compflag[57]=1;
      p->variablezeroflag[character-'A']=1;
   }
}

/*following function produces an error message if the polish is not of the right format. otherwise it returns the
number of tokens used after ':=' in the instruction 'set'*/
void checkpolish(Program *p, Stack *s){ 
   double a=0, output=0;
   char num=0;
   p->compflag[58]=1;
   if(strsame(p->wds[p->cw], ";")){
      p->compflag[59]=1;
      if(s->top==1){
         p->compflag[60]=1;
         output=pop(s, p);
         p->polishresult=output; 
      }
      else{
         ERROR("Need fewer operands in polish expression. Current expression has no unique output.");
      }
   }
   if(!op(p->wds[p->cw], p) && numorcaps(p->wds[p->cw], p)){  
      p->compflag[61]=1;  
      if(caps(p->wds[p->cw], p)){
         p->compflag[62]=1;
         sscanf(p->wds[p->cw], "%c", &num);
         a=retrieve(p, (int)num);
      }
      else{
         p->compflag[63]=1;
         sscanf(p->wds[p->cw], "%lf", &a);
      }
      push(a, s, p);
      p->cw=p->cw+1;
      checkpolish(p, s);
   }
   if(op(p->wds[p->cw], p)){
      p->compflag[64]=1;
      polishcalc(p->wds[p->cw], s, p);
      p->cw=p->cw+1;
      checkpolish(p, s);
   }
   if(!numorcaps(p->wds[p->cw], p) && !op(p->wds[p->cw], p) && !strsame(p->wds[p->cw], ";")){
      p->compflag[65]=1;
      ERRORTOKEN("Expecting a Polish expression (operation, variable or ';')", p->cw+1);
   }
/*   return(output);*/
}

/*push function created specifically for putting an input value as a new top value of a stack and incrementing the counter 'top'*/
void push(double a, Stack *s, Program *p){
   p->compflag[66]=1;
   s->contents[s->top]=a;
   s->top=s->top+1;
}

/*pop function created to remove a function from the top of a stack and reduce the top counter by 1*/
double pop(Stack *s, Program *p){
   double temp;
   p->compflag[67]=1;
   s->top=s->top-1;
   temp=s->contents[s->top];
   s->contents[s->top]=0;
   return(temp);
}

/*using a previously created stack, polishcalc takes in an operation in the type string. it pops the two values off a given stack 
and pushes the result back onto the stack*/
void polishcalc(char str[], Stack *s, Program *p){
   double x=0, y=0;
   p->compflag[68]=1;
   if((s->top)<MINBINARYOP){
      ERROR("Need fewer operations in polish expression. Current expression has no unique output.");
   }
   y=pop(s, p);
   x=pop(s, p);
   if(strsame(&str[0], "+")){
      p->compflag[69]=1;
      push(x+y, s, p);
   }
   if(strsame(&str[0], "-")){
      p->compflag[70]=1;
      push(x-y, s, p);
   }
   if(strsame(&str[0], "*")){
      p->compflag[71]=1;
      push(x*y, s, p);
   }
   if(strsame(&str[0], "/")){
      p->compflag[72]=1;
      if(dblsame(y, 0, p)){
         ERROR("Attempted division by 0 within polish!");
      }
      push(x/y, s, p);
   }   
}


/*function which returns 1 if a string is just an operation and 0 if it is not*/
char op(char str[], Program *p){
   p->compflag[73]=1;
   if((strsame(&str[0], "+") || strsame(&str[0], "-") || strsame(&str[0], "*") || strsame(&str[0], "/")) && strlen(str)==1){
      p->compflag[74]=1;
      return(str[0]);
   }
   return(0);
}

/* we want the following to return a 1 if a string is a valid number or capital letter according to the formal grammar
 and a 0 if not */
int numorcaps(char str[], Program *p){  
   int cnt=0, i=0, n=0;
   p->compflag[75]=1;
   if(caps(str, p)){
      p->compflag[76]=1;
      return(1);
   }
   if(str[0]!='-'&&(isdigit((int)str[0])==0)&&str[0]!='.'){
      p->compflag[77]=1;
      if(str[0]=='.'){
         p->compflag[78]=1;
         cnt++;
      }
      return(0);
   }
   n=strlen(str);
   for(i=1; i<n-1; i++){
      p->compflag[79]=1;
      if(str[i]!='.'&&(isdigit((int)str[i])==0)&&str[i]!='\0'){
         p->compflag[80]=1;
         return(0);
      }
      if(str[i]=='.'){
         p->compflag[81]=1;
         cnt++;
      }
   }
   if((cnt!=0)&&(cnt!=1)){
      p->compflag[82]=1;
      return(0);
   }
   return(1);
}

/*a function which returns 1 if a string is a single capital letter (variable) and 0 if it is not*/
int caps(char str[], Program *p){ 
   p->compflag[83]=1;
   if(str[0]>='A'&&str[0]<='Z'&&str[1]=='\0'){
      p->compflag[84]=1;
      return(1);
   }
   return(0);
}


/* Code which takes an input set of instructions and parses them to decide if they conform to 
the definition of the turtle language */
/*NOTE: we have made a minor adaptation to the formal grammer which we have taken into accounts is more rigorous regulations 
with regards to placing a '}' symbol in an input program. In this parser, the '}' symbol can only occur at the end of a program 
of at the end of a 'DO' loop We have also defined a 'number' to be a positive or negative decimal number. */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>

#define MAXNUMTOKENS 1000
#define MAXTOKENSIZE 30
#define NUM_ARGS 2
#define ALPHABET 26
#define strsame(A,B) (strcmp(A, B)==0)  /* if A and B are the same it returns a 1*/
#define ERROR(PHRASE) {fprintf(stderr, "Fatal Error: %s occured in %s, line %d\n.", PHRASE, __FILE__, __LINE__); exit(2);}
#define ERRORNEXT(X, Y, cw) {fprintf(stderr, "Fatal Error: Expecting %s following %s for input token number %d of your program.\n", X, Y, cw); exit(2);}
#define ERRORTOKEN(PHRASE, cw) {fprintf(stderr, "Fatal Error: %s occured for input token number %d of your program.\n", PHRASE, cw); exit(2);}

struct prog{
	char wds[MAXNUMTOKENS][MAXTOKENSIZE];
	int cw; /* Current word */
   int bracketflag; /* keeps track of close brackets so that the program only finished on a '}' if all do loops have been closed */
};
typedef struct prog Program;

void Initprog(Program *p); 
void Prog(Program *p);
void Code(Program *p);
int Statement(Program *p);
void Checkfile(int argc, FILE *fp);
int numorcaps(char str[]);
int caps(char str[]);
int checkpolish(Program *p);
int op(char str[]);
void subseqfd(Program *p);
void subseqturn(Program *p);
void subseqdo(Program *p);
void subseqset(Program *p);

int main(int argc, char **argv){
	int i;
   FILE *fp;
	Program prog;
   Initprog(&prog);
	if(!(fp = fopen(argv[1], "r"))){
		fprintf(stderr, "Cannot open file\n");
		exit(2);
	}
   Checkfile(argc, fp);
	i=0;
	while(fscanf(fp, "%s", prog.wds[i++])==1 && i<MAXNUMTOKENS);
	assert(i<MAXNUMTOKENS);
   Prog(&prog);
   printf("Parsed OK\n");
   fclose(fp);
   return(0);
}

/* The following function checks the first token to see if it is an opening brace. It then calls function
'code' which is a recursive loop for checking successive tokens */
void Prog(Program *p){
   if(strsame(p->wds[p->cw], "{")==0){
      ERROR("No opening '{'?");
   }
   p->cw = p->cw+1;
   Code(p);
}

/* Code checks to see if a token is the close brace character/string. If it is not then it checks what that
token is via statement. If it is, then it returns and so ends itself. */
void Code(Program *p){
   if(strsame(p->wds[p->cw], "}")){
      if(p->bracketflag==0){
         return;
      }
      else{
         p->bracketflag=p->bracketflag-1;
      }
   }
   Statement(p);
   p->cw = p->cw+1;
  /* checkstrlength(p->wds[p->cw], p->cw); */
   Code(p);
}

/* Checks a token to see if it is valid. If it is valid it returns the number of strings that have been
gone through in the process. Otherwise it produces an error message. */
int Statement(Program *p){
   if(strsame(p->wds[p->cw], "FD")){
      subseqfd(p);
      return(0);
   }
   if(strsame(p->wds[p->cw], "LT")){
      subseqturn(p);
      return(0);
   }
   if(strsame(p->wds[p->cw], "RT")){
      subseqturn(p);
      return(0);
   }
   if(strsame(p->wds[p->cw], "DO")){
      subseqdo(p);
      return(0);
   }
   if(strsame(p->wds[p->cw], "SET")){
      subseqset(p);
      checkpolish(p);
      return(0);
   }
   if(strsame(p->wds[p->cw], "}")){
      return(0);
   }
   ERROR("Expecting an FD, LT, RT, DO or SET instruction");
}

void subseqfd(Program *p){
   p->cw++;
   if(!numorcaps(p->wds[p->cw])){
      ERRORNEXT("a letter or number variable", "'FD'", p->cw);
   }
}

void subseqturn(Program *p){
   p->cw++;
   if(!numorcaps(p->wds[p->cw])){
      ERRORNEXT("a letter or number variable", "turn instruction", p->cw);
   }
}

/*package together all things which must follow the instruction do - including error messages*/
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
      ERRORNEXT("a letter or number variable", "'FROM'", p->cw);
   }
   p->cw++;
   if(!strsame(p->wds[p->cw], "TO")){
      ERRORNEXT("'TO'", "the preceding variable", p->cw);
   }
   p->cw++;
   if(!numorcaps(p->wds[p->cw])){
      ERRORNEXT("a letter or number variable", "'TO'", p->cw);
   }
   p->cw++;
   if(!strsame(p->wds[p->cw], "{")){
      ERRORNEXT("'{'", "the preceding variable", p->cw);
   }
   p->bracketflag++;
}

/*package together all things which must follow the instruction set - including error messages*/
void subseqset(Program *p){
   p->cw++;
   if(!caps(p->wds[p->cw])){
      ERRORNEXT("a capital letter variable", "'SET'", p->cw);
   }
   p->cw++;
   if(!strsame(p->wds[p->cw], ":=")){
      ERRORNEXT("':='", "the preceding variable", p->cw);
   }
   p->cw++;
}

/*following function returns 0 if the polish is not of the right format. otherwise it returns the number of tokens used after ':=' 
in the instruction 'set'*/
int checkpolish(Program *p){
   if(strsame(p->wds[p->cw], ";")){
      return(0); 
   }
   if(numorcaps(p->wds[p->cw]) || op(p->wds[p->cw])){
      p->cw=p->cw+1;
      checkpolish(p);
   }
   if(!numorcaps(p->wds[p->cw]) && !op(p->wds[p->cw]) && !strsame(p->wds[p->cw], ";")){
      ERRORTOKEN("Expecting a Polish expression (operation, variable or ';')", p->cw);
   }
   return(0);
}

/*function which returns 1 if a string is just an operation and 0 if it is not*/
int op(char str[]){
   if((strsame(&str[0], "+") || strsame(&str[0], "-") || strsame(&str[0], "*") || strsame(&str[0], "/")) && strlen(str)==1){
      return(1);
   }
   return(0);
}

/* we want the following to return a 1 if a string is a valid number or capital letter and a 0 if not */
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

/*used to be intorcaps and iscaps used to be a function*/
int caps(char str[]){ 
   if(str[0]>='A'&&str[0]<='Z'&&str[1]=='\0'){
      return(1);
   }
   return(0);
}

/*initialising a 'program'*/
void Initprog(Program *p){
   int i;
   p->cw = 0;
   p->bracketflag=0;
   for(i=0; i<MAXNUMTOKENS; i++){
      p->wds[i][0]='\0';         
   }
}

void Checkfile(int argc, FILE *fp){
   if(argc!=NUM_ARGS){ 
      fprintf(stderr, "ERROR: Wrong number of file inputs\n");
      /*Error printed & program exitted if there aren't 2 inputs. */
      exit(1);
   }
      if(fp==NULL){
      fprintf(stderr, "ERROR: input file is empty\n");
      /*Error printed & program exitted if there is no input file or it cannot be read. */
      exit(1);
   }
}







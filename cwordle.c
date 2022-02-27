/*
Wordle for Terminal. 
"dict.txt"
- Generated from Donald Knuth at Stanford University.
(https://homepage.cs.uiowa.edu/~sriram/21/fall04/words.html)

@version: 0.1
@author: v3l0r3k
- Last modified: 27/2/2022
*/

#include <stdio.h>
#include <termios.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <locale.h>
#include <time.h>
#include <wchar.h>

#include "config.h"

#define dummyWord "ERASE\0"
#define MAX_TEXTBOX 6
//words in the dictionary file should have 5 chars and a 0x0A at the end
#define SEPARATOR 0x0A
#define DICTIONARY DATA_DIR "/dict.txt"
#define POSSIBLES DATA_DIR "/possible.txt"

//UNICODE chars
#define HOR_LINE 9472
#define VER_LINE 9474
#define UPPER_LEFT_CORNER 9484
#define LOWER_LEFT_CORNER 9492
#define UPPER_RIGHT_CORNER 9488
#define LOWER_RIGHT_CORNER 9496

//KEYS
#define K_BACKSPACE 127
#define K_ENTER 13
#define K_ESCAPE 27
#define K_ENTER2 10

// Background colors low intensity
#define B_BLACK 40
#define B_RED 41
#define B_GREEN 42
#define B_YELLOW 43
#define B_BLUE 44
#define B_MAGENTA 45
#define B_CYAN 46
#define B_WHITE 47

// Foreground colors low intensity
#define F_BLACK 30
#define F_RED 31
#define F_GREEN 32
#define F_YELLOW 33
#define F_BLUE 34
#define F_MAGENTA 35
#define F_CYAN 36
#define F_WHITE 37
#define F_GREY 90

#define BOARDSIZEY 21
#define BOARDSIZEX 35
//GLOBALS
//Game
char boardInputs[7][6];
char secretWord[6];
char textbox1[MAX_TEXTBOX];
int  repeatedLetters[5]={1,1,1,1,1};
char textbox1[MAX_TEXTBOX];
int  checkTrue[5]={0,0,0,0,0};
struct winsize max;
static struct termios term1,term2, failsafe;
static int peek_character = -1;
int rows=0,columns=0;
int wherey=0, wherex=0;
int oldy=0, oldx=0;
int currentIndex = 0;
int     okFile;
int dictionaryPresent = 0;
FILE   *fileSource;
FILE   *fileSource2;
time_t t;
unsigned randomWord=0;
unsigned words=0;
unsigned words2=0;
//PROTOTYPES
//Terminal
int kbhit();
int read_keytrail(char chartrail[5]);
int readch();
void resetch();
void gotoxy(int x, int y);
void outputcolor(int foreground, int background);
int get_terminal_dimensions(int *rows, int *columns);
int get_pos(int *y, int *x);
void hidecursor();
void showcursor();
void resetAnsi(int x);
void pushTerm();
int resetTerm();
void cls();

//USER INTERFACE
void write_str(int wherex, int wherey, char *str, int backcolor, int forecolor);
char textbox(int wherex, int wherey, int displayLength,char label[MAX_TEXTBOX], char text[MAX_TEXTBOX], int backcolor,
	    int labelcolor, int textcolor);
void write_ch(int wherex, int wherey, wchar_t  ch, int backcolor, int forecolor);
int write_num(int x, int y, int num, char backcolor, char forecolor);
void window(int x1, int y1, int x2, int y2, int backcolor,
         int bordercolor, int titlecolor, int border, int title);

void toUpper(char *text);
//Game
void newGame();
void gameLoop();
void displayHelp();
void drawBoard();
void credits();
int checkGreen(char c, int index, char *str);
int checkOrange(char c, int index, char *str);
void checkRepeatedLetters();
void writeWord(int index,  char text[MAX_TEXTBOX]);
void cleanArea();
int findIndex(char c);
//FILE
int openFile(FILE ** fileHandler, const char *fileName, const char *mode);
long countWords(FILE * fileHandler);
void getWordfromDictionary(FILE * fileHandler, char WORD[MAX_TEXTBOX]);
int isWordinDictionary(FILE * fileHandler, char WORD[MAX_TEXTBOX]);
int closeFile(FILE * fileHandler);
//Terminal Routines
//----------------

/*-------------------------------------*/
/* Initialize new terminal i/o settings*/
/*-------------------------------------*/
void pushTerm() {
//Save terminal settings in failsafe to be retrived at the end
  tcgetattr(0, &failsafe);
}

/*---------------------------*/
/* Reset terminal to failsafe*/
/*---------------------------*/
int resetTerm() {
  //tcsetattr(0, TCSANOW, &failsafe);
  /* flush and reset */
  if (tcsetattr(0,TCSAFLUSH,&failsafe) < 0) return -1;
  return 0;
}


/*------------------------*/
/* Get terminal dimensions*/
/*------------------------*/
int get_terminal_dimensions(int *rows, int *columns) {
  ioctl(0, TIOCGWINSZ, &max);
  *columns = max.ws_col;
  *rows = max.ws_row;
  return 0;
}

/*--------------------------*/
/* Ansi function hide cursor*/
/*--------------------------*/
void hidecursor() {
  printf("\e[?25l");
}

/*--------------------------*/
/* Ansi function show cursor*/
/*--------------------------*/
void showcursor() {
  printf("\e[?25h");
}

void cls() {
  system("clear");
}


int get_pos(int *y, int *x) {

 char buf[30]={0};
 int ret, i, pow;
 char ch;

*y = 0; *x = 0;

 struct termios term, restore;

 tcgetattr(0, &term);
 tcgetattr(0, &restore);
 term.c_lflag &= ~(ICANON|ECHO);
 tcsetattr(0, TCSANOW, &term);

 write(1, "\033[6n", 4);

 for( i = 0, ch = 0; ch != 'R'; i++ )
 {
    ret = read(0, &ch, 1);
    if ( !ret ) {
       tcsetattr(0, TCSANOW, &restore);
       //fprintf(stderr, "getpos: error reading response!\n");
       return 1;
    }
    buf[i] = ch;
    //printf("buf[%d]: \t%c \t%d\n", i, ch, ch);
 }

 if (i < 2) {
    tcsetattr(0, TCSANOW, &restore);
    //printf("i < 2\n");
    return(1);
 }

 for( i -= 2, pow = 1; buf[i] != ';'; i--, pow *= 10)
     *x = *x + ( buf[i] - '0' ) * pow;

 for( i-- , pow = 1; buf[i] != '['; i--, pow *= 10)
     *y = *y + ( buf[i] - '0' ) * pow;

 tcsetattr(0, TCSANOW, &restore);
 return 0;
}
/*--------------------------------------.*/
/* Detect whether a key has been pressed.*/
/*---------------------------------------*/
int kbhit(void)
{
    if(peek_character != -1)
    return 1;

    tcgetattr(0, &term1);
    term2.c_lflag &= ~ICANON;
    tcsetattr(0, TCSANOW, &term2);

    int byteswaiting;
    ioctl(0, FIONREAD, &byteswaiting);

    tcsetattr(0, TCSANOW, &term1);

    return byteswaiting > 0;
}

/*----------------------*/
/*Read char with control*/
/*----------------------*/

int readch() {
  char    ch;
  if(peek_character != -1) {
    ch = peek_character;
    peek_character = -1;
    return ch;
  }
  read(0, &ch, 1);
  return ch;
}

void resetch() {
//Clear keyboard buffer
  term1.c_cc[VMIN] = 0;
  tcsetattr(0, TCSANOW, &term1);
  peek_character = 0;
}

/*----------------------------------*/
/* Move cursor to specified position*/
/*----------------------------------*/
void gotoxy(int x, int y) {
  printf("%c[%d;%df", 0x1B, y, x);
}

/*---------------------*/
/* Change colour output*/
/*---------------------*/
void outputcolor(int foreground, int background) {
  printf("%c[%d;%dm", 0x1b, foreground, background);
}

/*-----------------------*/
void resetAnsi(int x) {
  switch (x) {
    case 0:			//reset all colors and attributes
      printf("%c[0m", 0x1b);
      break;
    case 1:			//reset only attributes
      printf("%c[20m", 0x1b);
      break;
    case 2:			//reset foreg. colors and not attrib.
      printf("%c[39m", 0x1b);
      break;
    case 3:			//reset back. colors and not attrib.
      printf("%c[49m", 0x1b);
      break;
    default:
      break;
  }
}



//User Interface Routines
//----------------
/*----------------------------*/
/* User Interface -           */
/*----------------------------*/
void write_str(int wherex, int wherey, char *str, int backcolor, int forecolor){
  gotoxy(wherex,wherey);
  outputcolor(backcolor,forecolor);
  printf("%s", str);
}

void write_ch(int wherex, int wherey, wchar_t ch, int backcolor, int forecolor){
//Write unicode character
  gotoxy(wherex,wherey);
  outputcolor(backcolor,forecolor);
  printf("%lc", ch);
}

int write_num(int x, int y, int num, char backcolor, char forecolor) {
  //the length of the string must be passed on the function
  char   astr[30];
  char len=0;

  sprintf(astr, "%d", num);
  write_str(x, y, astr, backcolor, forecolor);
  len = strlen(astr);
  return len;
}


char textbox(int wherex, int wherey, int displayLength,
	    char label[MAX_TEXTBOX], char text[MAX_TEXTBOX], int backcolor,
	    int labelcolor, int textcolor) {
  int     charCount = 0;
  int     exitFlag = 0;
  int     cursorON = 1;
  long    cursorCount = 0;
  int     i;
  int     limitCursor = 0;
  int     positionx = 0;
  int     posCursor = 0;
  int     keypressed = 0;
  char    displayChar;
  char    ch;
  strcpy(text,"");
  text[0]='\0';
  positionx = wherex + strlen(label);
  limitCursor = wherex+strlen(label)+displayLength+1;
  write_str(wherex, wherey, label, backcolor, labelcolor);

  write_ch(positionx, wherey, '[', backcolor, textcolor);
  for(i = positionx + 1; i <= positionx + displayLength; i++) {
    write_ch(i, wherey, '.', backcolor, textcolor);
  }
  write_ch(positionx + displayLength + 1, wherey, ']', backcolor,
	   textcolor);
  //Reset keyboard
  if(kbhit() == 1) ch = readch();
  ch = 0;

  do {
      keypressed = kbhit();
    //Cursor Animation
   if (keypressed == 0){

   cursorCount++;
    if(cursorCount == 100) {
      cursorCount = 0;
      switch (cursorON) {
	case 1:
	  posCursor = positionx + 1;
          displayChar = '.';
          if (posCursor == limitCursor) {
            posCursor = posCursor - 1;
            displayChar = ch;
          }
          write_ch(posCursor, wherey, displayChar, backcolor, textcolor);
          cursorON = 0;
	  break;
	case 0:
          posCursor = positionx + 1;
          if (posCursor == limitCursor) posCursor = posCursor - 1;
	  write_ch(posCursor, wherey, '|', backcolor, textcolor);
          cursorON = 1;
	  break;
      }
     }
    }
    //Process keys
    if(keypressed == 1) {
      ch = readch();
      keypressed = 0;

      if(charCount < displayLength) {
	if(ch > 31 && ch < 127) {
	  write_ch(positionx + 1, wherey, ch, backcolor, textcolor);
	  text[charCount] = ch;
	  positionx++;
	  charCount++;
	}
      }
    }

    if (ch==K_BACKSPACE){
      if (positionx>0 && charCount>0){
       positionx--;
       charCount--;
       write_ch(positionx + 1, wherey, '.', backcolor, textcolor);
       if (positionx < limitCursor-2) write_ch(positionx + 2, wherey, '.', backcolor, textcolor);
       resetch();
      }
    }
    if(ch == K_ENTER || ch == K_ESCAPE)
      exitFlag = 1;

    //ENTER OR ESC TO FINISH LOOP
  } while(exitFlag != 1);
  //clear cursor
  write_ch(posCursor, wherey, ' ', backcolor, textcolor);
  resetch();
  return ch;
}

void window(int x1, int y1, int x2, int y2, int backcolor,
         int bordercolor, int titlecolor, int border, int title) {
  int     i, j;
  i = x1;
  j = y1;
  //borders
  if(border == 1) {
    for(i = x1; i <= x2; i++) {
      //upper and lower borders
      write_ch(i, y1, HOR_LINE, backcolor, bordercolor);   //horizontal line box-like char
      write_ch(i, y2, HOR_LINE, backcolor, bordercolor);
    }
    for(j = y1; j <= y2; j++) {
      //left and right borders
      write_ch(x1, j, VER_LINE, backcolor, bordercolor);   //vertical line box-like char
      write_ch(x2, j, VER_LINE, backcolor, bordercolor);
    }
    write_ch(x1, y1, UPPER_LEFT_CORNER, backcolor, bordercolor);   //upper-left corner box-like char
    write_ch(x1, y2, LOWER_LEFT_CORNER, backcolor, bordercolor);   //lower-left corner box-like char
    write_ch(x2, y1, UPPER_RIGHT_CORNER, backcolor, bordercolor);  //upper-right corner box-like char
    write_ch(x2, y2, LOWER_RIGHT_CORNER, backcolor, bordercolor);  //lower-right corner box-like char
  }
  if (title == 1) {
    for(i = x1; i <= x2; i++)
      write_ch(i, y1-1, ' ', titlecolor, titlecolor);
  }
}

void toUpper(char *text){
//CHANGE LETTERS TO UPPERCASE
size_t i=0;
 for (i=0; i<strlen(text); i++){
    if (text[i]>= 97 && text[i] <= 122) text[i] = text[i] - 32;
  }
}

//GAME FUNCTIONS


void drawBoard(){
 int i=0,  j=0, shiftx=0, shifty =0;
 int sizeX = 4;
 int sizeY = 2;
  cleanArea();
  shiftx = wherex+4;
  shifty = wherey;
  write_str(wherex,wherey,"[C WORDLE FOR TERMINAL by        ]", B_BLACK, F_WHITE);
  write_str(wherex+26,wherey,"v3l0r3k", B_BLACK, F_GREY);
  write_ch(wherex+4, wherey+2, 'a', B_BLACK, F_WHITE);
 
  for (j=0; j<6; j++){
  for (i = 0; i<5; i++){
      window(shiftx,shifty+1, (shiftx+sizeX)  , shifty + 1  + sizeY, B_BLACK, F_WHITE, F_WHITE ,1,0);
      shiftx = shiftx + sizeX+1;
   }
      shifty = shifty + sizeY+1;
      shiftx = wherex+4;
  }
 wherey = shifty;

 write_str(wherex,wherey+1,"[ESC: EXIT | TYPE <help> for more]", B_BLACK, F_GREY);
}

void cleanArea()
{
   int i=0,j=0;
   for (j=0; j<BOARDSIZEY; j++){
     for (i=0; i<50; i++){
       write_ch(oldx+i,oldy+j,' ', B_BLACK, F_BLACK);
     }
   }
}

void displayHelp(){
  char ch=0; 
   cleanArea();
   write_str(1,oldy,"C-WORDLE", B_WHITE, F_BLACK);
   write_str(1,oldy+1,"Guess a 5-letter secret word in 6 tries.", B_BLACK, F_WHITE);
   write_str(1,oldy+2,"Type any word to start.", B_BLACK, F_WHITE);
   write_str(1,oldy+4,"[C]", B_GREEN, F_WHITE);
   write_str(4,oldy+4,"-> LETTER IS IN THE RIGHT POSITION", B_BLACK, F_WHITE);
   write_str(1,oldy+5,"[C]", B_YELLOW, F_WHITE);
   write_str(4,oldy+5,"-> LETTER IS IN THE WRONG POSITION", B_BLACK, F_WHITE);
   write_str(1,oldy+6,"[C]", B_BLACK, F_WHITE);
   write_str(4,oldy+6,"-> LETTER IS NOT IN THE WORD", B_BLACK, F_WHITE);
   write_str(1,oldy+8,"Type <exit> or <quit> to exit.", B_BLACK, F_WHITE);
   write_str(1,oldy+9,"Type <cheat> to give up.", B_BLACK, F_WHITE);
   write_str(1,oldy+10,"Total words: ", B_BLACK, F_GREY);
   write_num(16,oldy+10,words, B_BLACK, F_GREY);
   write_str(1,oldy+11,"Possible words: ", B_BLACK, F_GREY);
   write_num(18,oldy+11,words2, B_BLACK, F_GREY);
   write_str(1,oldy+13, "Press <ENTER> or <ESC> key to return.", B_BLACK, F_WHITE);
   wherex=oldx;
   wherey=oldy;
   do{
      gotoxy(1,1);
      printf("\n");
      if (kbhit()) ch = readch();
      if (ch == K_ESCAPE) break;
   } while (ch != K_ENTER);
   cleanArea();
}

int findIndex(char c)
//returns the index of a char in an array of chars
{
  int i=0; char ch=0;
  do{
     ch=secretWord[i];
     if (c==ch) break;
     i++;
  } while(i<5);
 return i;
}

int checkGreen(char c, int index, char *str){
char ch=0;
size_t i=0;
size_t lindex=index;
int col=B_BLACK; int letterIndex=0;
  //color letters accordingly
  letterIndex= findIndex(c);
   for (i=0; i<strlen(str); i++){
     ch = str[i];
     if (c == ch && lindex==i) {
        col = B_GREEN;
        checkTrue[i] = 1;
        repeatedLetters[letterIndex]--;
     }
   }
 return col;
}

int checkOrange(char c, int index, char *str){
char ch=0;
size_t i=0;
int col=B_BLACK, letterIndex=0;

  letterIndex= findIndex(c);
  //color letters accordingly
   for (i=0; i<strlen(str); i++){
     ch = str[i];
     if (c == ch &&  repeatedLetters[letterIndex] >0 && checkTrue[index]==0) {
	     col= B_YELLOW;
	     repeatedLetters[letterIndex]--;
	     break;}
   }
  if (checkTrue[index] == 1) col = B_GREEN;
 return col;
}


void writeWord(int index, char text[MAX_TEXTBOX]){
    int j=0,i=0, x=0, y=0, col=B_BLACK;
    x = oldx + 6;
    y = oldy + 2;

    checkRepeatedLetters();
    for(j=0; j<index;j++)
     y = y + 3;
    //clean array of true values
    for (i=0; i<5; i++) checkTrue[i] = 0;
    //Search for Green letters
    for (i=0; i<MAX_TEXTBOX; i++)
       {
           col=checkGreen(text[i], i, secretWord);
           write_ch(x, y, text[i], col, F_WHITE);
           x = x + 5;
       }
    //Search for Orange and Black letters
    x = oldx + 6;
    for (i=0; i<5; i++)
       {
           col=checkOrange(text[i], i, secretWord);
           write_ch(x, y, text[i], col, F_WHITE);
           x = x + 5;
       }
}

void checkRepeatedLetters(){
char ch=0;
size_t i,j;
for (i=0; i<5; i++) repeatedLetters[i] = 1;
   for (i=0; i<strlen(secretWord); i++){
      ch = secretWord[i];
      for (j=i+1; j<strlen(secretWord); j++){
        if (ch==secretWord[j]) {
             repeatedLetters[i] = repeatedLetters[i] + 1;
             repeatedLetters[j] = repeatedLetters[i];
        }
      }
   }
}
void gameLoop(){
char ch=0;
int cheat=0;

  do{
    cheat=0;
    memset(&textbox1,'\0',sizeof(textbox1));
     ch = textbox(1,wherey+2,5,"[+] Word:",textbox1,F_WHITE,F_WHITE,F_WHITE);
     if (ch == K_ESCAPE) break;
     if (strcmp(textbox1,"exit") ==0 || strcmp(textbox1,"quit") == 0)
      break;
     if (strcmp(textbox1,"cheat") == 0){
       write_str(17,oldy+20,"                                  ", B_BLACK, F_GREEN);
       write_str(18,oldy+20,secretWord, B_BLACK, F_GREEN);
       memset(&textbox1,'\0',sizeof(textbox1));
       cheat=1;
      }
     if (strcmp(textbox1,"help") == 0)
       break;
      if (strlen(textbox1) == 5){
         checkRepeatedLetters();
         toUpper(textbox1);
 
         if (isWordinDictionary(fileSource,textbox1) == 1) {
             writeWord(currentIndex, textbox1);
             strcpy(boardInputs[currentIndex], textbox1);
             write_str(wherex+16,wherey+2,"->VALID WORD!                   ", B_BLACK, F_GREEN);
             if (currentIndex<6) currentIndex++;
             if (strcmp(textbox1,secretWord) == 0){
               write_str(wherex,wherey+1,"->SUCCESS!                        ", B_BLACK, F_BLUE);
               break;
              } else{
             if (currentIndex == 6) {
              write_str(wherex,wherey+1,"->GAME OVER:                     ", B_BLACK, F_MAGENTA);
              write_str(wherex+14,wherey+1,secretWord, B_BLACK, F_GREEN);
             break;
            }
          }
         } else {   
            write_str(wherex+16,wherey+2,"->WORD NOT FOUND!                ", B_BLACK, F_RED);
         }
    }
     else{
       if (cheat==0) write_str(wherex+16,wherey+2,"->TOO SHORT!                      ", B_BLACK, F_RED);
    }
  } while (ch!= K_ESCAPE);
   write_str(1,wherey+2,"                                 ", B_BLACK,F_WHITE);
   if (strcmp(textbox1,"help") == 0){
     displayHelp();
     newGame();
  }
}

void newGame(){
int i=0;
 drawBoard();
//Rewrite previous words on panel
   if (currentIndex >0){
     for (i=0; i<=currentIndex; i++)
       writeWord(i, boardInputs[i]);
   }
 gameLoop();
}

int openFile(FILE ** fileHandler, const char *fileName, const char *mode) {
  int     ok;
  *fileHandler = fopen(fileName, mode);
  //check whether buffer is assigned
  //and return value
  if(*fileHandler != NULL)
    ok = 1;
  else
    ok = 0;
  return ok;
}

long countWords(FILE * fileHandler) {
  long    wordCount = 0;
  char    ch;

  //Read char by char
  if(fileHandler != NULL) {
    rewind(fileHandler);	//Go to start of file
    ch = getc(fileHandler);	//peek into file
    while(!feof(fileHandler)) {
      //Read until SEPARATOR 0x0A
      if(ch == SEPARATOR) {
	wordCount++;
      }
      ch = getc(fileHandler);
    }
   }
  return wordCount;
}

void getWordfromDictionary(FILE * fileHandler, char WORD[MAX_TEXTBOX]) {
  long    i = 0;
  char    ch;
  char    dataString[MAX_TEXTBOX];

  //Read char by char
  if(fileHandler != NULL) {
    rewind(fileHandler);	
    //Go to where the word starts 6bytes * randomWord
    fseek(fileHandler,MAX_TEXTBOX*randomWord, SEEK_SET); 
    ch = getc(fileHandler);	//peek into file
    while(i<5) {
      //Read until SEPARATOR 0x0A
      if (ch!= SEPARATOR) dataString[i++] = ch;
      //i++;
      ch = getc(fileHandler);
    }
  }
  dataString[i] = '\0';	// null-end string
  i = 0;
  strcpy(WORD, dataString);
}

int isWordinDictionary(FILE * fileHandler, char WORD[MAX_TEXTBOX]) {
  long    i = 0;
  int     isFound = 0;
  char    ch;
  char    dataString[MAX_TEXTBOX];

  //Read char by char
  if(fileHandler != NULL) {
    rewind(fileHandler);	//Go to start of file
    ch = getc(fileHandler);	//peek into file
    while(!feof(fileHandler)) {
      //Read until SEPARATOR 0x0A
      if (ch != SEPARATOR) dataString[i++] = ch;
      //i++;
      if(ch == SEPARATOR) {
	dataString[i] = '\0';	// null-end string
        if (strcmp(dataString, WORD) == 0) {isFound = 1; break;}
	i = 0;
      }
      ch = getc(fileHandler);
    }
  }
  return isFound;
}

int closeFile(FILE * fileHandler) {
  int     ok;
  ok = fclose(fileHandler);
  return ok;
}

void credits(){
  resetTerm();
  gotoxy(wherex,wherey+2);
  printf("\r");
  printf("C-Wordle. Coded by v3l0r3k 2022                   \n");
  showcursor();
  resetAnsi(0);
}

int main(){
   srand((unsigned) time(&t));
   //INIT TERMINAL
   get_pos(&wherey,&wherex);
   oldx = wherex;
   oldy = wherey;
   get_terminal_dimensions(&rows, &columns);
   if (rows < BOARDSIZEY || columns < BOARDSIZEX)
    {
      printf("Error: Terminal size is too small. Resize terminal. \n");
      exit(0);
   }
   if (rows-wherey < BOARDSIZEY) {
      cls();
      get_pos(&wherey,&wherex);
      oldx = wherex;
     oldy = wherey;
   }
   //SEARCH FOR DICTIONARY
   okFile = openFile(&fileSource, DICTIONARY, "r");
   okFile = openFile(&fileSource2, POSSIBLES, "r");
   if (okFile == 0) {
     //No dictionary
     dictionaryPresent = 0;
     words = 1;
     strcpy(secretWord, dummyWord);
     printf("ERROR: Dictionary not found. Create file <dict.txt>\n");
     exit(0);
   } else {
     //Dictionary is present
     dictionaryPresent = 1;
     words = countWords(fileSource);
     words2 = countWords(fileSource2);
     //Selecting a random word from dictionary
     randomWord = rand() % words2;
     getWordfromDictionary(fileSource2, secretWord);
     pushTerm();
     hidecursor();
     resetch();
     //Set Locale For Unicode characters to work
     setlocale(LC_ALL, "");
     //GAME
     newGame();
     if (fileSource != NULL) closeFile(fileSource);
     if (fileSource2 != NULL) closeFile(fileSource2);
     credits();
   } 
  return 0;

}

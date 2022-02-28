/*
Wordle for Terminal.
"dict.txt"
- Generated from Donald Knuth at Stanford University.
(https://homepage.cs.uiowa.edu/~sriram/21/fall04/words.html)

@version: 0.1
@author: v3l0r3k
- Last modified: 27/2/2022
*/

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>

#define dummyWord "ERASE\0"
#define MAX_TEXTBOX 6
// words in the dictionary file should have 5 chars and a 0x0A at the end
#define SEPARATOR 0x0A
#define DICTIONARY "dict.txt"
#define POSSIBLES "possible.txt"

// UNICODE chars
#define HOR_LINE 9472
#define VER_LINE 9474
#define UPPER_LEFT_CORNER 9484
#define LOWER_LEFT_CORNER 9492
#define UPPER_RIGHT_CORNER 9488
#define LOWER_RIGHT_CORNER 9496

// KEYS
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
// GLOBALS
// Game
char                  board_inputs[7][6];
char                  secret_word[6];
char                  textbox1[MAX_TEXTBOX];
int                   repeated_letters[5] = {1, 1, 1, 1, 1};
char                  textbox1[MAX_TEXTBOX];
int                   check_true[5] = {0, 0, 0, 0, 0};
struct winsize        max;
static struct termios term1, term2, failsafe;
static int            peek_character = -1;
int                   rows = 0, columns = 0;
int                   wherey = 0, wherex = 0;
int                   oldy = 0, oldx = 0;
int                   current_index = 0;
int                   ok_file;
int                   dictionary_present = 0;
FILE                 *file_source;
FILE                 *file_source2;
time_t                t;
unsigned              random_word = 0;
unsigned              words      = 0;
unsigned              words2     = 0;
// PROTOTYPES
// Terminal
int  kbhit();
int  read_keytrail(char chartrail[5]);
int  readch();
void resetch();
void gotoxy(int x, int y);
void outputcolor(int foreground, int background);
int  get_terminal_dimensions(int *rows, int *columns);
int  get_pos(int *y, int *x);
void hidecursor();
void showcursor();
void reset_ansi(int x);
void push_term();
int  reset_term();
void cls();

// USER INTERFACE
void write_str(int wherex, int wherey, char *str, int backcolor, int forecolor);
char textbox(int wherex, int wherey, int displayLength, char label[MAX_TEXTBOX], char text[MAX_TEXTBOX], int backcolor,
             int labelcolor, int textcolor);
void write_ch(int wherex, int wherey, wchar_t ch, int backcolor, int forecolor);
int  write_num(int x, int y, int num, char backcolor, char forecolor);
void window(int x1, int y1, int x2, int y2, int backcolor, int bordercolor, int titlecolor, int border, int title);

void to_upper(char *text);
// Game
void new_game();
void game_loop();
void display_help();
void draw_board();
void credits();
int  check_green(char c, int index, char *str);
int  check_orange(char c, int index, char *str);
void check_repeated_letters();
void write_word(int index, char text[MAX_TEXTBOX]);
void clean_area();
int  find_index(char c);
// FILE
int  open_file(FILE **fileHandler, char *fileName, char *mode);
long count_words(FILE *fileHandler);
void get_wordfrom_dictionary(FILE *fileHandler, char WORD[MAX_TEXTBOX]);
int  is_wordin_dictionary(FILE *fileHandler, char WORD[MAX_TEXTBOX]);
int  close_file(FILE *fileHandler);
// Terminal Routines
//----------------

/*-------------------------------------*/
/* Initialize new terminal i/o settings*/
/*-------------------------------------*/
void push_term() {
  // Save terminal settings in failsafe to be retrived at the end
  tcgetattr(0, &failsafe);
}

/*---------------------------*/
/* Reset terminal to failsafe*/
/*---------------------------*/
int reset_term() {
  // tcsetattr(0, TCSANOW, &failsafe);
  /* flush and reset */
  if (tcsetattr(0, TCSAFLUSH, &failsafe) < 0)
    return -1;
  return 0;
}

/*------------------------*/
/* Get terminal dimensions*/
/*------------------------*/
int get_terminal_dimensions(int *rows, int *columns) {
  ioctl(0, TIOCGWINSZ, &max);
  *columns = max.ws_col;
  *rows    = max.ws_row;
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

  char buf[30] = {0};
  int  ret, i, pow;
  char ch;

  *y = 0;
  *x = 0;

  struct termios term, restore;

  tcgetattr(0, &term);
  tcgetattr(0, &restore);
  term.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(0, TCSANOW, &term);

  write(1, "\033[6n", 4);

  for (i = 0, ch = 0; ch != 'R'; i++) {
    ret = read(0, &ch, 1);
    if (!ret) {
      tcsetattr(0, TCSANOW, &restore);
      // fprintf(stderr, "getpos: error reading response!\n");
      return 1;
    }
    buf[i] = ch;
    // printf("buf[%d]: \t%c \t%d\n", i, ch, ch);
  }

  if (i < 2) {
    tcsetattr(0, TCSANOW, &restore);
    // printf("i < 2\n");
    return (1);
  }

  for (i -= 2, pow = 1; buf[i] != ';'; i--, pow *= 10)
    *x = *x + (buf[i] - '0') * pow;

  for (i--, pow = 1; buf[i] != '['; i--, pow *= 10)
    *y = *y + (buf[i] - '0') * pow;

  tcsetattr(0, TCSANOW, &restore);
  return 0;
}
/*--------------------------------------.*/
/* Detect whether a key has been pressed.*/
/*---------------------------------------*/
int kbhit(void) {
  if (peek_character != -1)
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
  char ch;
  if (peek_character != -1) {
    ch             = peek_character;
    peek_character = -1;
    return ch;
  }
  read(0, &ch, 1);
  return ch;
}

void resetch() {
  // Clear keyboard buffer
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
void reset_ansi(int x) {
  switch (x) {
  case 0: // reset all colors and attributes
    printf("%c[0m", 0x1b);
    break;
  case 1: // reset only attributes
    printf("%c[20m", 0x1b);
    break;
  case 2: // reset foreg. colors and not attrib.
    printf("%c[39m", 0x1b);
    break;
  case 3: // reset back. colors and not attrib.
    printf("%c[49m", 0x1b);
    break;
  default:
    break;
  }
}

// User Interface Routines
//----------------
/*----------------------------*/
/* User Interface -           */
/*----------------------------*/
void write_str(int wherex, int wherey, char *str, int backcolor, int forecolor) {
  gotoxy(wherex, wherey);
  outputcolor(backcolor, forecolor);
  printf("%s", str);
}

void write_ch(int wherex, int wherey, wchar_t ch, int backcolor, int forecolor) {
  // Write unicode character
  gotoxy(wherex, wherey);
  outputcolor(backcolor, forecolor);
  printf("%lc", ch);
}

int write_num(int x, int y, int num, char backcolor, char forecolor) {
  // the length of the string must be passed on the function
  char astr[30];
  char len = 0;

  sprintf(astr, "%d", num);
  write_str(x, y, astr, backcolor, forecolor);
  len = strlen(astr);
  return len;
}

char textbox(int wherex, int wherey, int displayLength, char label[MAX_TEXTBOX], char text[MAX_TEXTBOX], int backcolor,
             int labelcolor, int textcolor) {
  int  char_count   = 0;
  int  exit_flag    = 0;
  int  cursor_on    = 1;
  long cursor_count = 0;
  int  i;
  int  limit_cursor = 0;
  int  positionx   = 0;
  int  pos_cursor   = 0;
  int  keypressed  = 0;
  char display_char;
  char ch;
  strcpy(text, "");
  text[0]     = '\0';
  positionx   = wherex + strlen(label);
  limit_cursor = wherex + strlen(label) + displayLength + 1;
  write_str(wherex, wherey, label, backcolor, labelcolor);

  write_ch(positionx, wherey, '[', backcolor, textcolor);
  for (i = positionx + 1; i <= positionx + displayLength; i++) {
    write_ch(i, wherey, '.', backcolor, textcolor);
  }
  write_ch(positionx + displayLength + 1, wherey, ']', backcolor, textcolor);
  // Reset keyboard
  if (kbhit() == 1)
    ch = readch();
  ch = 0;

  do {
    keypressed = kbhit();
    // Cursor Animation
    if (keypressed == 0) {

      cursor_count++;
      if (cursor_count == 100) {
        cursor_count = 0;
        switch (cursor_on) {
        case 1:
          pos_cursor   = positionx + 1;
          display_char = '.';
          if (pos_cursor == limit_cursor) {
            pos_cursor   = pos_cursor - 1;
            display_char = ch;
          }
          write_ch(pos_cursor, wherey, display_char, backcolor, textcolor);
          cursor_on = 0;
          break;
        case 0:
          pos_cursor = positionx + 1;
          if (pos_cursor == limit_cursor)
            pos_cursor = pos_cursor - 1;
          write_ch(pos_cursor, wherey, '|', backcolor, textcolor);
          cursor_on = 1;
          break;
        }
      }
    }
    // Process keys
    if (keypressed == 1) {
      ch         = readch();
      keypressed = 0;

      if (char_count < displayLength) {
        if (ch > 31 && ch < 127) {
          write_ch(positionx + 1, wherey, ch, backcolor, textcolor);
          text[char_count] = ch;
          positionx++;
          char_count++;
        }
      }
    }

    if (ch == K_BACKSPACE) {
      if (positionx > 0 && char_count > 0) {
        positionx--;
        char_count--;
        write_ch(positionx + 1, wherey, '.', backcolor, textcolor);
        if (positionx < limit_cursor - 2)
          write_ch(positionx + 2, wherey, '.', backcolor, textcolor);
        resetch();
      }
    }
    if (ch == K_ENTER || ch == K_ESCAPE)
      exit_flag = 1;

    // ENTER OR ESC TO FINISH LOOP
  } while (exit_flag != 1);
  // clear cursor
  write_ch(pos_cursor, wherey, ' ', backcolor, textcolor);
  resetch();
  return ch;
}

void window(int x1, int y1, int x2, int y2, int backcolor, int bordercolor, int titlecolor, int border, int title) {
  int i, j;
  i = x1;
  j = y1;
  // borders
  if (border == 1) {
    for (i = x1; i <= x2; i++) {
      // upper and lower borders
      write_ch(i, y1, HOR_LINE, backcolor, bordercolor); // horizontal line box-like char
      write_ch(i, y2, HOR_LINE, backcolor, bordercolor);
    }
    for (j = y1; j <= y2; j++) {
      // left and right borders
      write_ch(x1, j, VER_LINE, backcolor, bordercolor); // vertical line box-like char
      write_ch(x2, j, VER_LINE, backcolor, bordercolor);
    }
    write_ch(x1, y1, UPPER_LEFT_CORNER, backcolor, bordercolor);  // upper-left corner box-like char
    write_ch(x1, y2, LOWER_LEFT_CORNER, backcolor, bordercolor);  // lower-left corner box-like char
    write_ch(x2, y1, UPPER_RIGHT_CORNER, backcolor, bordercolor); // upper-right corner box-like char
    write_ch(x2, y2, LOWER_RIGHT_CORNER, backcolor, bordercolor); // lower-right corner box-like char
  }
  if (title == 1) {
    for (i = x1; i <= x2; i++)
      write_ch(i, y1 - 1, ' ', titlecolor, titlecolor);
  }
}

void to_upper(char *text) {
  // CHANGE LETTERS TO UPPERCASE
  size_t i = 0;
  for (i = 0; i < strlen(text); i++) {
    if (text[i] >= 97 && text[i] <= 122)
      text[i] = text[i] - 32;
  }
}

// GAME FUNCTIONS

void draw_board() {
  int i = 0, j = 0, shiftx = 0, shifty = 0;
  int size_x = 4;
  int size_y = 2;
  clean_area();
  shiftx = wherex + 4;
  shifty = wherey;
  write_str(wherex, wherey, "[C WORDLE FOR TERMINAL by        ]", B_BLACK, F_WHITE);
  write_str(wherex + 26, wherey, "v3l0r3k", B_BLACK, F_GREY);
  write_ch(wherex + 4, wherey + 2, 'a', B_BLACK, F_WHITE);

  for (j = 0; j < 6; j++) {
    for (i = 0; i < 5; i++) {
      window(shiftx, shifty + 1, (shiftx + size_x), shifty + 1 + size_y, B_BLACK, F_WHITE, F_WHITE, 1, 0);
      shiftx = shiftx + size_x + 1;
    }
    shifty = shifty + size_y + 1;
    shiftx = wherex + 4;
  }
  wherey = shifty;

  write_str(wherex, wherey + 1, "[ESC: EXIT | TYPE <help> for more]", B_BLACK, F_GREY);
}

void clean_area() {
  int i = 0, j = 0;
  for (j = 0; j < BOARDSIZEY; j++) {
    for (i = 0; i < 50; i++) {
      write_ch(oldx + i, oldy + j, ' ', B_BLACK, F_BLACK);
    }
  }
}

void display_help() {
  char ch = 0;
  clean_area();
  write_str(1, oldy, "C-WORDLE", B_WHITE, F_BLACK);
  write_str(1, oldy + 1, "Guess a 5-letter secret word in 6 tries.", B_BLACK, F_WHITE);
  write_str(1, oldy + 2, "Type any word to start.", B_BLACK, F_WHITE);
  write_str(1, oldy + 4, "[C]", B_GREEN, F_WHITE);
  write_str(4, oldy + 4, "-> LETTER IS IN THE RIGHT POSITION", B_BLACK, F_WHITE);
  write_str(1, oldy + 5, "[C]", B_YELLOW, F_WHITE);
  write_str(4, oldy + 5, "-> LETTER IS IN THE WRONG POSITION", B_BLACK, F_WHITE);
  write_str(1, oldy + 6, "[C]", B_BLACK, F_WHITE);
  write_str(4, oldy + 6, "-> LETTER IS NOT IN THE WORD", B_BLACK, F_WHITE);
  write_str(1, oldy + 8, "Type <exit> or <quit> to exit.", B_BLACK, F_WHITE);
  write_str(1, oldy + 9, "Type <cheat> to give up.", B_BLACK, F_WHITE);
  write_str(1, oldy + 10, "Total words: ", B_BLACK, F_GREY);
  write_num(16, oldy + 10, words, B_BLACK, F_GREY);
  write_str(1, oldy + 11, "Possible words: ", B_BLACK, F_GREY);
  write_num(18, oldy + 11, words2, B_BLACK, F_GREY);
  write_str(1, oldy + 13, "Press <ENTER> or <ESC> key to return.", B_BLACK, F_WHITE);
  wherex = oldx;
  wherey = oldy;
  do {
    gotoxy(1, 1);
    printf("\n");
    if (kbhit())
      ch = readch();
    if (ch == K_ESCAPE)
      break;
  } while (ch != K_ENTER);
  clean_area();
}

int find_index(char c)
// returns the index of a char in an array of chars
{
  int  i  = 0;
  char ch = 0;
  do {
    ch = secret_word[i];
    if (c == ch)
      break;
    i++;
  } while (i < 5);
  return i;
}

int check_green(char c, int index, char *str) {
  char   ch          = 0;
  size_t i           = 0;
  size_t lindex      = index;
  int    col         = B_BLACK;
  int    letter_index = 0;
  // color letters accordingly
  letter_index = find_index(c);
  for (i = 0; i < strlen(str); i++) {
    ch = str[i];
    if (c == ch && lindex == i) {
      col          = B_GREEN;
      check_true[i] = 1;
      repeated_letters[letter_index]--;
    }
  }
  return col;
}

int check_orange(char c, int index, char *str) {
  char   ch  = 0;
  size_t i   = 0;
  int    col = B_BLACK, letter_index = 0;

  letter_index = find_index(c);
  // color letters accordingly
  for (i = 0; i < strlen(str); i++) {
    ch = str[i];
    if (c == ch && repeated_letters[letter_index] > 0 && check_true[index] == 0) {
      col = B_YELLOW;
      repeated_letters[letter_index]--;
      break;
    }
  }
  if (check_true[index] == 1)
    col = B_GREEN;
  return col;
}

void write_word(int index, char text[MAX_TEXTBOX]) {
  int j = 0, i = 0, x = 0, y = 0, col = B_BLACK;
  x = oldx + 6;
  y = oldy + 2;

  check_repeated_letters();
  for (j = 0; j < index; j++)
    y = y + 3;
  // clean array of true values
  for (i = 0; i < 5; i++)
    check_true[i] = 0;
  // Search for Green letters
  for (i = 0; i < MAX_TEXTBOX; i++) {
    col = check_green(text[i], i, secret_word);
    write_ch(x, y, text[i], col, F_WHITE);
    x = x + 5;
  }
  // Search for Orange and Black letters
  x = oldx + 6;
  for (i = 0; i < 5; i++) {
    col = check_orange(text[i], i, secret_word);
    write_ch(x, y, text[i], col, F_WHITE);
    x = x + 5;
  }
}

void check_repeated_letters() {
  char   ch = 0;
  size_t i, j;
  for (i = 0; i < 5; i++)
    repeated_letters[i] = 1;
  for (i = 0; i < strlen(secret_word); i++) {
    ch = secret_word[i];
    for (j = i + 1; j < strlen(secret_word); j++) {
      if (ch == secret_word[j]) {
        repeated_letters[i] = repeated_letters[i] + 1;
        repeated_letters[j] = repeated_letters[i];
      }
    }
  }
}
void game_loop() {
  char ch    = 0;
  int  cheat = 0;

  do {
    cheat = 0;
    memset(&textbox1, '\0', sizeof(textbox1));
    ch = textbox(1, wherey + 2, 5, "[+] Word:", textbox1, F_WHITE, F_WHITE, F_WHITE);
    if (ch == K_ESCAPE)
      break;
    if (strcmp(textbox1, "exit") == 0 || strcmp(textbox1, "quit") == 0)
      break;
    if (strcmp(textbox1, "cheat") == 0) {
      write_str(17, oldy + 20, "                                  ", B_BLACK, F_GREEN);
      write_str(18, oldy + 20, secret_word, B_BLACK, F_GREEN);
      memset(&textbox1, '\0', sizeof(textbox1));
      cheat = 1;
    }
    if (strcmp(textbox1, "help") == 0)
      break;
    if (strlen(textbox1) == 5) {
      check_repeated_letters();
      to_upper(textbox1);

      if (is_wordin_dictionary(file_source, textbox1) == 1) {
        write_word(current_index, textbox1);
        strcpy(board_inputs[current_index], textbox1);
        write_str(wherex + 16, wherey + 2, "->VALID WORD!                   ", B_BLACK, F_GREEN);
        if (current_index < 6)
          current_index++;
        if (strcmp(textbox1, secret_word) == 0) {
          write_str(wherex, wherey + 1, "->SUCCESS!                        ", B_BLACK, F_BLUE);
          break;
        } else {
          if (current_index == 6) {
            write_str(wherex, wherey + 1, "->GAME OVER:                     ", B_BLACK, F_MAGENTA);
            write_str(wherex + 14, wherey + 1, secret_word, B_BLACK, F_GREEN);
            break;
          }
        }
      } else {
        write_str(wherex + 16, wherey + 2, "->WORD NOT FOUND!                ", B_BLACK, F_RED);
      }
    } else {
      if (cheat == 0)
        write_str(wherex + 16, wherey + 2, "->TOO SHORT!                      ", B_BLACK, F_RED);
    }
  } while (ch != K_ESCAPE);
  write_str(1, wherey + 2, "                                 ", B_BLACK, F_WHITE);
  if (strcmp(textbox1, "help") == 0) {
    display_help();
    new_game();
  }
}

void new_game() {
  int i = 0;
  draw_board();
  // Rewrite previous words on panel
  if (current_index > 0) {
    for (i = 0; i <= current_index; i++)
      write_word(i, board_inputs[i]);
  }
  game_loop();
}

int open_file(FILE **fileHandler, char *fileName, char *mode) {
  int ok;
  *fileHandler = fopen(fileName, mode);
  // check whether buffer is assigned
  // and return value
  if (*fileHandler != NULL)
    ok = 1;
  else
    ok = 0;
  return ok;
}

long count_words(FILE *fileHandler) {
  long word_count = 0;
  char ch;

  // Read char by char
  if (fileHandler != NULL) {
    rewind(fileHandler);    // Go to start of file
    ch = getc(fileHandler); // peek into file
    while (!feof(fileHandler)) {
      // Read until SEPARATOR 0x0A
      if (ch == SEPARATOR) {
        word_count++;
      }
      ch = getc(fileHandler);
    }
  }
  return word_count;
}

void get_wordfrom_dictionary(FILE *fileHandler, char WORD[MAX_TEXTBOX]) {
  long i = 0;
  char ch;
  char data_string[MAX_TEXTBOX];

  // Read char by char
  if (fileHandler != NULL) {
    rewind(fileHandler);
    // Go to where the word starts 6bytes * randomWord
    fseek(fileHandler, MAX_TEXTBOX * random_word, SEEK_SET);
    ch = getc(fileHandler); // peek into file
    while (i < 5) {
      // Read until SEPARATOR 0x0A
      if (ch != SEPARATOR)
        data_string[i++] = ch;
      // i++;
      ch = getc(fileHandler);
    }
  }
  data_string[i] = '\0'; // null-end string
  i             = 0;
  strcpy(WORD, data_string);
}

int is_wordin_dictionary(FILE *fileHandler, char WORD[MAX_TEXTBOX]) {
  long i       = 0;
  int  is_found = 0;
  char ch;
  char data_string[MAX_TEXTBOX];

  // Read char by char
  if (fileHandler != NULL) {
    rewind(fileHandler);    // Go to start of file
    ch = getc(fileHandler); // peek into file
    while (!feof(fileHandler)) {
      // Read until SEPARATOR 0x0A
      if (ch != SEPARATOR)
        data_string[i++] = ch;
      // i++;
      if (ch == SEPARATOR) {
        data_string[i] = '\0'; // null-end string
        if (strcmp(data_string, WORD) == 0) {
          is_found = 1;
          break;
        }
        i = 0;
      }
      ch = getc(fileHandler);
    }
  }
  return is_found;
}

int close_file(FILE *fileHandler) {
  int ok;
  ok = fclose(fileHandler);
  return ok;
}

void credits() {
  reset_term();
  gotoxy(wherex, wherey + 2);
  printf("\r");
  printf("C-Wordle. Coded by v3l0r3k 2022                   \n");
  showcursor();
  reset_ansi(0);
}

int main() {
  srand((unsigned)time(&t));
  // INIT TERMINAL
  get_pos(&wherey, &wherex);
  oldx = wherex;
  oldy = wherey;
  get_terminal_dimensions(&rows, &columns);
  if (rows < BOARDSIZEY || columns < BOARDSIZEX) {
    printf("Error: Terminal size is too small. Resize terminal. \n");
    exit(0);
  }
  if (rows - wherey < BOARDSIZEY) {
    cls();
    get_pos(&wherey, &wherex);
    oldx = wherex;
    oldy = wherey;
  }
  // SEARCH FOR DICTIONARY
  ok_file = open_file(&file_source, DICTIONARY, "r");
  ok_file = open_file(&file_source2, POSSIBLES, "r");
  if (ok_file == 0) {
    // No dictionary
    dictionary_present = 0;
    words             = 1;
    strcpy(secret_word, dummyWord);
    printf("ERROR: Dictionary not found. Create file <dict.txt>\n");
    exit(0);
  } else {
    // Dictionary is present
    dictionary_present = 1;
    words             = count_words(file_source);
    words2            = count_words(file_source2);
    // Selecting a random word from dictionary
    random_word = rand() % words2;
    get_wordfrom_dictionary(file_source2, secret_word);
    push_term();
    hidecursor();
    resetch();
    // Set Locale For Unicode characters to work
    setlocale(LC_ALL, "");
    // GAME
    new_game();
    if (file_source != NULL)
      close_file(file_source);
    if (file_source2 != NULL)
      close_file(file_source2);
    credits();
  }
  return 0;
}

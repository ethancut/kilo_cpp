#include <cstdlib>
#include <iostream>
#include <string>
#include <windows.h>

/***  defines  ***/
#define CTRL_KEY(k) ((k) & 0x1f)
#define NOTEBOOK_VERSION "0.0.1"
/***  data  ***/
typedef struct {
  DWORD originalMode;
  int screenrows, screencols;
  int cx, cy;
} editorConfig;

editorConfig E;

/***  terminal  ***/
void die() {
  std::cout << "\x1b[H\x1b[2J\x1b[3J" << std::flush;
  std::cout << GetLastError() << std::endl;
  exit(1);
}
int getWindowSize(int *rows, int *cols) {
  HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  CONSOLE_SCREEN_BUFFER_INFO csbi;

  if (GetConsoleScreenBufferInfo(hConsole, &csbi)) {
    *cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    *rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    return 0;
  } else {
    return -1;
  }
}
unsigned char editorReadKey() {
  DWORD nread;
  INPUT_RECORD input;
  while (true) {
    if (!ReadConsoleInputA(GetStdHandle(STD_INPUT_HANDLE), &input, 1, &nread) ||
        nread != 1)
      die();
    if (input.EventType == KEY_EVENT && input.Event.KeyEvent.bKeyDown) {
      return input.Event.KeyEvent.uChar.AsciiChar;
    }
  }
}
int getCursorPosition(int *rows, int *cols) {
  HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  CONSOLE_SCREEN_BUFFER_INFO csbi;

  if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) {
    return -1;
  }

  *rows = csbi.dwCursorPosition.Y - csbi.srWindow.Top + 1;
  *cols = csbi.dwCursorPosition.X - csbi.srWindow.Left + 1;

  return 0;
}
/***  append buffer  ***/

void abAppend(std::string &ab, const char *s) { ab += s; }

/*** output ***/
void editorDrawRows(std::string &ab) {

  for (int y = 0; y < E.screenrows - 1; ++y) {
    if (y == E.screenrows / 3) {
      std::string welcome =
          std::format("Notebook editor -- version {}\r\n", NOTEBOOK_VERSION);
      if (welcome.length() > static_cast<size_t>(E.screencols))
        welcome.erase(E.screencols);

      int padding = (E.screencols - welcome.length()) / 2;
      if (padding) {
        ab += "~";
        padding--;
      }
      while (padding--)
        ab += " ";
      ab += welcome;
    } else {
      ab += "~\x1b[K\r\n";
    }
  }
  ab += "~\x1b[K";
}
void editorRefreshScreen() {
  std::string ab;
  ab += "\x1b[?25l"; // hide cursor
  ab += "\x1b[H";
  editorDrawRows(ab);

  ab += "\x1b[H";
  ab += "\x1b[?25h"; // show cursor

  std::cout << ab << std::flush;
}

void disableRawMode() {
  HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
  if (!SetConsoleMode(hInput, E.originalMode))
    die();
}
void enableRawMode() {
  // restore console to normal state after program execution
  atexit(disableRawMode);
  HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
  if (hInput == INVALID_HANDLE_VALUE)
    die();

  if (!GetConsoleMode(hInput, &E.originalMode))
    die();

  DWORD rawMode = E.originalMode;
  rawMode &=
      ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_VIRTUAL_TERMINAL_INPUT);
  rawMode &= ~(ENABLE_PROCESSED_INPUT |
               ENABLE_PROCESSED_OUTPUT); // disables ctrl + c handling
  std::cout << "\x1b[H\x1b[2J\x1b[3J" << std::flush;
  if (!SetConsoleMode(hInput, rawMode)) {
    die();
  }
}

/***input ***/

void editorProcessKeypress() {
  unsigned char c = editorReadKey();
  switch (c) {
  case CTRL_KEY('q'):
    std::cout << "\x1b[H\x1b[2J\x1b[3J" << std::flush;
    exit(0);
    break;
  }
}
/***  init  ***/
void initEditor() {
  if (getWindowSize(&E.screenrows, &E.screencols) == -1)
    die();
}
int main() {
  enableRawMode();
  initEditor();

  while (true) {
    editorRefreshScreen();
    editorProcessKeypress();
  }

  return 0;
}
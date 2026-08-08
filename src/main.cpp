#include <cstdlib>
#include <iostream>
#include <windows.h>

/***  defines  ***/
#define CTRL_KEY(k) ((k) & 0x1f)
/***  data  ***/
typedef struct {
  DWORD originalMode;
  int screenrows;
  int screencols;
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
void editorDrawRows() {

  for (int y = 0; y < E.screenrows - 1; ++y) {
    std::cout << "~\r\n";
  }
  std::cout << "~";
}
void editorRefreshScreen() {
  std::cout << "\x1b[H\x1b[2J\x1b[3J" << std::flush;

  editorDrawRows();

  std::cout << "\x1b[H";
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
  std::cout << std::format("rows: {} cols: {}", E.screenrows, E.screencols)
            << std::endl;
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
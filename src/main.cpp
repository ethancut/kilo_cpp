// #include <cctype>
#include <cstdlib>
#include <iostream>
// #include <processenv.h>
// #include <winbase.h>
#include <windows.h>
/***  defines  ***/
#define CTRL_KEY(k) ((k) & 0x1f)
/***  data  ***/
DWORD originalMode = 0;

/***  terminal  ***/
void editorDrawRows() {

  for (int y = 0; y < 24; ++y) {
    std::cout << "~" << std::endl;
  }
}
void editorRefreshScreen() {
  std::cout << "\x1b[H\x1b[2J\x1b[3J" << std::flush;

  editorDrawRows();

  std::cout << "\x1b[H";
}
void die() {
  std::cout << "\x1b[H\x1b[2J\x1b[3J" << std::flush;
  std::cout << GetLastError();
  exit(1);
}

void disableRawMode() {
  HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
  if (!SetConsoleMode(hInput, originalMode))
    die();
}
void enableRawMode() {
  atexit(disableRawMode);
  HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
  if (hInput == INVALID_HANDLE_VALUE)
    die();

  if (!GetConsoleMode(hInput, &originalMode))
    die();

  DWORD rawMode = originalMode;
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
int main() {
  enableRawMode();

  while (true) {
    editorProcessKeypress();
    editorRefreshScreen();
  }

  // restore console to normal state after program execution

  return 0;
}
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <windows.h>
#include <winuser.h>

namespace fs = std::filesystem;

/***  defines  ***/
#define CTRL_KEY(k) ((k) & 0x1f)
#define NOTEBOOK_VERSION "0.0.1"

enum editorKey {
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  DEL_KEY,
  PAGE_UP,
  PAGE_DOWN,
  HOME_KEY,
  END_KEY
};
/***  data  ***/
typedef struct {
  DWORD originalMode;
  int screenrows, screencols;
  int cx, cy;
  int rowoff;
  int coloff;
  std::vector<std::string> rows;
} editorConfig;

editorConfig E;

/***  terminal  ***/
void die() {
  std::cout << "\x1b[H\x1b[2J\x1b[3J" << std::flush;
  std::cout << "WinAPI ERR: " << GetLastError() << std::endl;
  exit(1);
}
void die(std::string_view err) {
  std::cout << "ERR: " << err << std::endl;
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

int editorReadKey() {
  DWORD nread;
  INPUT_RECORD input;
  while (true) {
    if (!ReadConsoleInputA(GetStdHandle(STD_INPUT_HANDLE), &input, 1, &nread) ||
        nread != 1)
      die();
    if (input.EventType != KEY_EVENT || !input.Event.KeyEvent.bKeyDown) {
      continue;
    }
    KEY_EVENT_RECORD &key = input.Event.KeyEvent;
    switch (key.wVirtualKeyCode) {
    case VK_UP:
      return ARROW_UP;
      break;
    case VK_DOWN:
      return ARROW_DOWN;
      break;
    case VK_LEFT:
      return ARROW_LEFT;
      break;
    case VK_RIGHT:
      return ARROW_RIGHT;
      break;
    case VK_PRIOR:
      return PAGE_UP;
      break;
    case VK_NEXT:
      return PAGE_DOWN;
      break;
    case VK_HOME:
      return HOME_KEY;
      break;
    case VK_END:
      return END_KEY;
      break;
    case VK_DELETE:
      return DEL_KEY;
      break;
    default:
      return key.uChar.AsciiChar;
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
/***  row operations  ***/

/***  file i/o  ***/
void editorOpen(fs::path name) {
  std::fstream file(name);
  if (!file.is_open())
    die("Invalid filepath");
  std::string line;

  // while (std::getline(file, line)) {
  //   E.row += line;

  // }
  while (std::getline(file, line)) {
    E.rows.emplace_back(line);
  }
}
void editorOpen() {
  std::string line = "hello world!";

  E.rows.emplace_back(line);
}
/***  append buffer  ***/

void abAppend(std::string &ab, const char *s) { ab += s; }

/*** output ***/
void editorScroll() {
  if (E.cy < E.rowoff) {
    E.rowoff = E.cy;
  }
  if (E.cy >= E.rowoff + E.screenrows) {
    E.rowoff = E.cy - E.screenrows + 1;
  }
  if (E.cx < E.coloff) {
    E.coloff = E.cx;
  }
  if (E.cx >= E.coloff + E.screencols) {
    E.coloff = E.cx - E.screencols + 1;
  }
}
void editorDrawRows(std::string &ab) {

  for (int y = 0; y < E.screenrows; ++y) {
    int filerow = y + E.rowoff;
    if (static_cast<size_t>(filerow) >= E.rows.size()) {
      if (y == E.screenrows / 3 && E.rows.size() == 0) {
        std::string welcome =
            std::format("Notebook editor -- version {}", NOTEBOOK_VERSION);
        if (welcome.length() > static_cast<size_t>(E.screencols))
          welcome.erase(E.screencols);

        int padding = (E.screencols - welcome.length()) / 2;
        if (padding) {
          ab += "~";
          --padding;
        }
        ab.append(padding, ' ');
        ab += welcome;
      } else {
        ab += "~";
      }
    } else {
      int len = E.rows[filerow].size() - E.coloff;
      if (len < 0)
        len = 0;
      if (len > E.screencols)
        len = E.screencols;
      ab.append(E.rows[filerow], E.coloff, len);
    }
    ab += "\x1b[K";

    if (y < E.screenrows - 1)
      ab += "\r\n";
  }
}

void editorRefreshScreen() {
  editorScroll();
  std::string ab;
  ab += "\x1b[?25l"; // hide cursor
  ab += "\x1b[H";
  editorDrawRows(ab);

  std::string buf = std::format("\x1b[{};{}H", (E.cy - E.rowoff) + 1, E.cx + 1);
  ab += buf;
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
void editorMoveCursor(int key) {

  const std::string *row =
      (static_cast<size_t>(E.cy) < E.rows.size()) ? &E.rows[E.cy] : nullptr;
  switch (key) {
  case ARROW_LEFT:
    if (E.cx != 0) {
      E.cx--;

    } else if (E.cy > 0) {
      E.cy--;
      E.cx = E.rows[E.cy].size();
    }
    break;
  case ARROW_RIGHT: {

    if (row && static_cast<size_t>(E.cx) < row->size()) {
      E.cx++;
    } else if (row && static_cast<size_t>(E.cx) == row->size()) {
      E.cy++;
      E.cx = 0;
    }
    break;
  }
  case ARROW_UP:
    if (E.cy != 0)
      E.cy--;
    break;
  case ARROW_DOWN:
    if (static_cast<size_t>(E.cy) < E.rows.size())
      E.cy++;
    break;
  }
  row = (static_cast<size_t>(E.cy) < E.rows.size()) ? &E.rows[E.cy] : nullptr;
  int rowlen = row ? row->size() : 0;
  if (E.cx > rowlen) {
    E.cx = rowlen;
  }
}
void editorProcessKeypress() {
  int c = editorReadKey();
  switch (c) {
  case CTRL_KEY('q'):
    std::cout << "\x1b[H\x1b[2J\x1b[3J" << std::flush;
    exit(0);
    break;

  case ARROW_LEFT:
  case ARROW_RIGHT:
  case ARROW_UP:
  case ARROW_DOWN:
    editorMoveCursor(c);
    break;
  case PAGE_UP:
  case PAGE_DOWN: {
    int times = E.screenrows;
    while (times--)
      editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
    break;
  }
  case HOME_KEY:
    E.cx = 0;
    break;
  case END_KEY:
    E.cx = E.screencols - 1;
    break;
  }
}
/***  init  ***/
void initEditor() {
  E.cx = E.cy = E.rowoff = E.coloff = 0;
  if (getWindowSize(&E.screenrows, &E.screencols) == -1)
    die();
}
int main(int argc, char *argv[]) {
  enableRawMode();
  initEditor();
  if (argc >= 2) {
    editorOpen(argv[1]);
  } else {
    editorOpen();
  }

  while (true) {
    editorRefreshScreen();
    editorProcessKeypress();
  }

  return 0;
}
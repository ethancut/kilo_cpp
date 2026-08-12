
#include <ostream>
#define NOMINMAX
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <windows.h>

// #include <winuser.h>

namespace fs = std::filesystem;

/***  defines  ***/
#define CTRL_KEY(k) ((k) & 0x1f)
#define NOTEBOOK_VERSION "0.0.1"
#define NOTEBOOK_TAB_STOP 8

enum editorKey {
  BACKSPACE = 127,
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
  int rx;
  int rowoff;
  int coloff;
  std::vector<std::string> rows;
  std::vector<std::string> render;
  std::string filename;
  std::string statusmsg;
  std::chrono::steady_clock::time_point statusmsg_time;
} editorConfig;

editorConfig E;
/***  prototypes  ***/
template <typename... Args>
void editorSetStatusMessage(const std::format_string<Args...> fmt,
                            Args &&...args) {
  E.statusmsg = std::format(fmt, std::forward<Args>(args)...);
  E.statusmsg_time = std::chrono::steady_clock::now();
}
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
    case VK_BACK:
      return BACKSPACE;
    default:
      if (key.uChar.AsciiChar == 0) {
        continue;
      }
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

int editorRowRxToCx(size_t index, size_t cx) {
  std::string &row = E.rows[index];
  int rx = 0;
  for (size_t j = 0; j < cx; ++j) {
    if (row[j] == '\t')
      rx += (NOTEBOOK_TAB_STOP - 1) - (rx % NOTEBOOK_TAB_STOP);
    ++rx;
  }
  return rx;
}
void editorUpdateRow(size_t index) {
  const std::string &row = E.rows[index];

  int tabs = 0;
  for (const char &c : row) {
    if (c == '\t')
      ++tabs;
  }

  std::string rendered;
  rendered.reserve(row.size() + tabs * (NOTEBOOK_TAB_STOP - 1));

  for (const char &c : row) {
    if (c == '\t') {
      rendered += ' ';
      while (rendered.size() % NOTEBOOK_TAB_STOP != 0) {
        rendered += ' ';
      }
    } else {
      rendered += c;
    }
  }

  E.render[index] = std::move(rendered);
}
void editorAppendRow(std::string s) {
  E.rows.emplace_back(s);

  E.render.emplace_back(s);
  size_t index = E.render.size() - 1;
  editorUpdateRow(index);
}
void editorRowInsertChar(size_t index, size_t at, int c) {
  std::string &row = E.rows[index];
  if (at > row.size())
    at = row.size();
  row.insert(at, 1, static_cast<char>(c));
  editorUpdateRow(index);
}
/*** editor operations ***/
void editorInsertChar(int c) {
  if (E.cy == E.rows.size()) {
    editorAppendRow("");
  }
  editorRowInsertChar(E.cy, E.cx, c);
  ++E.cx;
}
/***  file i/o  ***/
void editorOpen(fs::path name) {
  std::fstream file(name);
  E.filename = name.string();
  if (!file.is_open())
    die("Invalid filepath");
  std::string line;

  while (std::getline(file, line)) {
    editorAppendRow(line);
  }
}
void editorOpen() {
  std::string line = "hello world!";

  editorAppendRow(line);
}

void editorSave() {
  if (E.filename.empty())
    return;
  std::ofstream fd(E.filename);
  if (!fd.is_open())
    return;
  size_t bytesWritten = 0;
  for (const std::string &line : E.rows) {
    fd << line << std::endl;
    bytesWritten += line.size() + 1;
  }
  fd.close();

  if (fd.fail()) {
    editorSetStatusMessage("Write/Save error");
  } else {
    editorSetStatusMessage("{} bytes written to disc", bytesWritten);
  }
}
/***  append buffer  ***/

void abAppend(std::string &ab, const char *s) { ab += s; }

/*** output ***/

void editorScroll() {
  E.rx = 0;
  if (static_cast<size_t>(E.cy) < E.rows.size()) {
    E.rx = editorRowRxToCx(E.cy, E.cx);
  }
  if (E.cy < E.rowoff) {
    E.rowoff = E.cy;
  }
  if (E.cy >= E.rowoff + E.screenrows) {
    E.rowoff = E.cy - E.screenrows + 1;
  }
  if (E.rx < E.coloff) {
    E.coloff = E.rx;
  }
  if (E.rx >= E.coloff + E.screencols) {
    E.coloff = E.rx - E.screencols + 1;
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
      int len = E.render[filerow].size() - E.coloff;
      if (len < 0)
        len = 0;
      if (len > E.screencols)
        len = E.screencols;
      ab.append(E.render[filerow], E.coloff, len);
    }
    ab += "\x1b[K";

    // if (y < E.screenrows - 1)
    ab += "\r\n";
  }
}
void editorDrawStatusbar(std::string &ab) {
  ab += "\x1b[7m";
  std::string status = std::format(
      "{:.20s} - {} lines", (!E.filename.empty()) ? E.filename : "[No Name]",
      E.rows.size());
  std::string rstatus = std::format("{}/{}", E.cy + 1, E.rows.size());

  int len = std::min(status.size(), static_cast<size_t>(E.screencols));
  int rlen = rstatus.size();
  ab.append(status, 0, len);
  while (len < E.screencols) {
    if (E.screencols - len == rlen) {
      ab += rstatus;
      break;
    } else {
      ab += ' ';
      ++len;
    }
  }
  ab += "\x1b[m";
  ab += "\r\n";
}
void editorDrawMessageBar(std::string &ab) {
  ab += "\x1b[K";
  size_t msglen = E.statusmsg.length();
  if (msglen > static_cast<size_t>(E.screencols))
    msglen = E.screencols;

  if (msglen && std::chrono::steady_clock::now() - E.statusmsg_time <
                    std::chrono::seconds(5)) {
    ab.append(E.statusmsg, 0, msglen);
  }
}
void editorRefreshScreen() {
  editorScroll();
  std::string ab;
  ab += "\x1b[?25l"; // hide cursor
  ab += "\x1b[H";
  editorDrawRows(ab);
  editorDrawStatusbar(ab);
  editorDrawMessageBar(ab);

  std::string buf =
      std::format("\x1b[{};{}H", (E.cy - E.rowoff) + 1, (E.rx - E.coloff) + 1);
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
  case '\r':
    break;

  case CTRL_KEY('q'):
    std::cout << "\x1b[H\x1b[2J\x1b[3J" << std::flush;
    exit(0);
    break;
  case CTRL_KEY('s'):
    editorSave();
    break;
  case ARROW_LEFT:
  case ARROW_RIGHT:
  case ARROW_UP:
  case ARROW_DOWN:
    editorMoveCursor(c);
    break;
  case PAGE_UP:
  case PAGE_DOWN: {
    if (c == PAGE_UP) {
      E.cy = E.rowoff;
    } else if (c == PAGE_DOWN) {
      E.cy = E.rowoff + E.screenrows + 1;
      if (static_cast<size_t>(E.cy) > E.rows.size())
        E.cy = E.rows.size();
    }
    int times = E.screenrows;
    while (times--)
      editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
    break;
  }
  case HOME_KEY:
    E.cx = 0;
    break;
  case END_KEY:
    if (static_cast<size_t>(E.cy) < E.rows.size()) {
      E.cx = E.rows[E.cy].size();
    }
    break;
  case BACKSPACE:
  case CTRL_KEY('h'):
  case DEL_KEY:
    /* TODO*/
    break;
  case CTRL_KEY('l'):
  case '\x1b':
    break;
  default:
    editorInsertChar(c);
    break;
  }
}
/***  init  ***/
void initEditor() {
  E.cx = E.cy = E.rx = E.rowoff = E.coloff = 0;
  if (getWindowSize(&E.screenrows, &E.screencols) == -1)
    die();
  E.screenrows -= 2;
}
int main(int argc, char *argv[]) {
  enableRawMode();
  initEditor();
  if (argc >= 2) {
    editorOpen(argv[1]);
  } else {
    editorOpen();
  }
  editorSetStatusMessage("HELP: CTRL-S = save | Ctrl-Q = quit");

  while (true) {
    editorRefreshScreen();
    editorProcessKeypress();
  }

  return 0;
}
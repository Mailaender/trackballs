/* editWindows.cc
   Implements all the windows in the editMode

   Copyright (C) 2003-2004  Mathias Broxvall

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "editWindows.h"
#include "editMode.h"
#include "map.h"
#include "menuMode.h"
#include "menusystem.h"
#include "settings.h"

#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_timer.h>
#include <dirent.h>

EMenuWindow::EMenuWindow() : MyWindow(0, 0, screenWidth, 30) {
  activeSubID = -1;
  activeSubWindow = NULL;
  spacing = screenWidth / N_SUBMENUS;
  for (int i = 0; i < N_SUBMENUS; i++) subWindows[i] = new ESubWindow(i, spacing * i, 30);
}
EMenuWindow::~EMenuWindow() {
  for (int i = 0; i < N_SUBMENUS; i++) delete subWindows[i];
}
void EMenuWindow::refreshChildPositions() {
  this->resize(screenWidth, 30);
  spacing = screenWidth / N_SUBMENUS;
  for (int i = 0; i < N_SUBMENUS; i++) { subWindows[i]->moveTo(spacing * i, 30); }
}

void EMenuWindow::draw() {
  int fontSize = 20;  // 16;
  char str[256];

  this->MyWindow::draw();
  for (int i = 0; i < N_SUBMENUS; i++) {
    snprintf(str, sizeof(str), "F%d %s", i + 1, cMenuNames[i]);
    addText_Center(CODE_FROM_MENU(i), fontSize / 2, height / 2, str,
                   spacing * i + spacing / 2);
  }
}
void EMenuWindow::mouseDown(int /*state*/, int /*x*/, int /*y*/) {
  int code = getSelectedArea();
  int menu = CODE_TO_MENU(code);
  if (menu >= 0 && menu < N_SUBMENUS) openSubMenu(menu);
}
void EMenuWindow::openSubMenu(int id) {
  if (activeSubWindow) activeSubWindow->remove();
  if (activeSubID == id) { /* Close window instead if already opened */
    activeSubID = -1;
    return;
  }
  activeSubID = id;
  activeSubWindow = subWindows[id];
  activeSubWindow->attach();
}
int EMenuWindow::keyToMenuEntry(int key, int shift) const {
  /* Convert keypad numbers to normal numbers */
  if (key >= SDLK_KP_0 && key <= SDLK_KP_9) key = '0' + key - SDLK_KP_0;
  if (key == SDLK_KP_PLUS) key = '+';
  if (key == SDLK_KP_MINUS) key = '-';

  /* Make all keys lower case since SHIFT and uppercase keys have
         a special meaning */
  if (shift) key = key - 'A' + 'a';
  if (key == SDLK_UP) key = 'U';
  if (key == SDLK_LEFT) key = 'L';
  if (key == SDLK_RIGHT) key = 'R';
  if (key == SDLK_DOWN) key = 'D';

  /* Search current menu first */
  int i = activeSubID;
  if (i != -1)
    for (int j = 0; cKeyShortcuts[i][j]; j++)
      if (cKeyShortcuts[i][j] == key) return i * MAX_MENU_ENTRIES + j;
  for (i = 0; i < N_SUBMENUS; i++)
    for (int j = 0; cKeyShortcuts[i][j]; j++)
      if (cKeyShortcuts[i][j] == key) return i * MAX_MENU_ENTRIES + j;
  return -1;
}
void EMenuWindow::key(int key, int shift, int /*x*/, int /*y*/) {
  if (key >= SDLK_F1 && key <= SDLK_F12) {
    int menu = key - SDLK_F1;
    if (menu >= 0 && menu < N_SUBMENUS) openSubMenu(menu);
  } else {
    int command = keyToMenuEntry(key, shift);
    if (command >= 0) EditMode::editMode->doCommand(command);
  }
}

ESubWindow::ESubWindow(int id, int x, int y) : MyWindow(x, y, 100, 0) {
  fontSize = 16;

  this->id = id;
  rows = countRows();
  resize(180, fontSize * rows + 2);
}

void ESubWindow::draw() {
  char str[256];

  this->MyWindow::draw();
  for (int i = 0; i < rows; i++) {
    if (cMenuEntries[id][i][0] == '*') {
      snprintf(str, 255, "%s", cMenuEntries[id][i] + 1);
      addText_Left(0, fontSize / 2, y + 2 + fontSize * i + fontSize / 2, str,
                   x + 2 + fontSize / 2);
    } else {
      if (cMenuEntries[id][i][0] == '/')
        snprintf(str, 255, "%s", cMenuEntries[id][i] + 1); /* Shortcut already part of name */
      else
        snprintf(str, 255, "%c %s", cKeyShortcuts[id][i], cMenuEntries[id][i]);

      addText_Left(CODE_FROM_MENUENTRY(id * MAX_MENU_ENTRIES + i), fontSize / 2,
                   y + 2 + fontSize * i + fontSize / 2, str, x + 2 + fontSize / 2);
      if (id == FLAGS_MENU && i < 9) {
        /* The flags current status can only be added if we have a map loaded */
        if (EditMode::editMode->map) {
          Cell& cell =
              EditMode::editMode->map->cell(EditMode::editMode->x, EditMode::editMode->y);
          if (cell.flags & (1 << i) || (cell.flags & (1 << 11) && i == 9))
            addText_Left(CODE_FROM_MENUENTRY(id * MAX_MENU_ENTRIES + i), fontSize / 2,
                         y + 2 + fontSize * i + fontSize / 2, _("y"),
                         x + 2 + fontSize / 2 + 160);
          else
            addText_Left(CODE_FROM_MENUENTRY(id * MAX_MENU_ENTRIES + i), fontSize / 2,
                         y + 2 + fontSize * i + fontSize / 2, _("n"),
                         x + 2 + fontSize / 2 + 160);
        }
      } else if (id == FLAGS_MENU && i == 10) {
        /* The current texture can only be added if we have a map loaded */
        if (EditMode::editMode->map) {
          Cell& cell =
              EditMode::editMode->map->cell(EditMode::editMode->x, EditMode::editMode->y);
          snprintf(str, sizeof(str), "%d", cell.texture);
          addText_Left(CODE_FROM_MENUENTRY(id * MAX_MENU_ENTRIES + i), fontSize / 2,
                       y + 2 + fontSize * i + fontSize / 2, str, x + 2 + fontSize / 2 + 160);
        }
      }
    }
  }
}

/** Count how many menu entries exists for a given menu. Uses the length of the shortcut key
 * string for this */
int ESubWindow::countRows() const {
  for (int i = 0; i < MAX_MENU_ENTRIES; i++)
    if (!cMenuEntries[id][i]) return i;
  return MAX_MENU_ENTRIES;
}

void ESubWindow::mouseDown(int /*state*/, int /*x*/, int /*y*/) {
  int code = getSelectedArea();
  int menuentry = CODE_TO_MENUENTRY(code);
  if (menuentry >= id * MAX_MENU_ENTRIES && menuentry < (id + 1) * MAX_MENU_ENTRIES)
    EditMode::editMode->doCommand(menuentry);
}

EStatusWindow::EStatusWindow() : MyWindow(0, screenHeight - 110, screenWidth, 110) {}

void EStatusWindow::draw() {
  int fontSize = 24;

  this->MyWindow::draw();
  if (!EditMode::editMode->map) return;
  int row1 = y + fontSize / 2 + 2;
  int row2 = row1 + fontSize + 2;
  int row3 = row2 + fontSize + 2;
  int row4 = row3 + fontSize + 2;
  int col0 = x + 2 + fontSize / 2;
  int col1 = x + 2 + fontSize / 2 + fontSize * 10;
  int area3x = x + 520;
  char str[512];

  const char* editModeNames[6] = {_("height"),   _("color"), _("water"),
                                  _("velocity"), _("lines"), _("feature")};

  snprintf(str, 255, _("Pos: %d,%d"), EditMode::editMode->x, EditMode::editMode->y);
  addText_Left(0, fontSize / 2, row2, str, col0);
  snprintf(str, 255, _("Increment: %3.2f"), EditMode::editMode->scale);
  addText_Left(CODE_INCREMENT, fontSize / 2, row3, str, col0);
  snprintf(str, 255, _("Edit: %s"), editModeNames[EditMode::editMode->currentEditMode]);
  addText_Left(CODE_EDITMODE, fontSize / 2, row4, str, col0);

  /* Small separator between area 1 - 2 */
  draw2DRectangle(col1 - fontSize - 1, y, 2, height, 0., 0., 1., 1.,
                  Color(SRGBColor(0.5, 0.5, 0.5, 1.0)));
  /* Small separator between area 2 - 3 */
  draw2DRectangle(area3x - 1, y, 2, height, 0., 0., 1., 1.,
                  Color(SRGBColor(0.5, 0.5, 0.5, 1.0)));

  /* TODO. Make the height/colour etc. text selectable areas
         with the same effect as corresponding menu choice */

  Cell& cell = EditMode::editMode->map->cell(EditMode::editMode->x, EditMode::editMode->y);

  if (EditMode::editMode->currentEditMode == EDITMODE_HEIGHT) {
    addText_Left(0, fontSize / 2, row1, _("Heights"), col1 + fontSize * 1);
    snprintf(str, sizeof(str), "%2.1f", cell.heights[3]);
    addText_Left(CODE_FROM_MENUENTRY(EDIT_UPPER), fontSize / 2, row2, str,
                 col1 + fontSize * 2);
    snprintf(str, sizeof(str), "%2.1f", cell.heights[1]);
    addText_Left(CODE_FROM_MENUENTRY(EDIT_LEFT), fontSize / 2, row3, str, col1 + fontSize * 0);
    snprintf(str, sizeof(str), "%2.1f", cell.heights[Cell::CENTER]);
    addText_Left(CODE_FROM_MENUENTRY(EDIT_CENTER), fontSize / 2, row3, str,
                 col1 + fontSize * 2);
    snprintf(str, sizeof(str), "%2.1f", cell.heights[2]);
    addText_Left(CODE_FROM_MENUENTRY(EDIT_RIGHT), fontSize / 2, row3, str,
                 col1 + fontSize * 4);
    snprintf(str, sizeof(str), "%2.1f", cell.heights[0]);
    addText_Left(CODE_FROM_MENUENTRY(EDIT_BOTTOM), fontSize / 2, row4, str,
                 col1 + fontSize * 2);
  } else if (EditMode::editMode->currentEditMode == EDITMODE_WATER) {
    addText_Left(0, fontSize / 2, row1, _("Water heights"), col1 + fontSize * 1);
    snprintf(str, sizeof(str), "%2.1f", cell.waterHeights[3]);
    addText_Left(CODE_FROM_MENUENTRY(EDIT_UPPER), fontSize / 2, row2, str,
                 col1 + fontSize * 2);
    snprintf(str, sizeof(str), "%2.1f", cell.waterHeights[1]);
    addText_Left(CODE_FROM_MENUENTRY(EDIT_LEFT), fontSize / 2, row3, str, col1 + fontSize * 0);
    snprintf(str, sizeof(str), "%2.1f", cell.waterHeights[Cell::CENTER]);
    addText_Left(CODE_FROM_MENUENTRY(EDIT_CENTER), fontSize / 2, row3, str,
                 col1 + fontSize * 2);
    snprintf(str, sizeof(str), "%2.1f", cell.waterHeights[2]);
    addText_Left(CODE_FROM_MENUENTRY(EDIT_RIGHT), fontSize / 2, row3, str,
                 col1 + fontSize * 4);
    snprintf(str, sizeof(str), "%2.1f", cell.waterHeights[0]);
    addText_Left(CODE_FROM_MENUENTRY(EDIT_BOTTOM), fontSize / 2, row4, str,
                 col1 + fontSize * 2);
  } else if (EditMode::editMode->currentEditMode == EDITMODE_COLOR) {
    snprintf(str, 255, _("red: %2.2f"), EditMode::editMode->color.f0());
    addText_Left(CODE_FROM_MENUENTRY(COLOR_RED), fontSize / 2, row2, str, col1 + fontSize * 3);
    snprintf(str, 255, _("green: %2.2f"), EditMode::editMode->color.f1());
    addText_Left(CODE_FROM_MENUENTRY(COLOR_GREEN), fontSize / 2, row3, str,
                 col1 + fontSize * 3);
    snprintf(str, 255, _("blue: %2.2f"), EditMode::editMode->color.f2());
    addText_Left(CODE_FROM_MENUENTRY(COLOR_BLUE), fontSize / 2, row4, str,
                 col1 + fontSize * 3);
    snprintf(str, 255, _("alpha: %2.2f"), EditMode::editMode->color.f3());
    addText_Left(CODE_FROM_MENUENTRY(COLOR_ALPHA), fontSize / 2, row1, str,
                 col1 + fontSize * 3);

    draw2DRectangle(col1, row2, 2 * fontSize, row4 - row2, 0., 0., 1., 1.,
                    Color(EditMode::editMode->color));
  } else if (EditMode::editMode->currentEditMode == EDITMODE_VELOCITY) {
    Cell& cell = EditMode::editMode->map->cell(EditMode::editMode->x, EditMode::editMode->y);
    snprintf(str, 255, _("dx: %2.2f"), cell.velocity[0]);
    addText_Left(0, fontSize / 2, row1, str, col1 + fontSize * 1);
    snprintf(str, 255, _("dy: %2.2f"), cell.velocity[1]);
    addText_Left(0, fontSize / 2, row1, str, col1 + fontSize * 7);
  } else if (EditMode::editMode->currentEditMode == EDITMODE_NOLINES) {
    addText_Left(0, fontSize / 2, row1, _("Lines"), col1);

    Color w(1., 1., 1., 1.), b(0., 0., 0., 1.);
    Color line_off[4] = {w, w, w, w};
    Color line_on[4] = {b, b, b, b};
    GLfloat txco[4][2] = {{0., 0.}, {0., 0.}, {0., 0.}, {0., 0.}};

    GLfloat r = 1.5;
    GLfloat lineA[4][2] = {{col1 + 0.f, row3 - r},
                           {col1 + 0.f, row3 + r},
                           {col1 + fontSize * 1.5f, row2 - r},
                           {col1 + fontSize * 1.5f, row2 + r}};
    draw2DQuad(lineA, txco, (cell.flags & CELL_NOLINENORTH) ? line_off : line_on);

    GLfloat lineB[4][2] = {{col1 + fontSize * 3.f, row3 - r},
                           {col1 + fontSize * 3.f, row3 + r},
                           {col1 + fontSize * 1.5f, row2 - r},
                           {col1 + fontSize * 1.5f, row2 + r}};
    draw2DQuad(lineB, txco, (cell.flags & CELL_NOLINEEAST) ? line_off : line_on);

    GLfloat lineC[4][2] = {{col1 + fontSize * 3.f, row3 - r},
                           {col1 + fontSize * 3.f, row3 + r},
                           {col1 + fontSize * 1.5f, row4 - r},
                           {col1 + fontSize * 1.5f, row4 + r}};
    draw2DQuad(lineC, txco, (cell.flags & CELL_NOLINESOUTH) ? line_off : line_on);

    GLfloat lineD[4][2] = {{col1 + 0.f, row3 - r},
                           {col1 + 0.f, row3 + r},
                           {col1 + fontSize * 1.5f, row4 - r},
                           {col1 + fontSize * 1.5f, row4 + r}};
    draw2DQuad(lineD, txco, (cell.flags & CELL_NOLINEWEST) ? line_off : line_on);
  } else if (EditMode::editMode->currentEditMode == EDITMODE_FEATURES) {
    const char* feature = "";
    switch (EditMode::editMode->currentFeature) {
    case FEATURE_SPIKE:
      feature = _("spike");
      break;
    case FEATURE_SMALL_HILL:
      feature = _("small hill");
      break;
    case FEATURE_MEDIUM_HILL:
      feature = _("medium hill");
      break;
    case FEATURE_LARGE_HILL:
      feature = _("large hill");
      break;
    case FEATURE_HUGE_HILL:
      feature = _("huge hill");
      break;
    case FEATURE_SMALL_SMOOTH:
      feature = _("small smooth");
      break;
    case FEATURE_LARGE_SMOOTH:
      feature = _("large smooth");
      break;
    }
    addText_Center(0, fontSize / 2, row1, feature, (col1 + area3x) / 2);
  }

  /* Draw area 3, eg. some extra info */
  snprintf(str, sizeof(str), "%s/%s", EditMode::editMode->pathname,
           EditMode::editMode->levelname);
  addText_Left(0, fontSize / 3, row1, str, area3x + 10);
}
void EStatusWindow::mouseDown(int button, int /*x*/, int /*y*/) {
  int code = getSelectedArea();
  if (code == CODE_INCREMENT) {
    if (button == SDL_BUTTON_LEFT)
      EditMode::editMode->doCommand(EDIT_RAISE_INCREMENT);
    else
      EditMode::editMode->doCommand(EDIT_LOWER_INCREMENT);
  }
  if (code == CODE_EDITMODE)
    EditMode::editMode->currentEditMode =
        (EditMode::editMode->currentEditMode +
         ((button == SDL_BUTTON_LEFT) ? 1 : N_EDITMODES - 1)) %
        N_EDITMODES;

  if (CODE_TO_MENUENTRY(code) >= 0 && CODE_TO_MENUENTRY(code) <= 170)
    EditMode::editMode->doCommand(CODE_TO_MENUENTRY(code));
}

/***********************************
            QuitWindow
***********************************/

EQuitWindow::EQuitWindow() : MyWindow(0, 0, 300, 80) {}
void EQuitWindow::draw() {
  int fontSize = 24;
  this->MyWindow::draw();
  int row1 = y + fontSize + 2;
  int row2 = row1 + fontSize + 2;

  addText_Center(0, fontSize / 2, row1, _("Quit without saving?"), x + width / 2);
  addText_Center(CODE_YES, fontSize / 2, row2, _("Yes"), x + fontSize * 5);
  addText_Center(CODE_NO, fontSize / 2, row2, _("No"), x + width - fontSize * 5);
}
void EQuitWindow::mouseDown(int /*state*/, int /*x*/, int /*y*/) {
  int code = getSelectedArea();
  if (code == CODE_YES)
    yes();
  else if (code == CODE_NO)
    no();
}

void EQuitWindow::yes() {
  remove();
  EditMode::editMode->closeMap();
  Settings::settings->doSpecialLevel = 0;
  GameMode::activate(MenuMode::init());
}

void EQuitWindow::no() { remove(); }

/***********************************
            SaveWindow
***********************************/

/* The private variable saveCnt is used to delay the save action
   until we have drawing the text "saving..." on the screen. The actual
   saving will be done during the drawing cycle when saveCnt reaches 1 */
ESaveWindow::ESaveWindow() : MyWindow(0, 0, 400, 80) { saveCnt = 0; }
void ESaveWindow::draw() {
  int fontSize = 24;
  this->MyWindow::draw();
  int row1 = y + fontSize + 2;
  int row2 = row1 + fontSize + 2;
  char str[512];

  if (!saveCnt) {
    addText_Center(0, fontSize / 2, row1, _("Save map?"), x + width / 2);
    addText_Center(CODE_YES, fontSize / 2, row2, _("Yes"), x + fontSize * 5);
    addText_Center(CODE_NO, fontSize / 2, row2, _("No"), x + width - fontSize * 5);
  } else if (saveCnt == 2) {
    addText_Center(0, fontSize / 2, row1, _("Saving"), x + width / 2);
    snprintf(str, sizeof(str), "%s/levels/%s", effectiveLocalDir,
             EditMode::editMode->levelname);
    // addText_Center(0,fontSize/2,row2,str,x+width/2);
    addText_Center(0, fontSize / 3, row2, str, x + width / 2);
    saveCnt = 1;
  } else if (saveCnt == 1) {
    struct timespec t0 = getMonotonicTime();
    EditMode::editMode->saveMap();
    remove();
    saveCnt = 0;
    /* Make sure it takes atleast three seconds to save, so user can see the message above
     * properly */
    struct timespec t1;
    do {
      t1 = getMonotonicTime();
      SDL_Delay(10);
    } while (getTimeDifference(t0, t1) < 3.0);
  }
}
void ESaveWindow::mouseDown(int /*state*/, int /*x*/, int /*y*/) {
  int code = getSelectedArea();
  if (code == CODE_YES)
    yes();
  else if (code == CODE_NO)
    no();
}

void ESaveWindow::yes() { saveCnt = 2; }

void ESaveWindow::no() { remove(); }

/***********************************
            CloseWindow
***********************************/

ECloseWindow::ECloseWindow() : MyWindow(0, 0, 300, 80) {}
void ECloseWindow::draw() {
  int fontSize = 24;
  this->MyWindow::draw();
  int row1 = y + fontSize + 2;
  int row2 = row1 + fontSize + 2;

  addText_Center(0, fontSize / 2, row1, _("Close without saving?"), x + width / 2);
  addText_Center(CODE_YES, fontSize / 2, row2, _("Yes"), x + fontSize * 5);
  addText_Center(CODE_NO, fontSize / 2, row2, _("No"), x + width - fontSize * 5);
}
void ECloseWindow::mouseDown(int /*state*/, int /*x*/, int /*y*/) {
  int code = getSelectedArea();
  if (code == CODE_YES)
    yes();
  else if (code == CODE_NO)
    no();
}

void ECloseWindow::yes() {
  remove();
  EditMode::editMode->closeMap();
}

void ECloseWindow::no() { remove(); }

/***********************************
            NewWindow
***********************************/

ENewWindow::ENewWindow() : MyWindow(0, 0, 300, 100) { name[0] = 0; }
void ENewWindow::draw() {
  int fontSize = 24;
  this->MyWindow::draw();
  int row1 = y + fontSize + 2;
  int row2 = row1 + fontSize + 2;
  int row3 = row1 + fontSize * 2 + 2;

  addText_Center(0, fontSize / 2, row1, _("Name of map"), x + width / 2);
  addText_Center(0, fontSize / 2, row2, name, x + width / 2);
  addText_Left(CODE_CANCEL, fontSize / 2, row3, _("Cancel"), x + fontSize / 2 + 2 + 10);
  addText_Right(CODE_OK, fontSize / 2, row3, _("Open"), x + width - fontSize / 2 - 2);
}
void ENewWindow::mouseDown(int /*state*/, int /*x*/, int /*y*/) {
  int code = getSelectedArea();
  if (code == CODE_CANCEL)
    remove();
  else if (code == CODE_OK) {
    EditMode::editMode->loadMap(name);
    remove();
  }
}
void ENewWindow::key(int key, int shift, int /*x*/, int /*y*/) {
  if (key == SDLK_BACKSPACE) {
    int len = strlen(name);
    if (len > 0) name[len - 1] = 0;
  } else if ((key < 128 && isalnum(key)) || key == ' ' || key == '_' || key == '-') {
    if (shift && islower(key)) key = toupper(key);
    int len = strlen(name);
    if (len >= 255) return;
    name[len++] = key;
    name[len] = 0;
  }
}

/***********************************
            OpenWindow
***********************************/
#define PAGE_SIZE 8

EOpenWindow::EOpenWindow() : MyWindow(0, 0, 400, 300) {
  nNames = 0;
  currPage = 0;
  memset(names, 0, sizeof(names));
}
void EOpenWindow::draw() {
  int fontSize = 24;
  this->MyWindow::draw();

  int nPages = (nNames + PAGE_SIZE - 1) / PAGE_SIZE;
  int row1 = y + fontSize + 2;
  int rowN = y + 300 - fontSize;
  int col0 = x + 2 + fontSize / 2;
  int colN = x + 400 - fontSize / 2;
  char str[256];

  addText_Center(0, fontSize / 2, row1, _("Open map"), x + width / 2);
  for (int i = 0; i < PAGE_SIZE; i++) {
    int n = i + currPage * PAGE_SIZE;
    if (n < nNames)
      addText_Left(CODE_MAP0 + n, fontSize / 2, row1 + fontSize * (i + 1), names[n], col0);
  }

  snprintf(str, sizeof(str), _("page %d / %d"), currPage + 1, nPages);
  addText_Left(CODE_PAGE, fontSize / 2, rowN, str, col0);
  addText_Right(CODE_CANCEL, fontSize / 2, rowN, _("Cancel"), colN);
}
void EOpenWindow::mouseDown(int button, int /*x*/, int /*y*/) {
  int code = getSelectedArea();
  int nPages = (nNames + PAGE_SIZE - 1) / PAGE_SIZE;

  if (code >= CODE_MAP0 && code <= CODE_MAP0 + nNames) {
    /* A map has been selected */
    int mapNumber = code - CODE_MAP0;
    EditMode::editMode->loadMap(names[mapNumber]);
    remove();
  } else
    switch (code) {
    case CODE_CANCEL:
      remove();
      break;
    case CODE_PAGE:
      if (button == SDL_BUTTON_LEFT)
        currPage = (currPage + 1) % nPages;
      else
        currPage = (currPage + nPages - 1) % nPages;
      break;
    }
}

int sortstrcmp(const void* n1, const void* n2) { return strcmp((char*)n1, (char*)n2); }

void EOpenWindow::refreshMapList() {
  char str[512];
  struct dirent* dirent;
  DIR* dir;

  nNames = 0;
  currPage = 0;

  /* Add all maps from the home directory */
  snprintf(str, sizeof(str), "%s/levels", effectiveLocalDir);
  dir = opendir(str);
  if (dir) {
    /* This iteratives over all files there */
    while ((dirent = readdir(dir))) {
      /* And adds the ones ending with .map */
      if (strlen(dirent->d_name) > 4 &&
          (strcmp(&dirent->d_name[strlen(dirent->d_name) - 4], ".map") == 0)) {
        strncpy(str, dirent->d_name, sizeof(str));
        str[strlen(str) - 4] = 0; /* Remove .map ending */
        strncpy(names[nNames++], str, 256);
      }
    }
    closedir(dir);
  }
  qsort(names, nNames, sizeof(char[256]), sortstrcmp);
  int divNames = nNames;

  /* Add all maps from the share directory */
  snprintf(str, sizeof(str), "%s/levels", effectiveShareDir);
  dir = opendir(str);
  if (dir) {
    /* This iteratives over all files there */
    while ((dirent = readdir(dir))) {
      /* And adds the ones ending with .map *but* not if they already where added from the home
       * directory */
      if (strlen(dirent->d_name) > 4 &&
          (strcmp(&dirent->d_name[strlen(dirent->d_name) - 4], ".map") == 0)) {
        strncpy(str, dirent->d_name, sizeof(str));
        str[strlen(str) - 4] = 0; /* Remove .map ending */
        int i;
        for (i = 0; i < nNames; i++)
          if (strncasecmp(str, names[i], 256) == 0) break;
        if (i == nNames) strncpy(names[nNames++], str, 256);
      }
    }
    closedir(dir);
  }
  qsort(&names[divNames][0], nNames - divNames, sizeof(char[256]), sortstrcmp);
}

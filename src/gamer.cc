/* gamer.cc
   Represents a gamer

   Copyright (C) 2000  Mathias Broxvall

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

#include "gamer.h"
#include "game.h"
#include "general.h"
#include "guile.h"
#include "map.h"
#include "player.h"
#include "settings.h"

#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <zlib.h>
#include <cstdlib>

Gamer *Gamer::gamer = NULL;

Gamer::Gamer() {
  memset(name, 0, sizeof(name));
  strncpy(name, _("John Doe"), sizeof(name));
  name[19] = '\0';
  if (username[0]) { snprintf(name, sizeof(name) - 1, "%s", username); }

  memset(levels, 0, sizeof(levels));
  for (int i = 0; i < Settings::settings->nLevelSets; i++) levels[i] = new KnownLevel[256];

  setDefaults();
  update();
  reloadNames();
  if (nNames > 0) strncpy(name, names[0], 20);
  name[19] = '\0';
  update();
  currentLevelSet = 0;
}
Gamer::~Gamer() {
  for (int i = 0; i < 256; i++)
    if (levels[i]) delete[] levels[i];
}

void Gamer::setDefaults() {
  color = 0;
  totalScore = 0;
  timesPlayed = 0;
  nLevelsCompleted = 0;
  for (int i = 0; i < Settings::settings->nLevelSets; i++) {
    nKnownLevels[i] = 1;
    strncpy(levels[i][0].name, Settings::settings->levelSets[i].startLevelName,
            sizeof(levels[i][0].name));
    strncpy(levels[i][0].fileName, Settings::settings->levelSets[i].startLevel,
            sizeof(levels[i][0].fileName));
  }
  textureNum = 0;
}
void Gamer::levelStarted(Game *game) {
  int i;
  char *level = game->levelName;
  if (game->currentLevelSet < 0) return;  // don't modify profile when cheating
  if (game->map->isBonus) return;         // bonus levels are not added to known levels
  for (i = 0; i < nKnownLevels[game->currentLevelSet]; i++)
    if (strcmp(levels[game->currentLevelSet][i].fileName, level) == 0) break;
  if (i == nKnownLevels[game->currentLevelSet]) {
    strncpy(levels[game->currentLevelSet][i].fileName, level, 64);
    strncpy(levels[game->currentLevelSet][i].name, game->map->mapname, 64);
    nKnownLevels[game->currentLevelSet]++;
    nLevelsCompleted++;
    save();
  } else if (strncmp(levels[game->currentLevelSet][i].name, game->map->mapname, 64) != 0) {
    /* level name change, i.e. due to translation or order change. */
    strncpy(levels[game->currentLevelSet][i].fileName, level, 64);
    strncpy(levels[game->currentLevelSet][i].name, game->map->mapname, 64);
    save();
  }
}

/* Note. save/update need not use a platform independent format since the save
   files are meant to be local. */
void Gamer::save() {
  char str[256];

  Settings *settings = Settings::settings;
  snprintf(str, sizeof(str) - 1, "%s/%s.gmr", effectiveLocalDir, name);
  if (pathIsLink(str)) {
    warning("Error, %s/%s.gmr is a symbolic link. Cannot save settings", effectiveLocalDir,
            name);
    return;
  }

  gzFile gp = gzopen(str, "wb9");
  if (gp) {
    gzprintf(gp, "(color %d)\n", color);
    gzprintf(gp, "(texture %d)\n", textureNum);
    gzprintf(gp, "(total-score %d)\n", totalScore);
    gzprintf(gp, "(levels-completed %d)\n", nLevelsCompleted);
    gzprintf(gp, "(times-played %d)\n", timesPlayed);
    gzprintf(gp, "(difficulty %d)\n", Settings::settings->difficulty);
    gzprintf(gp, "(sandbox %d)\n", Settings::settings->sandbox);
    gzprintf(gp, "(levelsets %d\n", Settings::settings->nLevelSets);
    for (int levelSet = 0; levelSet < Settings::settings->nLevelSets; levelSet++) {
      char *name = ascm_format(settings->levelSets[levelSet].path);
      gzprintf(gp, "  (%s %d\n", name, nKnownLevels[levelSet]);
      free(name);
      for (int i = 0; i < nKnownLevels[levelSet]; i++) {
        char *fname = ascm_format(&levels[levelSet][i].fileName[0]);
        char *tname = ascm_format(&levels[levelSet][i].name[0]);
        gzprintf(gp, "    (%s %s)\n", fname, tname);
        free(fname);
        free(tname);
      }
      gzprintf(gp, "  )\n");
    }
    gzprintf(gp, ")\n");
    gzclose(gp);
  }
}
void Gamer::update() {
  char str[256];

  snprintf(str, sizeof(str) - 1, "%s/%s.gmr", effectiveLocalDir, name);
  SCM ip = scm_port_from_gzip(str, 256 * 256 * 128);
  if (SCM_EOF_OBJECT_P(ip)) {
    setDefaults();
    return;
  }
  for (int i = 0; i < 1000; i++) {
    SCM blob = scm_read(ip);
    if (SCM_EOF_OBJECT_P(blob)) { break; }
    if (!scm_to_bool(scm_list_p(blob)) || scm_to_int(scm_length(blob)) < 2 ||
        !scm_is_symbol(SCM_CAR(blob))) {
      warning("Profile format error for player %s", name);
      break;
    }
    char *skey = scm_to_utf8_string(scm_symbol_to_string(SCM_CAR(blob)));
    if (!strcmp(skey, "levelsets")) {
      free(skey);
      if (!scm_is_integer(SCM_CADR(blob))) {
        warning("Profile format error for player %s", name);
        break;
      }
      int nLevelSets = scm_to_int32(SCM_CADR(blob));
      if (scm_to_int(scm_length(blob)) != nLevelSets + 2) {
        warning("Profile format error for player %s", name);
        break;
      }

      for (; nLevelSets; nLevelSets--) {
        SCM block = scm_list_ref(blob, scm_from_int32(nLevelSets + 1));
        if (!scm_to_bool(scm_list_p(block)) || !scm_is_string(SCM_CAR(block)) ||
            !scm_is_integer(SCM_CADR(block)) || scm_to_int32(SCM_CADR(block)) <= 0) {
          warning("Profile format error for player %s", name);
          break;
        }
        char *lsname = scm_to_utf8_string(SCM_CAR(block));
        int levelSet;
        for (levelSet = 0; levelSet < Settings::settings->nLevelSets; levelSet++)
          if (strcmp(lsname, Settings::settings->levelSets[levelSet].path) == 0) break;
        if (levelSet == Settings::settings->nLevelSets) {
          warning("Profile %s contains info for unknown levelset %s", str, lsname);
          free(lsname);
          break;
        }
        free(lsname);
        nKnownLevels[levelSet] = scm_to_int32(SCM_CADR(block));
        for (int i = 0; i < nKnownLevels[levelSet]; i++) {
          SCM cell = scm_list_ref(block, scm_from_int32(i + 2));
          if (!scm_to_bool(scm_list_p(cell)) || scm_to_int(scm_length(cell)) != 2 ||
              !scm_is_string(SCM_CAR(cell)) || !scm_is_string(SCM_CADR(cell)) ||
              scm_to_int32(scm_string_length(SCM_CAR(cell))) >= 64 ||
              scm_to_int32(scm_string_length(SCM_CADR(cell))) >= 64) {
            warning("Profile format error for player %s", name);
            break;
          }
          char *fname = scm_to_utf8_string(SCM_CAR(cell));
          char *tname = scm_to_utf8_string(SCM_CADR(cell));
          strncpy(&levels[levelSet][i].fileName[0], fname, 64);
          strncpy(&levels[levelSet][i].name[0], tname, 64);
          free(fname);
          free(tname);
        }
      }
    } else {
      const char *keys[7] = {"color",        "texture",    "total-score", "levels-completed",
                             "times-played", "difficulty", "sandbox"};
      int *dests[7] = {&color,
                       &textureNum,
                       &totalScore,
                       &nLevelsCompleted,
                       &timesPlayed,
                       &Settings::settings->difficulty,
                       &Settings::settings->sandbox};
      if (scm_to_int(scm_length(blob)) != 2 || !scm_is_integer(SCM_CADR(blob))) {
        warning("Profile format error for player %s", name);
        free(skey);
        break;
      }
      int val = scm_to_int32(SCM_CADR(blob));
      for (int i = 0; i < 7; i++) {
        if (!strcmp(skey, keys[i])) { *dests[i] = val; }
      }
      free(skey);
    }
  }
  scm_close_input_port(ip);
  return;
}
void Gamer::playerLose(Game *game) {
  timesPlayed++;
  totalScore += game->player1->score;
  save();
}
void Gamer::reloadNames() {
  nNames = 0;
  DIR *dir = opendir(effectiveLocalDir);
  if (dir) {
    struct dirent *dirent;
    while ((dirent = readdir(dir))) {
      if (strlen(dirent->d_name) > 4 &&
          strcmp(&dirent->d_name[strlen(dirent->d_name) - 4], ".gmr") == 0) {
        int len = strlen(dirent->d_name);
        for (int i = 0; i < len - 4; i++) names[nNames][i] = dirent->d_name[i];
        names[nNames][len - 4] = 0;
        nNames++;
      }
    }
    closedir(dir);
  }
}

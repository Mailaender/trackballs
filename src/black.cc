/** \file black.cc
   Represents the black hostile balls in the game
*/
/*
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

#include "black.h"
#include "debris.h"
#include "game.h"
#include "map.h"
#include "player.h"
#include "sound.h"

Black::Black(Game& g, Real x, Real y) : Ball(g, Role_Ball) {
  realRadius = 0.4;
  radius = realRadius;
  ballResolution = BALL_HIRES;
  position[0] = x;
  position[1] = y;
  position[2] = game.map->getHeight(position[0], position[1]) + radius;
  crashTolerance = 7;

  /* Set color to black */
  primaryColor = Color(0.05f, 0.05f, 0.2f, 1.f);
  specularColor = Color(0.5f, 0.5f, 0.5f, 1.f);

  bounceFactor = .8;
  horizon = 5.0;
  likesPlayer = 1;

  setReflectivity(0.4, 0);

  scoreOnDeath = Game::defaultScores[SCORE_BLACK][0];
  timeOnDeath = Game::defaultScores[SCORE_BLACK][1];
}
void Black::die(int how) {
  Ball::die(how);

  Coord3d pos, vel;
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++) {
      Real a = i / 4.0 * M_PI2;
      Real b = (j + 0.5) / 4.0 * M_PI;
      pos[0] = position[0] + std::cos(a) * 0.25 * std::sin(b);
      pos[1] = position[1] + std::sin(a) * 0.25 * std::sin(b);
      pos[2] = position[2] + 0.25 * std::cos(b) + 0.5;
      vel[0] = velocity[0] + (game.frandom() - 0.5);
      vel[1] = velocity[1] + (game.frandom() - 0.5);
      vel[2] = velocity[2] + (game.frandom() - 0.5);
      game.add(new Debris(game, this, pos, vel, 2.0 + 8.0 * game.frandom()));
    }

  remove();
  if (how == DIE_CRASH)
    playEffect(SFX_BLACK_DIE);
  else if (how == DIE_FF)
    playEffect(SFX_FF_DEATH);
}
void Black::tick(Real t) {
  if (game.player1->playing && is_on) {
    Coord3d v = game.player1->position - position;
    double dist = length(v);

    double d =
        game.map->getHeight(position[0] + velocity[0] * 1.0, position[1] + velocity[1] * 1.0);
    Cell& c2 = game.map->cell((int)(position[0] + velocity[0] * 1.0),
                              (int)(position[1] + velocity[1] * 1.0));

    bool expect_fall = d < position[2] - 1.0 && !modTimeLeft[MOD_FLOAT];
    bool death_cell = c2.flags & CELL_ACID || c2.flags & CELL_KILL;

    /* TODO. Make these checks better */
    if ((expect_fall || death_cell) && !inPipe) {
      /* Stop, we are near an edge or acid or ... */
      v[0] = velocity[0];
      v[1] = velocity[1];
      v[2] = 0.0;
      double vsc = length(v) > 0 ? 1. / length(v) : 0.;
      v = v * vsc;
      Ball::drive(-v[0], -v[1]);
    } else if (dist < horizon) {
      /* Go toward the player */
      double vsc = length(v) > 0 ? 1. / length(v) : 0.;
      v = v * vsc;
      Ball::drive(v[0] * likesPlayer, v[1] * likesPlayer);
    } else {
      Ball::drive(0., 0.);
    }
  } else {
    Ball::drive(0., 0.);
  }

  Ball::tick(t);
}
Black::~Black() {}

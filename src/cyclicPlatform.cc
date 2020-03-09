/* cyclicPlatform.h
   A cyclicPlatform modifies the map so that a (flat) rectangle of it
   goes up and down cyclicaly with a specified frequency

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

#include "cyclicPlatform.h"

#include "animatedCollection.h"
#include "ball.h"
#include "game.h"
#include "map.h"

Real timeLow = 2.0, timeRise = 3.0, timeHigh = 2.0, timeFall = 3.0;

CyclicPlatform::CyclicPlatform(Game &g, int x1, int y1, int x2, int y2, Real low, Real high,
                               Real offset, Real speed)
    : GameHook(g, Role_GameHook) {
  this->x1 = std::min(x1, x2);
  this->x2 = std::max(x1, x2);
  this->y1 = std::min(y1, y2);
  this->y2 = std::max(y1, y2);
  this->low = low;
  this->high = high;
  phase = offset;
  this->speed = speed;
}

void CyclicPlatform::tick(Real dt) {
  GameHook::tick(dt);

  if (is_on) phase += dt / speed;
  Real t = std::fmod(phase, 10.0);
  /*
  Real t = Game::current->gameTime + offset;
  t -= ((int)(t/cycleTime)) * cycleTime;*/
  Map *map = game.map;
  if (!map) return;

  Real h, oldHeight = map->cell(x1, y1).heights[0];

  if (t < timeLow)
    h = low;
  else if (t < timeLow + timeRise)
    h = low + (high - low) * (t - timeLow) / timeRise;
  else if (t < timeLow + timeRise + timeHigh)
    h = high;
  else
    h = high + (low - high) * (t - timeLow - timeRise - timeHigh) / timeFall;

  for (int x = x1; x <= x2; x++)
    for (int y = y1; y <= y2; y++) {
      Cell &c = map->cell(x, y);
      for (int j = 0; j < 5; j++) c.heights[j] = h;
    }
  map->markCellsUpdated(x1, y1, x2, y2, true);

  if (h < oldHeight) {
    double lower[3] = {(double)x1, (double)y1, oldHeight - 1.0};
    double upper[3] = {(double)x2, (double)y2, oldHeight + 0.1};

    Animated **balls;
    int nballs = game.balls->bboxOverlapsWith(lower, upper, &balls);
    for (int i = 0; i < nballs; i++) {
      Ball *ball = (Ball *)balls[i];
      if (ball->no_physics) continue;

      if (ball->position[0] >= x1 && ball->position[0] < x2 + 1.0 && ball->position[1] >= y1 &&
          ball->position[1] < y2 + 1.0 &&
          ball->position[2] - ball->radius <=
              oldHeight +
                  0.1)  //		 ball->position[2] - oldHeight < ball->radius + 0.02)
        ball->position[2] += h - oldHeight;
    }
  }
}

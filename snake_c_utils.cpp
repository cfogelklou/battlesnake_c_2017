#include "snake_c_utils.h"
#include <assert.h>
#include <string.h>
#include <iostream>
extern "C" {

// ----------------------------------------------------------------------------
// See header file for description.
const char * SnakeHeadStr(const SnakeHeadTypeE head) {
  switch (head) {
  case SH_BENDR: return "bendr";
  case SH_DEAD: return "dead";
  case SH_FANG: return "fang";
  case SH_PIXEL: return "pixel";
  case SH_REGULAR: return "regular";
  case SH_SAFE: return "safe";
  case SH_SAND_WORM: return "sand-worm";
  case SH_SHADES: return "shades";
  case SH_SMILE: return "smile";
  default: return "tongue";
  }
}

// ----------------------------------------------------------------------------
// See header file for description.
const char * SnakeTailStr(const SnakeTailTypeE tail) {
  switch (tail) {
  case ST_SMALL_RATTLE: return "small-rattle";
  case ST_SKINNY_TAIL: return "skinny-tail";
  case ST_ROUND_BUM: return "round-bum";
  case ST_REGULAR: return "regular";
  case ST_PIXEL: return "pixel";
  case ST_FRECKLED: return "freckled";
  case ST_FAT_RATTLE: return "fat-rattle";
  case ST_CURLED: return "curled";
  default: return "block-bum";
  }
}

// ----------------------------------------------------------------------------
// See header file for description.
const char * SnakeDirStr(const SnakeDirectionE dir) {
  switch (dir) {
  case DIR_UP: return "up";
  case DIR_DOWN: return "down";
  case DIR_LEFT: return "left";
  default: return "right";
  };
}

// ----------------------------------------------------------------------------
// See header file for description.
void SnakeDoMove(MoveOutput *const pMoveOut, const SnakeDirectionE dir, const char * const taunt) {
  assert(pMoveOut);
  pMoveOut->dir = dir;
  strncpy(pMoveOut->taunt, taunt, SNAKE_STRLEN);
}

// See header file for description.
bool SnakeBattlefieldIsAllowedMove(const Battlefield * const pBattlefield,
  const int x,
  const int y
) {
  const int width = pBattlefield->width;
  const int height = pBattlefield->height;
  const char * const battlefield = pBattlefield->battlefieldArr;

  if ((x < 0) || (y < 0) || (x >= width) || (y >= height)) {
    std::cout << "outside" << std::endl;
    return false;
  }
  char pt = battlefield[x + y*width];
  return ('#' == pt) || ('*' == pt) || ('.' == pt); // Food or clear field
}

// ----------------------------------------------------------------------------
// See header file for description.
Battlefield *SnakeBattlefieldAlloc(const int width, const int height) {
  Battlefield * const pField = (Battlefield *)malloc(sizeof(Battlefield) + (width*height));
  pField->width = width;
  pField->height = height;
  return pField;
}

// ----------------------------------------------------------------------------
// See header file for description.
Battlefield *SnakeBattlefieldAllocAndUpdate(
  const MoveInput * const pMoveInput) {
  Battlefield * const pB = SnakeBattlefieldAlloc(pMoveInput->width, pMoveInput->height);
  SnakeBattlefieldUpdate(pB, pMoveInput);
  return pB;
}

// ----------------------------------------------------------------------------
// See header file for description.
void SnakeBattlefieldFree(Battlefield * const pBattlefield) {
  free(pBattlefield);
}

// ----------------------------------------------------------------------------
// See header file for description.
void SnakeBattlefieldUpdate(Battlefield * const pBattlefield, const MoveInput * const pMoveInput) {
  const int width = pBattlefield->width;
  const int height = pBattlefield->height;
  assert(width == pMoveInput->width);
  assert(height == pMoveInput->height);

  char * const battlefield = pBattlefield->battlefieldArr;
  memset(battlefield, '.', pBattlefield->width*pBattlefield->height);
  // Draw alive snakes (lower case letters)
  int c = 'a';
  for (int s = 0; s < pMoveInput->numSnakes; s++) {
    Snake &snake = pMoveInput->snakesArr[s];
    std::cout << "snake: " << snake.id << " " << snake.name << std::endl;
    //for (auto& pt : snake.coords) {
    for (int p = 0; p < snake.numCoords; p++) {
      Coords &pt = snake.coordsArr[p];
      battlefield[pt.x + pt.y*width] = (0 == p) ? (c - 'a' + 'A') : c;
    }
    c++;
  }

  // Draw food
  for (int fIdx = 0; fIdx < pMoveInput->numFood; fIdx++) {
    Coords &food = pMoveInput->foodArr[fIdx];
    battlefield[food.x + food.y * width] = '*';
  }
}

// ----------------------------------------------------------------------------
// See header file for description.
void SnakeBattlefieldPrint(const Battlefield * const pBattlefield) {
  const int width = pBattlefield->width;
  const int height = pBattlefield->height;
  const char * const battlefield = pBattlefield->battlefieldArr;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      std::cout << battlefield[x + y*width];
    }
    std::cout << std::endl;
  }

  //std::cout << you << " -------------------------------------------------------" << endl;
  std::cout << " -------------------------------------------------------" << std::endl;

}

} // extern "C"

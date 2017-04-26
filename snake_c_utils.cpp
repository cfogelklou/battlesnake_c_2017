#include "snake_c_utils.h"
#include <assert.h>
#include <string.h>
extern "C" {

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

// See header file for description.
const char * SnakeDirStr(const SnakeDirectionE dir) {
  switch (dir) {
  case UP: return "up";
  case DOWN: return "down";
  case LEFT: return "left";
  default: return "right";
  };
}

// See header file for description.
void SnakeDoMove(MoveOutput *const pMoveOut, const SnakeDirectionE dir, const char * const taunt) {
  assert(pMoveOut);
  pMoveOut->dir = dir;
  strncpy(pMoveOut->taunt, taunt, SNAKE_STRLEN);
}

}

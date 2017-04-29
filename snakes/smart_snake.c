#include "smart_snake.h"
#include "snake_c_utils.h"
#include <stddef.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef ABS
#define ABS(x) (((x) < 0) ? 0-x : x)
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ////////////////////////////////////////////////////////////////////////////
// Callback called when the game starts.
static void snake_start(
  void * const pUserData,
  const char * const pGameId,
  const int width,
  const int height,
  StartOutputT * const pStartOutput
) {
  printf("Started game %s with width %d and height %d!\r\n", pGameId, width, height);

  // Fill in the snake info
  strncpy(pStartOutput->color, "red", SNAKE_STRLEN);
  strncpy(pStartOutput->secondary_color, "white", SNAKE_STRLEN);
  strncpy(pStartOutput->name, "Smarty C McSmarterface", SNAKE_STRLEN);
  strncpy(pStartOutput->taunt, "I'm a gonna getcha!", SNAKE_STRLEN);

  pStartOutput->head_type = SH_FANG;
  pStartOutput->tail_type = ST_SMALL_RATTLE;

}

// ////////////////////////////////////////////////////////////////////////////
// Callback called when it's time to make a new move.
static void snake_move(
  void * const pUserData,
  const char * const pGameId,
  const MoveInput * const pMoveInput,
  MoveOutput * const pMoveOutput) {
  int heading = -1;

  printf("Got move for game %s!\r\n", pGameId);

  if (pMoveInput->numFood <= 0) {
    SnakeDoMove(pMoveOutput, DIR_DOWN, "No food!  Let's go DOOOWN!");
    return;
  }

  const int width = pMoveInput->width;
  const int height = pMoveInput->height;

  Snake * const pMe = &pMoveInput->snakesArr[pMoveInput->yourSnakeIdx];
  const Coords myHead = pMe->coordsArr[0];

  // Print the current battlefield.
  Battlefield * const pB = SnakeBattlefieldAllocAndUpdate(pMoveInput);
  SnakeBattlefieldPrint(pB);

  // Assuming 'you' is the index of my snake
  char * const battlefield = pB->battlefieldArr;

  // Find closest food
  Coords closestFood;
  double minDistance = sqrt(height * height + width * width) + 1;
  for (int fI = 0; fI < pMoveInput->numFood; fI++) {
    Coords f = pMoveInput->foodArr[fI];
    int a = f.x - myHead.x;
    int b = f.y - myHead.y;
    double distance = sqrt(a*a + b*b);
    if (distance < minDistance) {
      closestFood = f;
      minDistance = distance;
    }
  }
  battlefield[closestFood.x + closestFood.y * width] = '#';
  printf("my pos: [%d,%d] food:[%d,%d]\r\n", myHead.x, myHead.y, closestFood.x, closestFood.y);

  //set<Direction> allowedMoves;
  const bool d = SnakeBattlefieldIsAllowedMove(pB, myHead.x, myHead.y + 1);
  const bool u = SnakeBattlefieldIsAllowedMove(pB, myHead.x, myHead.y - 1);
  const bool r = SnakeBattlefieldIsAllowedMove(pB, myHead.x + 1, myHead.y);
  const bool l = SnakeBattlefieldIsAllowedMove(pB, myHead.x - 1, myHead.y);

  // Direction to closest food
  {
    int a = closestFood.x - myHead.x;
    int b = closestFood.y - myHead.y;

    //if (abs(a) > abs(b) && (allowedMoves.count(Direction::right) || allowedMoves.count(Direction::left))) {
    if ((ABS(a) > ABS(b)) && (r || l)) {
      // Try to close in on x axis
      if (closestFood.x > myHead.x && (r)) {
        heading = DIR_RIGHT;
      }
      else if (l) {
        heading = DIR_LEFT;
      }
    }

    if (heading < 0) {
      // Try to close in on y axis
      //if (closestFood.y > myHead.y && allowedMoves.count(Direction::down)) {
      if ((closestFood.y > myHead.y) && (d)) {
        heading = DIR_DOWN;
      }
      //else if (allowedMoves.count(Direction::up)) {
      else if (u) {
        heading = DIR_UP;
      }
    }
    
    if (heading < 0) {
      printf("undecided\r\n");
      bool found = false;
      if (u) {
        heading = DIR_UP;
      }
      else if (d) {
        heading = DIR_DOWN;
      }
      else if (r) {
        heading = DIR_RIGHT;
      }
      else {
        heading = DIR_LEFT;
      }
    }
  }
  SnakeBattlefieldFree(pB);

  SnakeDoMove(pMoveOutput, heading, "Kill!");
}

const SnakeCallbacks smart_snake = {
  snake_start,
  snake_move
};

#ifdef __cplusplus
}
#endif

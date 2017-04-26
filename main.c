// battlesnake_c.cpp : Defines the entry point for the console application.
//
#include "snake_c_api.h"
#include <stdio.h>
#include <string.h>

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
  strncpy(pStartOutput->color, "white", SNAKE_STRLEN);
  strncpy(pStartOutput->secondary_color, "red", SNAKE_STRLEN);
  strncpy(pStartOutput->name,  "Dorky C McDorkerface", SNAKE_STRLEN);
  strncpy(pStartOutput->taunt, "I'm a gonna getcha!", SNAKE_STRLEN);

  pStartOutput->head_type = SH_TONGUE;
  pStartOutput->tail_type = ST_FRECKLED;

}

// ////////////////////////////////////////////////////////////////////////////
// Callback called when it's time to make a new move.
static void snake_move(
  void * const pUserData,
  const char * const pGameId,
  const MoveInput * const pMoveInput, 
  MoveOutput * const pMoveOutput) {

  printf("Got move for game %s!\r\n", pGameId);

  if (pMoveInput->numFood <= 0) {
    SnakeDoMove(pMoveOutput, DOWN, "No food!  Let's go DOOOWN!");
  }
  else {

    Snake * const pMe = &pMoveInput->snakesArr[pMoveInput->yourSnakeIdx];
    Coords * const pMyHead = &pMe->coordsArr[0];
    Coords * const pFirstFood = &pMoveInput->foodArr[0];

    printf("my pos: [%d,%d] food:[%d,%d]\r\n", 
      pMyHead->x, pMyHead->y, pFirstFood->x, pFirstFood->y );

    if (pFirstFood->y > pMyHead->y) {
      SnakeDoMove(pMoveOutput, DOWN, "Watch out! Going down!!!");
    }
    else if (pFirstFood->y < pMyHead->y) {
      SnakeDoMove(pMoveOutput, UP, "Going up up up!!!");
    }
    else if (pFirstFood->x < pMyHead->x) {
      SnakeDoMove(pMoveOutput, LEFT, "Left we go!!!");
    }
    else {
      SnakeDoMove(pMoveOutput, RIGHT, "Food!!! Yummy!");
    }
  }

}


// ////////////////////////////////////////////////////////////////////////////
int main()
{
  const SnakeCallbacks snakeApi = {
    snake_start,
    snake_move
  };
  
  // This is a blocking call.  If you want multiple 
  // snakes, run multiple threads!
  printf("SnakeStart() listening socket %s...\r\n", DEFAULT_PORT);
  SnakeStart(&snakeApi, DEFAULT_PORT, NULL);

  return 0;
}


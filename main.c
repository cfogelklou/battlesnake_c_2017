// battlesnake_c.cpp : Defines the entry point for the console application.
//
#include "snake_socket.h"
#include <stdio.h>
#include <string.h>

static void snakeStart(
  void * const pUserData,
  const char * const pGameId,
  const int width,
  const int height,
  StartOutputT * const pStartOutput
  ) {
  printf("Started game %s with width %d and height %d!\r\n", pGameId, width, height);

  // Fill in the snake info
  strcpy(pStartOutput->color, "white");
  strcpy(pStartOutput->secondary_color, "red");
  strcpy(pStartOutput->name,  "Dorky C McDorkerface");
  strcpy(pStartOutput->taunt, "I'm a gonna getcha!");

  pStartOutput->head_type = SH_TONGUE;
  pStartOutput->tail_type = ST_FRECKLED;

}

static void snakeMove(
  void * const pUserData,
  const char * const pGameId,
  const MoveInputT * const pMoveInput, 
  MoveOutputT * const pMoveOutput) {
  printf("Got move for game %s!\r\n", pGameId);
  pMoveOutput->dir = LEFT;
  strncpy(pMoveOutput->taunt, "Whee", SNAKE_STRLEN);
}


int main()
{
  SnakeImplementationT snakeApi = {
    snakeStart,
    snakeMove
  };
  
  SnakeStart(&snakeApi, DEFAULT_PORT, NULL);

  return 0;
}


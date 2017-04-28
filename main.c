// battlesnake_c.cpp : Defines the entry point for the console application.
//
#include "snake_c_api.h"
#include "snakes/stupid_snake.h"
#include "snakes/smart_snake.h"
#include "snakes/smarter_snake.h"
#include <stdio.h>
#include <string.h>




// ////////////////////////////////////////////////////////////////////////////
int main(int argv, char **argc)
{
  const SnakeCallbacks *pCallbacks = &stupid_snake;
  if (argv > 1) {
    char *szNum = argc[1];
    switch (szNum[0]) {
    case '1': pCallbacks = &smart_snake;
      break;
    case '2': pCallbacks = &smarter_snake;
      break;
    default: pCallbacks = &stupid_snake;
      break;
    }
  }

  const char * port = DEFAULT_PORT;
  if ((argv > 2) && (strlen(argc[2]) > 0)){
    port = argc[2];
  }

  // This is a blocking call.  If you want multiple snakes, run multiple threads!
  printf("SnakeStart() listening socket %s...\r\n", port);
  SnakeStart(pCallbacks, port, NULL);

  return 0;
}


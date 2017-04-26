// battlesnake_c.cpp : Defines the entry point for the console application.
//
#include "snake_c_api.h"
#include "stupid_snake/stupid_snake.h"
#include <stdio.h>
#include <string.h>




// ////////////////////////////////////////////////////////////////////////////
int main()
{
 
  // This is a blocking call.  If you want multiple snakes, run multiple threads!
  printf("SnakeStart() listening socket %s...\r\n", DEFAULT_PORT);
  SnakeStart(&stupid_snake, DEFAULT_PORT, NULL);

  return 0;
}


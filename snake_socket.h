#ifndef SNAKE_SOCKET
#define SNAKE_SOCKET 1

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif



typedef struct SnakeImplementationTag {
  void(*Start)(void);
} SnakeImplementationT;


void * SnakeStart(SnakeImplementationT * pSnake);


#ifdef __cplusplus
}
#endif



#endif

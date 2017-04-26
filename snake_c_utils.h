#ifndef SNAKE_C_UTILS_H__
#define SNAKE_C_UTILS_H__

#include "snake_c_api.h"

#ifdef __cplusplus
extern "C" {
#endif

// Utility: Get a character string for the head type.
const char * SnakeHeadStr(const SnakeHeadTypeE head);

// Utility: Get a character string for the tail type
const char * SnakeTailStr(const SnakeTailTypeE tail);

// Utility: Get a character string for the direction
const char * SnakeDirStr(const SnakeDirectionE dir);

// Helper function to allow the pMoveOut struct to be set with a single line of code.
void SnakeDoMove(MoveOutput *const pMoveOut, const SnakeDirectionE dir, const char * const taunt);

#ifdef __cplusplus
}
#endif


#endif // SNAKE_C_UTILS_H__

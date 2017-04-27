#ifndef SNAKE_C_UTILS_H__
#define SNAKE_C_UTILS_H__

#include "snake_c_api.h"
#include <stdbool.h>

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

// A battlefield allows printing out of the current state of the field.
typedef struct BattlefieldTag {
  int width;
  int height;
  char battlefieldArr[4];
} Battlefield;

// Allocate a battlefield - can be called from Start() callback.
Battlefield *SnakeBattlefieldAlloc(
  const int width, const int height);

// Allocate and update a battlefield.
Battlefield *SnakeBattlefieldAllocAndUpdate(
  const MoveInput * const pMoveInput);

// Free a battlefield
void SnakeBattlefieldFree(
  Battlefield * const pBattlefield);

// Updates the contents of the battlefield based on pMoveInput.
void SnakeBattlefieldUpdate(
  Battlefield * const pBattlefield,
  const MoveInput * const pMoveInput);

// Prints the battlefield to the terminal.  Ensure you "update" the battlefield first.
void SnakeBattlefieldPrint(
  const Battlefield * const pBattlefield);

bool SnakeBattlefieldIsAllowedMove(
  const Battlefield * const pBattlefield,
  const int destX,
  const int destY
);

#ifdef __cplusplus
}
#endif


#endif // SNAKE_C_UTILS_H__

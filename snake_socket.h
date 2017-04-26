#ifndef SNAKE_SOCKET
#define SNAKE_SOCKET 1

#define SNAKE_STRLEN 80

#define DEFAULT_PORT  "27015"
#define ARRSZ(x) (sizeof(x)/sizeof(x[0]))
#define MIN(x,y) ((x) < (y)) ? (x) : (y)
#define MAX(x,y) ((x) > (y)) ? (x) : (y)

#ifdef __cplusplus
extern "C" {
#endif

typedef enum SnakeHeadTypeTag {
  SH_BENDR = 0,
  SH_DEAD,
  SH_FANG,
  SH_PIXEL,
  SH_REGULAR,
  SH_SAFE,
  SH_SAND_WORM,
  SH_SHADES,
  SH_SMILE,
  SH_TONGUE
} SnakeHeadTypeE;

// Get a character string for the current direction.
const char * SnakeHeadStr(const SnakeHeadTypeE head);

typedef enum SnakeTailTypeTag {
  ST_SMALL_RATTLE,
  ST_SKINNY_TAIL,
  ST_ROUND_BUM,
  ST_REGULAR,
  ST_PIXEL,
  ST_FRECKLED,
  ST_FAT_RATTLE,
  ST_CURLED,
  ST_BLOCK_BUM
} SnakeTailTypeE;

// Get a character string for the current direction.
const char * SnakeTailStr(const SnakeTailTypeE tail);

typedef struct StartOutputTag {
  // Format: "#ff0000, "gold", "rgb(255, 255, 255)", etc..
  char            color[SNAKE_STRLEN + 1];
  
  // Format: "#ff0000, "gold", "rgb(255, 255, 255)", etc..
  char            secondary_color[SNAKE_STRLEN + 1];

  // Name of the snake, null terminated.
  char            name[SNAKE_STRLEN + 1];

  // A starting taunt for the snake, null terminated.
  char            taunt[SNAKE_STRLEN + 1];

  // Choose the head type
  SnakeHeadTypeE  head_type;

  // Choose the tail type
  SnakeTailTypeE  tail_type;

} StartOutputT;

typedef void(*SnakeStartFn)(
  void * const pUserData,
  const char * const pGameId,
  const int width,
  const int height,

  // The Start() function shall fill in this structure.
  StartOutputT * const pStartOutput
);


typedef enum {
  UP = 0,
  LEFT = 1,
  DOWN = 2,
  RIGHT = 3
} SnakeDirectionE;

// Get a character string for the current direction.
const char * SnakeDirStr(const SnakeDirectionE dir);

typedef struct CoordsTag {
  int x;
  int y;
} CoordsT;


typedef struct SnakeInfoTag {
  char      name[SNAKE_STRLEN + 1];
  char      taunt[SNAKE_STRLEN + 1];
  char      id[SNAKE_STRLEN + 1];
  int       healthPercent;
  CoordsT  *coordsArr;
  int       numCoords;
} SnakeInfoT;

typedef struct MoveInputTag {
  int         yourSnakeIdx;
  SnakeInfoT *snakesArr;
  int         numSnakes;
  CoordsT    *foodArr;
  int         numFood;
} MoveInputT;

typedef struct MoveOutputTag {
  SnakeDirectionE dir;
  char            taunt[SNAKE_STRLEN + 1];
} MoveOutputT;

typedef void(*SnakeMoveFn)(
  void * const pUserData,
  const char * const pGameId,
  const MoveInputT * const pMoveInput,

  // Your snake logic needs to update this structure.
  MoveOutputT * const pMoveOutput
);

typedef struct SnakeImplementationTag {

  SnakeStartFn Start;

  SnakeMoveFn Move;

} SnakeImplementationT;




// ////////////////////////////////////////////////////////////////////////////
void SnakeStart(
  SnakeImplementationT * const pSnake, 
  const char * const port,
  void * const pUserData);



#ifdef __cplusplus
}
#endif



#endif

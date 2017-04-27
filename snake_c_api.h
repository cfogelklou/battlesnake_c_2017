#ifndef SNAKE_SOCKET
#define SNAKE_SOCKET 1

#define SNAKE_STRLEN 80

#define DEFAULT_PORT  "8080"
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

// Snake directions
typedef enum {
  DIR_UP = 0,
  DIR_LEFT = 1,
  DIR_DOWN = 2,
  DIR_RIGHT = 3,
  DIR_NUM = 4
} SnakeDirectionE;

typedef struct CoordsTag {
  int x;
  int y;
} Coords;

// A description of each snake
typedef struct SnakeTag {
  char      name[SNAKE_STRLEN + 1]; // This snake's name.
  char      taunt[SNAKE_STRLEN + 1]; // This snake's "taunt"
  char      id[SNAKE_STRLEN + 1]; // The ID of the snake
  int       healthPercent; // Percentage health
  Coords   *coordsArr; //< Array of coordinates in x,y
  int       numCoords; //< Number of coordinates
} Snake;

// Input you get about how the current snake game looks.
typedef struct MoveInputTag {
  int         width; // width of the board
  int         height; // height of the board
  int         yourSnakeIdx; // The index of your snake in snakesArr
  Snake      *snakesArr; // Array of snakes, including yours.
  int         numSnakes; // Number of snakes in snakesArr
  Coords     *foodArr; // Array of available food coordinates
  int         numFood; // Number of food(s??)
} MoveInput;


// Your snake shall set up this struct with its desired direction.
typedef struct MoveOutputTag {

  // Choose a direction for the snake.
  SnakeDirectionE dir;

  // Choose a taunt for the snake.
  char            taunt[SNAKE_STRLEN + 1];
} MoveOutput;

// The move function prototype
typedef void(*SnakeMoveFn)(

  // Data that you passed into the call to SnakeSnart().  You might
  // not need this..
  void * const pUserData,
  
  // The ID of the game that wants you to move.  Not really important.
  const char * const pGameId,
  
  // Data you will need to determine where to move.
  const MoveInput * const pMoveInput,

  // Your snake logic needs to update this structure.
  MoveOutput * const pMoveOutput
);


// The snake "brains" are here, provided by the competitor.
typedef struct SnakeCallbacksTag {

  // Called when the game starts.
  SnakeStartFn  Start;

  // Called when a new move is requested.  
  // Be quick, or you'll miss your chance!
  SnakeMoveFn   Move;

} SnakeCallbacks;



// ////////////////////////////////////////////////////////////////////////////
// This is where the magic happens.
// Call this, and the snake starts waiting for an incoming connection.
void SnakeStart(
  const SnakeCallbacks * const pSnake,
  const char * const port,
  void * const pUserData);



#ifdef __cplusplus
}
#endif



#endif

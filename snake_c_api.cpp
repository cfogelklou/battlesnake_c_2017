/**
* Copyright (c) 2017, Chris Fogelklou
* All rights reserved.
*/

#ifdef STANDALONE_JSON
#include "nlohmann/src/json.hpp"
#else
#include <main/json.hpp>
#endif
#include "snake_c_api.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
/* See http://stackoverflow.com/questions/12765743/getaddrinfo-on-win32 */
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501  /* Windows XP. */
#endif
#include <winsock2.h>
#include <Ws2tcpip.h>
#else
/* Assume that any non-Windows platform uses POSIX-style sockets instead. */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>  /* Needed for getaddrinfo() and freeaddrinfo() */
#include <unistd.h> /* Needed for close() */
typedef int SOCKET;
#endif

#include <iostream>
#include <sstream>
#include <assert.h>

static const int DEFAULT_BUFLEN = 65536;

// ////////////////////////////////////////////////////////////////////////////
static int sockInit(void) {
#ifdef _WIN32
  WSADATA wsa_data;
  return WSAStartup(MAKEWORD(1, 1), &wsa_data);
#else
  return 0;
#endif
}

// ////////////////////////////////////////////////////////////////////////////
static int sockQuit(void) {
#ifdef _WIN32
  return WSACleanup();
#else
  return 0;
#endif
}

// ////////////////////////////////////////////////////////////////////////////
static int sockClose(SOCKET sock) {

  int status = 0;

#ifdef _WIN32
  status = shutdown(sock, SD_BOTH);
  if (status == 0) { status = closesocket(sock); }
#else
  status = shutdown(sock, SHUT_RDWR);
  if (status == 0) { status = close(sock); }
#endif

  return status;
}

// ////////////////////////////////////////////////////////////////////////////
class SnakeSng {
public:
  static SnakeSng &inst() {
    if (NULL == mpInst) {
      mpInst = new SnakeSng();
    }
    return *mpInst;
  }

  SnakeSng() {
    sockInit();
  }

  ~SnakeSng() {
    sockQuit();
  }

private:


  static SnakeSng *mpInst;

};

SnakeSng *SnakeSng::mpInst = NULL;

// ////////////////////////////////////////////////////////////////////////////
class SnakeMove {
public:
  SnakeMove(SnakeCallbacks * pSnake, const char * const port, void * const pUserData);
  ~SnakeMove();

  std::string parseStart(const char * const cbuf);
  std::string parseMove(const char * const cbuf);
  std::string handleReceive(std::string &rxBuf, const int recvbuflen);
  bool nextMove();


private:
  SOCKET ListenSocket = INVALID_SOCKET;

  char recvbuf[DEFAULT_BUFLEN];
  int recvbuflen = DEFAULT_BUFLEN;
  SnakeCallbacks *mpSnake;
  void *mpUserData;

};

extern "C" {

// Get a string for the current direction.
const char * SnakeHeadStr(const SnakeHeadTypeE head) {
  switch (head) {
    case SH_BENDR: return "bendr";
    case SH_DEAD: return "dead";
    case SH_FANG: return "fang";
    case SH_PIXEL: return "pixel";
    case SH_REGULAR: return "regular";
    case SH_SAFE: return "safe";
    case SH_SAND_WORM: return "sand-worm";
    case SH_SHADES: return "shades";
    case SH_SMILE: return "smile";
    default: return "tongue";
  }
}


// Get a string for the current direction.
const char * SnakeTailStr(const SnakeTailTypeE tail) {
  switch (tail) {
    case ST_SMALL_RATTLE: return "small-rattle";
    case ST_SKINNY_TAIL: return "skinny-tail";
    case ST_ROUND_BUM: return "round-bum";
    case ST_REGULAR: return "regular";
    case ST_PIXEL: return "pixel";
    case ST_FRECKLED: return "freckled";
    case ST_FAT_RATTLE: return "fat-rattle";
    case ST_CURLED: return "curled";
    default: return "block-bum";
  }
}

// ////////////////////////////////////////////////////////////////////////////
const char * SnakeDirStr(const SnakeDirectionE dir) {
  switch (dir) {
  case UP: return "up";
  case DOWN: return "down";
  case LEFT: return "left";
  default: return "right";
  };
}

// ////////////////////////////////////////////////////////////////////////////
void SnakeDoMove(MoveOutput *const pMoveOut, const SnakeDirectionE dir, const char * const taunt) {
  assert(pMoveOut);
  pMoveOut->dir = dir;
  strncpy(pMoveOut->taunt, taunt, SNAKE_STRLEN);
}


// ////////////////////////////////////////////////////////////////////////////
void SnakeStart(
  SnakeCallbacks * const pSnake, 
  const char * const port,
  void * const pUserData ) {
  (void)SnakeSng::inst();

  assert(pSnake);
  assert(port);

  SnakeMove snake(pSnake, port, pUserData);

  while (snake.nextMove()) {;}
}


} // extern "C" {




// ////////////////////////////////////////////////////////////////////////////
SnakeMove::SnakeMove(SnakeCallbacks * pSnake, const char * const port, void * const pUserData)
  : mpSnake(pSnake)
  , mpUserData(pUserData)
{
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags = AI_PASSIVE;

  // Resolve the server address and port
  struct addrinfo *result = NULL;
  int iResult = getaddrinfo(NULL, port, &hints, &result);
  if (iResult != 0) {
    std::cerr << "getaddrinfo failed with error: " << iResult << std::endl;
    return;
  }

  // Create a SOCKET for connecting to server
  ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
  if (ListenSocket == INVALID_SOCKET) {
    std::cerr << "socket failed with error" << std::endl;
    freeaddrinfo(result);
    return;
  }

  // Setup the TCP listening socket
  iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
  if (iResult == SOCKET_ERROR) {
    std::cerr << "listen failed with error" << std::endl;
    freeaddrinfo(result);
    sockClose(ListenSocket);
    return;
  }

  freeaddrinfo(result);

  iResult = listen(ListenSocket, SOMAXCONN);
  if (iResult == SOCKET_ERROR) {
    std::cerr << "listen failed with error" << std::endl;
    sockClose(ListenSocket);
    return;
  }
}

// ////////////////////////////////////////////////////////////////////////////
SnakeMove::~SnakeMove() {
  // No longer need server socket
  sockClose(ListenSocket);
}

using namespace nlohmann;

// ////////////////////////////////////////////////////////////////////////////
std::string SnakeMove::parseStart(const char * const cbuf) {

  
  StartOutputT out = {
    "red",
    "white",
    "Default McDefaultyFace",
    "Your mother smells of elderberries!",
    SH_FANG,
    ST_CURLED,
  };

  try {
    json req = json::parse(cbuf);
    if ((mpSnake) && (mpSnake->Start)) {
      const std::string game_id = req["game_id"].get<std::string>();
      auto width = req["width"].get<int>();
      auto height = req["height"].get<int>();
      mpSnake->Start(mpUserData, game_id.c_str(), width, height, &out);
    }
  }
  catch (std::exception& e) {
    std::cerr << "ERROR in /start: " << e.what() << std::endl;
  }
  catch (...) {
    std::cerr << "ERROR in /start: Unknown exception caught." << std::endl;
  }

  /*
  json rval = {
  { "color", "#FF0000" },
  { "secondary_color", "#00FF00" },
  { "name", "Basic snake" },
  { "taunt", "I'm hungry!" },
  { "head_type", "pixel" },
  { "tail_type", "pixel" }
  };
  */

  json rval = json::object();
  if (strlen(out.color) >= 2) {
    rval["color"] = out.color;
  }

  if (strlen(out.secondary_color) >= 2) {
    rval["secondary_color"] = out.secondary_color;
  }

  if (strlen(out.name) >= 1) {
    rval["name"] = out.name;
  }

  if (strlen(out.taunt) >= 1) {
    rval["taunt"] = out.taunt;
  }

  rval["head_type"] = SnakeHeadStr(out.head_type);
  rval["tail_type"] = SnakeTailStr(out.tail_type);

  return rval.dump();
}



// ////////////////////////////////////////////////////////////////////////////
// Convert a json coordinate to a Coords struct
void from_json(const nlohmann::json& jcoord, Coords& p) {
  p.x = jcoord[0].get<int>();
  p.y = jcoord[1].get<int>();
}


// ////////////////////////////////////////////////////////////////////////////
// Convert json coords to coords array
static void jsonArrToCArr(const json jarr, Coords * &pArr, int &numCoords) {
  numCoords = jarr.size();
  if (numCoords > 0) {
    pArr = (Coords *)calloc(numCoords, sizeof(Coords));

    int coordsIdx = 0;
    for (const json coord : jarr) {
      pArr[coordsIdx] = coord.get<Coords>();
      coordsIdx++;
    }
  }
}

void from_json(const nlohmann::json& jsnake, Snake& s) {

  if (jsnake["id"].size() > 0) {
    const std::string id = jsnake["id"].get<std::string>();
    memcpy(s.id, id.c_str(), SNAKE_STRLEN);
  }

  if (jsnake["name"].size() > 0) {
    const std::string name = jsnake["name"].get<std::string>();
    memcpy(s.name, name.c_str(), SNAKE_STRLEN);
  }

  if (jsnake["taunt"].size() > 0) {
    const std::string taunt = jsnake["taunt"].get<std::string>();
    memcpy(s.taunt, taunt.c_str(), SNAKE_STRLEN);
  }

  if (jsnake["health_points"].size() > 0) {
    s.healthPercent = jsnake["health_points"].get<int>();
  }

  if (jsnake["coords"].size() > 0) {
    jsonArrToCArr(jsnake["coords"], s.coordsArr, s.numCoords);
  }
}




// ////////////////////////////////////////////////////////////////////////////
std::string SnakeMove::parseMove(const char * const cbuf) {
  json rval = {
    { "move", "up" },
    { "taunt", "ouch" }
  };
  try {

    MoveInput moveInput = { 0 };
    MoveOutput moveOutput = { UP };

    const json req = json::parse(cbuf);

    if ((req["you"].size() <= 0) && (req["snakes"].size() <= 0)) {
      return rval.dump();
    }

    const std::string game_id = req["game_id"].get<std::string>();
    const std::string you_uuid = req["you"];
    const json snakes = req["snakes"];

#ifdef _DEBUG
    std::cout << "snakes were " << snakes.dump() << std::endl;
    std::cout << std::endl;
#endif

    moveInput.numSnakes = snakes.size();
    moveInput.snakesArr = (Snake *)calloc(moveInput.numSnakes, sizeof(Snake));

    // Convert from json to struct.
    int snakeIdx = 0;
    for (const json jsnake : snakes) {
      Snake &destSnake = moveInput.snakesArr[snakeIdx];
      destSnake = jsnake.get<Snake>();
      if (destSnake.id == you_uuid) {
        moveInput.yourSnakeIdx = snakeIdx;
      }
      snakeIdx++;
    }

    // Convert food to a C array.
    jsonArrToCArr(req["food"], moveInput.foodArr, moveInput.numFood);

    // If the move function is defined, call it.
    if ((mpSnake) && (mpSnake->Move)) {
      mpSnake->Move(mpUserData, game_id.c_str(), &moveInput, &moveOutput);
      
      // Handle output of the move call
      rval["move"] = SnakeDirStr(moveOutput.dir);
      if (strlen(moveOutput.taunt) >= 1) {
        rval["taunt"] = moveOutput.taunt;
      }
    }

    // Free allocated food.
    if (moveInput.foodArr) {
      free(moveInput.foodArr);
    }

    // Free the snakes array.
    if (moveInput.snakesArr) {

      // Free allocated snake coordinates.
      for (int snakeIdx = 0; snakeIdx < moveInput.numSnakes; snakeIdx++) {
        Snake &snake = moveInput.snakesArr[snakeIdx];
        if (snake.coordsArr) {
          free(snake.coordsArr);
        }
      }

      // Free snakes array.
      free(moveInput.snakesArr);
    }
  }
  catch (std::exception& e) {
    std::cerr << "ERROR in /move: " << e.what() << std::endl;
  }
  catch (...) {
    std::cerr << "ERROR in /move: Unknown exception caught." << std::endl;
  }
  return rval.dump();
}


// //////////////////////////////////////////////////////////////////////////
std::string SnakeMove::handleReceive(std::string &rxBuf, const int recvbuflen) {
  const char *cbuf = rxBuf.c_str();
  const int jsonIdx = rxBuf.find("json", 0);
  std::string rval = "{ \"move\":\"up\" }";
  if (jsonIdx >= 0) {
    const int contentLengthIdx = rxBuf.find("Content-Length", jsonIdx);
    if (contentLengthIdx >= jsonIdx) {
      const int bracketIdx = rxBuf.find("{", contentLengthIdx);
      if (bracketIdx >= 0) {
        const int startIdx = rxBuf.find("/start");
        const bool isStart = ((startIdx > 0) && (startIdx < jsonIdx));
        if (isStart) {
          rval = parseStart(&cbuf[bracketIdx]);
        }
        else {
          rval = parseMove(&cbuf[bracketIdx]);
        }
      }
    }
  }
  return rval;

}

// //////////////////////////////////////////////////////////////////////////
bool SnakeMove::nextMove() {
  int iResult = 0;

  bool rval = true;

  // Accept a client socket
  SOCKET clientSocket = accept(ListenSocket, NULL, NULL);
  if (clientSocket == INVALID_SOCKET) {
    printf("accept failed with error: %d\n", WSAGetLastError());
    return false;
  }

  // Receive until the peer shuts down the connection
  do {
    iResult = recv(clientSocket, recvbuf, recvbuflen - 1, 0);
    if (iResult > 0) {
      recvbuf[iResult] = 0;
      printf("Bytes received: %d\n", iResult);
      std::string rxBuf = recvbuf;
      std::string response = handleReceive(rxBuf, iResult);
      response.append("\r\n");
      // Echo the buffer back to the sender
      std::stringstream sstream;
      sstream <<
        "HTTP/1.1 200 OK\r\n"
        "Server: Apache/1.3.0 (Unix)\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: " << response.length() << "\r\n\r\n"
        << response;
      const std::string s = sstream.str();
      int iSendResult = send(clientSocket, s.c_str(), s.length(), 0);
      if (iSendResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        sockClose(clientSocket);
      }
      printf("Bytes sent: %d\n", iSendResult);
    }
    else if (iResult == 0) {
      // Do nothing, just exit...
    }
    else {
      printf("recv failed with error: %d\n", WSAGetLastError());
      sockClose(clientSocket);
    }

  } while (iResult > 0);

  // shutdown the connection since we're done
  iResult = sockClose(clientSocket);
  if (iResult == SOCKET_ERROR) {
    printf("shutdown failed with error: %d\n", WSAGetLastError());
    sockClose(clientSocket);
  }

  // cleanup
  sockClose(clientSocket);

  return rval;
}

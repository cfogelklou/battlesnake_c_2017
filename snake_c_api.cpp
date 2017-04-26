#include "nlohmann/src/json.hpp"
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
  SnakeMove(SnakeImplementationT * pSnake, const char * const port, void * const pUserData);
  ~SnakeMove();

  std::string parseStart(const char * const cbuf);
  std::string parseMove(const char * const cbuf);
  std::string handleReceive(std::string &rxBuf, const int recvbuflen);
  bool nextMove();


private:
  SOCKET ListenSocket = INVALID_SOCKET;

  char recvbuf[DEFAULT_BUFLEN];
  int recvbuflen = DEFAULT_BUFLEN;
  SnakeImplementationT *mpSnake;
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
void SnakeStart(
  SnakeImplementationT * const pSnake, 
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
SnakeMove::SnakeMove(SnakeImplementationT * pSnake, const char * const port, void * const pUserData)
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
static void jsonArrToCArr(const json jarr, CoordsT * &pArr, int &numCoords) {
  numCoords = jarr.size();
  if (numCoords > 0) {
    pArr = (CoordsT *)calloc(numCoords, sizeof(CoordsT));

    int coordsIdx = 0;
    for (const json coord : jarr) {
      pArr[coordsIdx].x = coord[0].get<int>();
      pArr[coordsIdx++].y = coord[1].get<int>();
    }
  }
}

// ////////////////////////////////////////////////////////////////////////////
std::string SnakeMove::parseMove(const char * const cbuf) {
  json rval = {
    { "move", "up" },
    { "taunt", "ouch" }
  };
  try {

    MoveInputT moveInput = { 0 };
    MoveOutputT moveOutput = { UP };

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
    moveInput.snakesArr = (SnakeInfoT *)calloc(moveInput.numSnakes, sizeof(SnakeInfoT));

    int snakeIdx = 0;
    for (const json snake : snakes) {
      std::string id = you_uuid;
      SnakeInfoT &destSnake = moveInput.snakesArr[snakeIdx++];

      if (snake["id"].size() > 0) {
        id = snake["id"].get<std::string>();
      }

      if (id == you_uuid) {
        moveInput.yourSnakeIdx = snakeIdx;
      }

      if (snake["name"].size() > 0) {
        const std::string name = snake["name"].get<std::string>();
        memcpy(destSnake.name, name.c_str(), SNAKE_STRLEN);
      }

      if (snake["taunt"].size() > 0) {
        const std::string taunt = snake["taunt"].get<std::string>();
        memcpy(destSnake.taunt, taunt.c_str(), SNAKE_STRLEN);
      }

      memcpy(destSnake.id, id.c_str(), SNAKE_STRLEN);

      if (snake["health_points"].size() > 0) {
        destSnake.healthPercent = snake["health_points"].get<int>();
      }

      jsonArrToCArr(snake["coords"], destSnake.coordsArr, destSnake.numCoords);

    }

    jsonArrToCArr(req["food"], moveInput.foodArr, moveInput.numFood);

    if ((mpSnake) && (mpSnake->Move)) {
      mpSnake->Move(mpUserData, game_id.c_str(), &moveInput, &moveOutput);
      rval["move"] = SnakeDirStr(moveOutput.dir);
      if (strlen(moveOutput.taunt) >= 1) {
        rval["taunt"] = moveOutput.taunt;
      }
    }

    // Free allocated food.
    if (moveInput.foodArr) {
      free(moveInput.foodArr);
    }

    // Free allocated snake coordinates.
    for (int snakeIdx = 0; snakeIdx < moveInput.numSnakes; snakeIdx++) {
      SnakeInfoT &snake = moveInput.snakesArr[snakeIdx];
      if (snake.coordsArr) {
        free(snake.coordsArr);
      }
    }

    // Free snakes array.
    if (moveInput.snakesArr) {
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
    //sockClose(ListenSocket);
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

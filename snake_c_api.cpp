/**
* Copyright (c) 2017, Chris Fogelklou
* All rights reserved.
*/


#include "snake_c_api.h"
#include "snake_c_utils.h"
#include <thread>
#include <mutex>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
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
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)
#endif

#ifdef STANDALONE_JSON
#include "nlohmann/src/json.hpp"
#else
#include <main/json.hpp>
#endif

#include <iostream>
#include <sstream>
#include <assert.h>

static const int DEFAULT_BUFLEN = 32768;

// ////////////////////////////////////////////////////////////////////////////
class SnakeSng {
public:

  // Singleton getter
  static SnakeSng &inst() {
    if (NULL == mpInst) {
      mpInst = new SnakeSng();
    }
    return *mpInst;
  }

  // Constructor.  Init sockets.
  SnakeSng() {
#ifdef _WIN32
    WSADATA wsa_data;
    WSAStartup(MAKEWORD(1, 1), &wsa_data);
#endif
  }

  // Destructor, deinit sockets.
  ~SnakeSng() {
#ifdef _WIN32
    WSACleanup();    
#endif
  }

  // Close a socket.
  int sockClose(const SOCKET sock) {
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

private:
  static SnakeSng *mpInst;

};

// Singleton instance
SnakeSng *SnakeSng::mpInst = NULL;

class SnakeMoveListener;

typedef struct {
  SnakeMoveListener *pThis = NULL;
  SOCKET clientSocket = 0;
  int recvbuflen = DEFAULT_BUFLEN;
  char recvbuf[DEFAULT_BUFLEN] = { 0 };
} MoveListenerThreadData;

// ////////////////////////////////////////////////////////////////////////////
class SnakeMoveListener {
public:
  SnakeMoveListener(const SnakeCallbacks * const pSnake, const char * const port, void * const pUserData);
  ~SnakeMoveListener();

  std::string parseStart(const char * const cbuf);
  std::string parseMove(const char * const cbuf);
  std::string handleReceive(std::string &rxBuf, const int recvbuflen);
  bool nextMove();

  void Moving(MoveListenerThreadData &sd);

private:
  SOCKET ListenSocket = INVALID_SOCKET;


  const SnakeCallbacks *mpSnake;
  void *mpUserData;
  std::mutex mMutex;

};


// ////////////////////////////////////////////////////////////////////////////
SnakeMoveListener::SnakeMoveListener(const SnakeCallbacks * const pSnake, const char * const port, void * const pUserData)
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
    SnakeSng::inst().sockClose(ListenSocket);
    return;
  }

  freeaddrinfo(result);

  iResult = listen(ListenSocket, SOMAXCONN);
  if (iResult == SOCKET_ERROR) {
    std::cerr << "listen failed with error" << std::endl;
    SnakeSng::inst().sockClose(ListenSocket);
    return;
  }
}

// ////////////////////////////////////////////////////////////////////////////
SnakeMoveListener::~SnakeMoveListener() {
  // No longer need server socket
  SnakeSng::inst().sockClose(ListenSocket);
}


// ////////////////////////////////////////////////////////////////////////////
std::string SnakeMoveListener::parseStart(const char * const cbuf) {

  StartOutputT out = {
    "red",
    "white",
    "Default McDefaultyFace",
    "Your mother smells of elderberries!",
    SH_FANG,
    ST_CURLED,
  };

  try {
    nlohmann::json req = nlohmann::json::parse(cbuf);
    if ((mpSnake) && (mpSnake->Start)) {
      const std::string game_id = req["game_id"].get<std::string>();
      auto width = req["width"].get<int>();
      auto height = req["height"].get<int>();
      mMutex.lock();
      mpSnake->Start(mpUserData, game_id.c_str(), width, height, &out);
      mMutex.unlock();
    }
  }
  catch (std::exception& e) {
    std::cerr << "ERROR in /start: " << e.what() << std::endl;
  }
  catch (...) {
    std::cerr << "ERROR in /start: Unknown exception caught." << std::endl;
  }

  // Create return value
  nlohmann::json rval = nlohmann::json::object();
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
static void jsonArrToCArr(const nlohmann::json &jarr, Coords * &pArr, int &numCoords) {
  numCoords = jarr.size();
  if (numCoords > 0) {
    pArr = (Coords *)calloc(numCoords, sizeof(Coords));

    int coordsIdx = 0;
    for (const nlohmann::json coord : jarr) {
      pArr[coordsIdx] = coord.get<Coords>();
      coordsIdx++;
    }
  }
}

// ////////////////////////////////////////////////////////////////////////////
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
std::string SnakeMoveListener::parseMove(const char * const cbuf) {
  nlohmann::json rval = {
    { "move", "up" },
    { "taunt", "ouch" }
  };
  try {

    // If the move function is defined, call it.
    if ((mpSnake) && (mpSnake->Move)) {
      MoveInput moveInput = { 0 };
      MoveOutput moveOutput = { DIR_UP };

      const nlohmann::json req = nlohmann::json::parse(cbuf);

      if ((req["you"].size() <= 0) && (req["snakes"].size() <= 0)) {
        return rval.dump();
      }

      const std::string game_id = req["game_id"].get<std::string>();
      const std::string you_uuid = req["you"];
      const nlohmann::json snakes = req["snakes"];

#ifdef _DEBUG
      std::cout << "snakes were " << snakes.dump() << std::endl;
      std::cout << std::endl;
#endif

      moveInput.numSnakes = snakes.size();
      moveInput.snakesArr = (Snake *)calloc(moveInput.numSnakes, sizeof(Snake));

      // Convert from json to struct.
      int snakeIdx = 0;
      for (const nlohmann::json jsnake : snakes) {
        Snake &destSnake = moveInput.snakesArr[snakeIdx];
        destSnake = jsnake.get<Snake>();
        if (destSnake.id == you_uuid) {
          moveInput.yourSnakeIdx = snakeIdx;
        }
        snakeIdx++;
      }

      // Convert food to a C array.
      jsonArrToCArr(req["food"], moveInput.foodArr, moveInput.numFood);

      moveInput.width = req["width"].get<int>();
      moveInput.height = req["height"].get<int>();

      mMutex.lock();
      mpSnake->Move(mpUserData, game_id.c_str(), &moveInput, &moveOutput);
      mMutex.unlock();
      
      // Handle output of the move call
      rval["move"] = SnakeDirStr(moveOutput.dir);
      if (strlen(moveOutput.taunt) >= 1) {
        rval["taunt"] = moveOutput.taunt;
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
std::string SnakeMoveListener::handleReceive(std::string &rxBuf, const int recvbuflen) {
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
void ThreadCb(MoveListenerThreadData *p) {

  p->pThis->Moving(*p);

  delete p;
}

// //////////////////////////////////////////////////////////////////////////
void SnakeMoveListener::Moving(MoveListenerThreadData &sd) {
  // Receive until the peer shuts down the connection
  int iResult = 1;
  do {
    iResult = recv(sd.clientSocket, sd.recvbuf, sd.recvbuflen - 1, 0);
    if (iResult > 0) {
      sd.recvbuf[iResult] = 0;
      printf("Bytes received: %d\n", iResult);
      std::string rxBuf = sd.recvbuf;
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
      int iSendResult = send(sd.clientSocket, s.c_str(), s.length(), 0);
      if (iSendResult == SOCKET_ERROR) {
        std::cerr << "send failed with error" << std::endl;
        SnakeSng::inst().sockClose(sd.clientSocket);
      }
      printf("Bytes sent: %d\n", iSendResult);
    }
    else if (iResult == 0) {
      // Do nothing, just exit...
      //std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    else {
      std::cerr << "recv failed with error" << std::endl;
      SnakeSng::inst().sockClose(sd.clientSocket);
    }
  } while (iResult > 0);

  // shutdown the connection since we're done
  iResult = SnakeSng::inst().sockClose(sd.clientSocket);
  if (iResult == SOCKET_ERROR) {
    std::cerr << "shutdown failed with error" << std::endl;
    SnakeSng::inst().sockClose(sd.clientSocket);
  }

  // cleanup
  SnakeSng::inst().sockClose(sd.clientSocket);

}

// //////////////////////////////////////////////////////////////////////////
bool SnakeMoveListener::nextMove() {
  int iResult = 0;
  bool rval = true;

  // Accept a client socket
  SOCKET clientSocket = accept(ListenSocket, NULL, NULL);
  if (clientSocket == INVALID_SOCKET) {
    std::cerr << "accept failed with error" << std::endl;
    return false;
  }

  MoveListenerThreadData *pThreadData = new MoveListenerThreadData;
  pThreadData->clientSocket = clientSocket;
  pThreadData->pThis = this;
  pThreadData->recvbuflen = sizeof(pThreadData->recvbuf);
  //OSALStartThread(ThreadCb, pThreadData);
  std::thread t(ThreadCb, pThreadData); // pass by reference
  t.detach();
  return rval;
}






extern "C" {

// ////////////////////////////////////////////////////////////////////////////
void SnakeStart(
  const SnakeCallbacks * const pSnake,
  const char * const port,
  void * const pUserData) {
  (void)SnakeSng::inst();

  assert(pSnake);
  assert(port);

  SnakeMoveListener snake(pSnake, port, pUserData);

  while (snake.nextMove()) { ; }
}


} // extern "C" {

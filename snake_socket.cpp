#include "snake_socket.h"

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

#include "jsmn/jsmn.h"

#include <iostream>
#include <sstream>


// ////////////////////////////////////////////////////////////////////////////
static int sockInit(void)
{
#ifdef _WIN32
  WSADATA wsa_data;
  return WSAStartup(MAKEWORD(1, 1), &wsa_data);
#else
  return 0;
#endif
}

// ////////////////////////////////////////////////////////////////////////////
static int sockQuit(void)
{
#ifdef _WIN32
  return WSACleanup();
#else
  return 0;
#endif
}


// ////////////////////////////////////////////////////////////////////////////
/* Note: For POSIX, typedef SOCKET as an int. */

static int sockClose(SOCKET sock)
{

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

#define DEFAULT_PORT  "27015"
static const int DEFAULT_BUFLEN = 65536;

#define ARRSZ(x) (sizeof(x)/sizeof(x[0]))
#define MIN(x,y) ((x) < (y)) ? (x) : (y)
#define MAX(x,y) ((x) > (y)) ? (x) : (y)

// ////////////////////////////////////////////////////////////////////////////
class SnakeSng {
public:
  static SnakeSng &inst() {
    if (NULL == mpInst) {
      mpInst = new SnakeSng();
    }
    return *mpInst;
  }

  void parse(const char *cbuf, const int len) {
    int err = jsmn_parse(&mParser, cbuf, len, tokens, ARRSZ(tokens));

    for (int i = 0; i < ARRSZ(tokens); i++) {
      jsmntok_t &t = tokens[i];
      char token[256];
      memset(token, 0, sizeof(token));
      const int cpy = MIN(ARRSZ(token) - 1, t.size);
      memcpy(token, &cbuf[t.start], cpy);
      switch (t.type) {
      case JSMN_UNDEFINED: {
        std::cout << "undefined" << std::endl;
      } break;
      case JSMN_OBJECT: {
        std::cout << "object" << std::endl;
      } break;
      case JSMN_ARRAY: {
        std::cout << "array" << std::endl;
      } break;
      case JSMN_STRING: {
        std::cout << "string" << std::endl;

      } break;
      case JSMN_PRIMITIVE: {
        std::cout << "primitive" << std::endl;
      } break;
      default:
        break;
      } // Switch
    }
  }

  SnakeSng() {
    sockInit();
    jsmn_init(&mParser);
  }

  ~SnakeSng() {
    sockQuit();
  }

private:

  jsmn_parser mParser;
  jsmntok_t tokens[2048];
  
  static SnakeSng *mpInst;

};

SnakeSng *SnakeSng::mpInst = NULL;

// ////////////////////////////////////////////////////////////////////////////
class SnakeMove {
public:
  SnakeMove(SnakeImplementationT * pSnake)
    : mpSnake(pSnake)
  {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    struct addrinfo *result = NULL;
    int iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
      std::cout << "getaddrinfo failed with error: " << iResult << std::endl;
      return;
    }

    // Create a SOCKET for connecting to server
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
      std::cout << "socket failed with error:" << std::endl;
      freeaddrinfo(result);
      return;
    }

    // Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
      printf("bind failed with error: %d\n", WSAGetLastError());
      freeaddrinfo(result);
      sockClose(ListenSocket);
      return;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
      printf("listen failed with error: %d\n", WSAGetLastError());
      sockClose(ListenSocket);
      return;
    }

  }

  // //////////////////////////////////////////////////////////////////////////
  std::string handleReceive(std::string &rxBuf) {
    const char *cbuf = rxBuf.c_str();
    const int jsonIdx = rxBuf.find("json", 0);
    if (jsonIdx >= 0) {
      const int contentLengthIdx = rxBuf.find("Content-Length", jsonIdx);
      if (contentLengthIdx >= jsonIdx) {
        const int bracketIdx = rxBuf.find("{", contentLengthIdx);
        if (bracketIdx >= 0) {
          SnakeSng::inst().parse(&cbuf[bracketIdx], rxBuf.length() - jsonIdx);
        }
      }
    }
    std::string cmd = "{ \"move\":\"up\" }";
    return cmd;

  }

  // //////////////////////////////////////////////////////////////////////////
  bool nextMove() {
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

      iResult = recv(clientSocket, recvbuf, recvbuflen, 0);
      if (iResult > 0) {
        printf("Bytes received: %d\n", iResult);
        std::string rxBuf = recvbuf;
        std::string response = handleReceive(rxBuf);

        // Echo the buffer back to the sender
        std::stringstream sstream;
        sstream << "HTTP / 1.1 200 OK\r\n"
          "Content - Type: application / json\r\n"
          "Content - Length:";
        int cmdlen = response.length();
        sstream << response << "\r\n\r\n";
        std::string s = sstream.str();
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

  ~SnakeMove() {
    // No longer need server socket
    sockClose(ListenSocket);
  }

private:
  SOCKET ListenSocket = INVALID_SOCKET;

  char recvbuf[DEFAULT_BUFLEN];
  int recvbuflen = DEFAULT_BUFLEN;
  SnakeImplementationT *mpSnake;

};

// ////////////////////////////////////////////////////////////////////////////
extern "C" {
  void *SnakeAllocAndStart(SnakeImplementationT * pSnake) {  
    (void)SnakeSng::inst();
    SnakeMove snake(pSnake);
    while (snake.nextMove()) {
    }

    return NULL;

  }

  void SnakeFree(void *pSnakeRunner) {
    //SnakeSocket *pSnakeClass = (SnakeSocket *)pSnakeRunner;
    //delete pSnakeClass;
  }

}

//#define constexpr const
//#include "nlohmann/src/json.hpp"

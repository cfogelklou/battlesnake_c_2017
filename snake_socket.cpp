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
using namespace std;

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
static const int DEFAULT_BUFLEN = 8192;

// ////////////////////////////////////////////////////////////////////////////
class SnakeSocketSingleton {
public:
  void SayHi() {
    cout << "Hi!" << endl;
  }

  SnakeSocketSingleton() {
    int m = sockInit();
  }

  ~SnakeSocketSingleton() {
    sockQuit();
  }
};

SnakeSocketSingleton sock;


// ////////////////////////////////////////////////////////////////////////////
class SnakeMove {
public:
  SnakeMove(SnakeImplementationT * pSnake) {
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
      cout << "getaddrinfo failed with error: " << iResult << endl;
      return;
    }

    // Create a SOCKET for connecting to server
    {
      SOCKET ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
      if (ListenSocket == INVALID_SOCKET) {
        cout << "socket failed with error:" << endl;
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

      // Accept a client socket
      ClientSocket = accept(ListenSocket, NULL, NULL);
      if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        sockClose(ListenSocket);
        return;
      }

      // No longer need server socket
      sockClose(ListenSocket);
    }

    // Receive until the peer shuts down the connection
    do {

      iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
      if (iResult > 0) {
        printf("Bytes received: %d\n", iResult);

        // Echo the buffer back to the sender
        stringstream sstream;
        sstream << "HTTP / 1.1 200 OK\r\n"
          "Content - Type: application / json\r\n"
          "Content - Length:";
        std::string cmd = "{ \"move\":\"up\" }";
        int cmdlen = cmd.length();
        sstream << cmdlen << "\r\n\r\n";
        string s = sstream.str();
        int iSendResult = send(ClientSocket, s.c_str(), s.length(), 0);
        if (iSendResult == SOCKET_ERROR) {
          printf("send failed with error: %d\n", WSAGetLastError());
          sockClose(ClientSocket);
        }
        printf("Bytes sent: %d\n", iSendResult);
      }
      else if (iResult == 0) {
#ifdef _WIN32
        Sleep(10);
#else
        usleep(10 * 1000);
#endif
      }
      else {
        printf("recv failed with error: %d\n", WSAGetLastError());
        sockClose(ClientSocket);
      }

    } while (iResult >= 0);

    // shutdown the connection since we're done
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
      printf("shutdown failed with error: %d\n", WSAGetLastError());
      sockClose(ClientSocket);
    }

    // cleanup
    sockClose(ClientSocket);
    WSACleanup();

  }

  ~SnakeMove() {

  }

private:
  SOCKET ClientSocket = INVALID_SOCKET;
  char recvbuf[DEFAULT_BUFLEN];
  int recvbuflen = DEFAULT_BUFLEN;

};

// ////////////////////////////////////////////////////////////////////////////
extern "C" {
  void *SnakeAllocAndStart(SnakeImplementationT * pSnake) {
    sock.SayHi();
    while (1) {
      SnakeMove move(pSnake);
    }

    return NULL;// (void *)new SnakeSocket(pSnake);

  }

  void SnakeFree(void *pSnakeRunner) {
    //SnakeSocket *pSnakeClass = (SnakeSocket *)pSnakeRunner;
    //delete pSnakeClass;
  }

}

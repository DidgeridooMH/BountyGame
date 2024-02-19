#include "Otter/Networking/Server/TcpGameServer.h"

#include "Otter/Networking/Messages/ControlMessages.h"
#include "Otter/Networking/Shared/GameNetworking.h"

static SOCKET game_server_accept_client(TcpGameServer* server)
{
  struct sockaddr clientAddress = {0};
  int addressLength             = sizeof(struct sockaddr);
  SOCKET client =
      WSAAccept(server->socket, &clientAddress, &addressLength, NULL, 0);
  if (client == INVALID_SOCKET)
  {
    fprintf(stderr, "(E%d) There was a problem accepting the client.",
        WSAGetLastError());
    return INVALID_SOCKET;
  }

  uint32_t mode = 1;
  ioctlsocket(client, FIONBIO, &mode);

  wchar_t clientIp[] = L"000.000.000.000:00000";
  int clientIpLength = sizeof(clientIp) - 2;
  if (WSAAddressToString((struct sockaddr*) &clientAddress, addressLength, NULL,
          clientIp, &clientIpLength)
      == SOCKET_ERROR)
  {
    fprintf(stderr, "(E%d) There was an issue getting the IP of the client\n",
        WSAGetLastError());
  }

  wprintf(L"Client connected %s\n", clientIp);

  return client;
}

static int game_server_get_free_client_slot(TcpGameServer* server)
{
  for (int i = 0; i < MAX_CLIENTS; i++)
  {
    if (server->clients[i] == INVALID_SOCKET)
    {
      return i;
    }
  }
  return -1;
}

static void game_server_disconnect_client_socket(SOCKET socket)
{
  shutdown(socket, SD_BOTH);
  closesocket(socket);
}

void tcp_game_server_disconnect_client(TcpGameServer* server, int clientId)
{
  printf("Client disconnected\n");
  game_server_disconnect_client_socket(server->clients[clientId]);
  server->disconnectCb(clientId);
}

static int WINAPI game_server_accept_thread(void* _server)
{
  TcpGameServer* server = _server;

  server->running = true;

  WSAPOLLFD fd = {.fd = server->socket, .events = POLLRDNORM};
  while (server->running)
  {
    if (WSAPoll(&fd, 1, 0) > 0)
    {
      SOCKET client = game_server_accept_client(server);
      if (client != INVALID_SOCKET)
      {
        int freeClientSlot = game_server_get_free_client_slot(server);
        if (freeClientSlot >= 0)
        {
          server->clients[freeClientSlot] = client;
        }
        else
        {
          game_server_disconnect_client_socket(client);
        }
      }
    }
  }

  for (int i = 0; i < MAX_CLIENTS; i++)
  {
    if (server->clients[i] != INVALID_SOCKET)
    {
      tcp_game_server_disconnect_client(server, i);
    }
  }

  printf("Accept thread is done running.\n");

  return 0;
}

bool tcp_game_server_create(TcpGameServer* server, const char* host,
    const char* port, GameServerDisconnectCb disconnectCb)
{
  struct addrinfo serverAddress = {.ai_family = AF_INET,
      .ai_socktype                            = SOCK_STREAM,
      .ai_protocol                            = IPPROTO_TCP,
      .ai_flags                               = AI_PASSIVE};

  struct addrinfo* resolvedAddress;
  if (getaddrinfo(host, port, &serverAddress, &resolvedAddress))
  {
    fprintf(stderr, "Unable to resolve address %s:%s because %d", host, port,
        GetLastError());
    return false;
  }

  server->socket = socket(resolvedAddress->ai_family,
      resolvedAddress->ai_socktype, resolvedAddress->ai_protocol);

  if (server->socket == INVALID_SOCKET)
  {
    fprintf(stderr, "(%d) Unable to open socket.", GetLastError());
    freeaddrinfo(resolvedAddress);
    return false;
  }

  if (bind(server->socket, resolvedAddress->ai_addr,
          (int) resolvedAddress->ai_addrlen))
  {
    fprintf(stderr, "(%d) Unable to bind socket.", GetLastError());
    freeaddrinfo(resolvedAddress);
    return false;
  }

  freeaddrinfo(resolvedAddress);

  if (listen(server->socket, SOMAXCONN) == SOCKET_ERROR)
  {
    fprintf(stderr, "(%d) Unable to listen to socket.", GetLastError());
    closesocket(server->socket);
    return false;
  }

  for (int i = 0; i < MAX_CLIENTS; i++)
  {
    server->clients[i] = INVALID_SOCKET;
  }

  server->disconnectCb = disconnectCb;

  server->acceptThread =
      CreateThread(NULL, 0, game_server_accept_thread, server, 0, NULL);

  return true;
}

Message* tcp_game_server_get_message(TcpGameServer* server, int* clientId)
{
  int fdsId[MAX_CLIENTS]     = {0};
  WSAPOLLFD fds[MAX_CLIENTS] = {0};
  int nextFd                 = 0;
  for (int i = 0; i < MAX_CLIENTS; i++)
  {
    if (server->clients[i] != INVALID_SOCKET)
    {
      fds[nextFd].fd     = server->clients[i];
      fds[nextFd].events = POLLRDNORM;
      fdsId[nextFd]      = i;
      nextFd += 1;
    }
  }

  // TODO: We need to make sure that one client isn't overtaking everyone.
  if (nextFd > 0 && WSAPoll(fds, nextFd, 0) > 0)
  {
    for (int i = 0; i < nextFd; i++)
    {
      if (fds[i].revents > 0)
      {
        Message* message = game_networking_recv_message_tcp(fds[i].fd);
        if (message != NULL)
        {
          *clientId = fdsId[i];
          return message;
        }

        // Could not read from client. Assume connection is dead.
        tcp_game_server_disconnect_client(server, fdsId[i]);
      }
    }
  }

  return NULL;
}

void tcp_game_server_send_message(
    TcpGameServer* server, int clientId, const Message* message)
{
  if (clientId != ALL_CLIENT_ID)
  {
    game_networking_send_message_tcp(server->clients[clientId], message);
  }
  else
  {
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
      if (server->clients[i] != INVALID_SOCKET)
      {
        game_networking_send_message_tcp(server->clients[i], message);
      }
    }
  }
}

void tcp_game_server_destroy(TcpGameServer* server)
{
  server->running = false;
  WaitForSingleObject(server->acceptThread, 60000);
  closesocket(server->socket);
}

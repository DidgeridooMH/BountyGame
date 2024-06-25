set(SOURCES
  Private/Otter/Networking/Client/TcpGameClient.c
  Private/Otter/Networking/Client/UdpGameClient.c
  Private/Otter/Networking/Messages/ControlMessages.c
  Private/Otter/Networking/Messages/EntityMessages.c
  Private/Otter/Networking/Messages/Message.c
  Private/Otter/Networking/Server/TcpGameServer.c
  Private/Otter/Networking/Server/UdpGameServer.c
  Private/Otter/Networking/Shared/GameNetworking.c
  Private/Otter/Networking/dllmain.c
)

set(PRIVATE_HEADERS
  Private/pch.h
)

set(PUBLIC_HEADERS
  Public/Otter/Networking/Client/TcpGameClient.h
  Public/Otter/Networking/Client/UdpGameClient.h
  Public/Otter/Networking/Messages/ControlMessages.h
  Public/Otter/Networking/Messages/EntityMessages.h
  Public/Otter/Networking/Messages/Message.h
  Public/Otter/Networking/Server/TcpGameServer.h
  Public/Otter/Networking/Server/UdpGameServer.h
  Public/Otter/Networking/Shared/GameNetworking.h
)


if (BUILD_STATIC)
  add_library(OtterNetworking STATIC ${SOURCES} ${PUBLIC_HEADERS} ${PRIVATE_HEADERS})
else()
  add_library(OtterNetworking SHARED ${SOURCES} ${PUBLIC_HEADERS} ${PRIVATE_HEADERS})
endif()

if (WIN32)
  target_link_libraries(OtterNetworking PUBLIC ws2_32.lib)
endif()

target_compile_definitions(OtterNetworking PRIVATE OTTERNETWORKING_EXPORTS)
target_precompile_headers(OtterNetworking PRIVATE Private/pch.h)
target_include_directories(OtterNetworking PUBLIC Public PRIVATE Private)
target_link_libraries(OtterNetworking PUBLIC OtterGameState)


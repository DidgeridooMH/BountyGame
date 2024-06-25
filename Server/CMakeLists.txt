set(SOURCE
  Source/Server.c
)

set(HEADERS
  Source/Server.h
  Source/pch.h
)

add_executable(GameServer ${SOURCE} ${HEADERS})
target_include_directories(GameServer PRIVATE Source)
target_link_libraries(GameServer PRIVATE
  OtterGameState
  OtterAsync
  OtterMath
  OtterNetworking
  OtterConfig
  OtterUtil)
target_precompile_headers(GameServer PRIVATE Source/pch.h)

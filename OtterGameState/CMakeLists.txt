set(SOURCES
  Private/Otter/GameState/GameState.c
  Private/Otter/GameState/Player/Player.c
)

set(PRIVATE_HEADERS
  Private/pch.h
)

set(PUBLIC_HEADERS
  Public/Otter/GameState/export.h
  Public/Otter/GameState/GameState.h
  Public/Otter/GameState/Player/Player.h
  Public/Otter/GameState/Player/PlayerInput.h
)

if (BUILD_STATIC)
  add_library(OtterGameState STATIC ${SOURCES} ${PUBLIC_HEADERS})
else()
  add_library(OtterGameState SHARED ${SOURCES} ${PUBLIC_HEADERS})
endif()

target_compile_definitions(OtterGameState PRIVATE OTTERGAMESTATE_EXPORTS)
target_precompile_headers(OtterGameState PRIVATE Private/pch.h)
target_include_directories(OtterGameState PUBLIC Public)

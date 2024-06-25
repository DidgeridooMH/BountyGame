set(SOURCES
  Private/Otter/Render/Gltf/GlbAsset.c
  Private/Otter/Render/Gltf/GlbJsonChunk.c
  Private/Otter/Render/Memory/GpuBuffer.c
  Private/Otter/Render/Memory/MemoryType.c
  Private/Otter/Render/Pipeline/GBufferPipeline.c
  Private/Otter/Render/Pipeline/PbrPipeline.c
  Private/Otter/Render/Pipeline/Pipeline.c
  Private/Otter/Render/Texture/RenderImage.c
  Private/Otter/Render/Mesh.c
  Private/Otter/Render/RenderFrame.c
  Private/Otter/Render/RenderInstance.c
  Private/Otter/Render/RenderStack.c
  Private/Otter/Render/RenderSwapchain.c
)

set(PRIVATE_HEADERS
  Private/Otter/Render/Gltf/GlbJsonChunk.h
  Private/pch.h
)

set(PUBLIC_HEADERS
  Public/Otter/Render/Gltf/GlbAsset.h
  Public/Otter/Render/Memory/GpuBuffer.h
  Public/Otter/Render/Memory/MemoryType.h
  Public/Otter/Render/Pipeline/GBufferPipeline.h
  Public/Otter/Render/Pipeline/PbrPipeline.h
  Public/Otter/Render/Pipeline/Pipeline.h
  Public/Otter/Render/Texture/RenderImage.h
  Public/Otter/Render/Uniform/ModelViewProjection.h
  Public/Otter/Render/export.h
  Public/Otter/Render/Mesh.h
  Public/Otter/Render/RenderFrame.h
  Public/Otter/Render/RenderInstance.h
  Public/Otter/Render/RenderQueue.h
  Public/Otter/Render/RenderStack.h
  Public/Otter/Render/RenderSwapchain.h
)

if (BUILD_STATIC)
  add_library(OtterRender STATIC ${SOURCES} ${PUBLIC_HEADERS} ${PRIVATE_HEADERS})
else()
  add_library(OtterRender SHARED ${SOURCES} ${PUBLIC_HEADERS} ${PRIVATE_HEADERS})
endif()

find_package(Vulkan REQUIRED)

target_compile_definitions(OtterRender PRIVATE OTTERRENDER_EXPORTS)
target_precompile_headers(OtterRender PRIVATE Private/pch.h)
target_include_directories(OtterRender PUBLIC Public PRIVATE Private)
target_link_libraries(OtterRender PRIVATE OtterMath OtterUtil OtterAsync Vulkan::Vulkan)


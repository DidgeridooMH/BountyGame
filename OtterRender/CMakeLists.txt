set(SOURCES
  Private/Otter/Render/Gltf/GlbAsset.c
  Private/Otter/Render/Gltf/GlbJsonChunk.c
  Private/Otter/Render/Memory/GpuBuffer.c
  Private/Otter/Render/Memory/MemoryType.c
  Private/Otter/Render/Pipeline/GBufferPipeline.c
  Private/Otter/Render/Pipeline/PbrPipeline.c
  Private/Otter/Render/Pipeline/Pipeline.c
  Private/Otter/Render/RayTracing/AccelerationStructure.c
  Private/Otter/Render/Texture/Image.c
  Private/Otter/Render/Texture/ImageSampler.c
  Private/Otter/Render/Texture/Texture.c
  Private/Otter/Render/Mesh.c
  Private/Otter/Render/RenderFrame.c
  Private/Otter/Render/RenderInstance.c
  Private/Otter/Render/RenderPass/GBufferPass.c
  Private/Otter/Render/RenderPass/LightingPass.c
  Private/Otter/Render/RenderQueue.c
  Private/Otter/Render/RenderSwapchain.c
)

set(PRIVATE_HEADERS
  Private/pch.h
  Private/Extern/stb_image.h
)

set(PUBLIC_HEADERS
  Public/Otter/Render/Gltf/GlbAsset.h
  Public/Otter/Render/Gltf/GlbJsonChunk.h
  Public/Otter/Render/Memory/GpuBuffer.h
  Public/Otter/Render/Memory/MemoryType.h
  Public/Otter/Render/Pipeline/GBufferPipeline.h
  Public/Otter/Render/Pipeline/PbrPipeline.h
  Public/Otter/Render/Pipeline/Pipeline.h
  Public/Otter/Render/RayTracing/AccelerationStructure.h
  Public/Otter/Render/RenderPass/GBufferPass.h
  Public/Otter/Render/RenderPass/LightingPass.h
  Public/Otter/Render/Texture/Image.h
  Public/Otter/Render/Texture/ImageSampler.h
  Public/Otter/Render/Texture/Texture.h
  Public/Otter/Render/Uniform/ViewProjection.h
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

set(GLSL_VALIDATOR "glslangValidator")
set(SHADERS
  Shaders/gbuffer.vert.glsl
  Shaders/gbuffer.frag.glsl
  Shaders/pbr.vert.glsl
  Shaders/pbr.frag.glsl
)

foreach(SHADER ${SHADERS})
  get_filename_component(FILE_NAME ${SHADER} NAME_WLE)
  set(SPIRV "${CMAKE_BINARY_DIR}/Shaders/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${GLSL_VALIDATOR} -V ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER} -o ${SPIRV}
    DEPENDS ${SHADER}
  )
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach()

add_custom_target(Shaders DEPENDS ${SPIRV_BINARY_FILES})
add_dependencies(OtterRender Shaders)

add_custom_command(
  TARGET Shaders POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/bin/Shaders
  COMMAND ${CMAKE_COMMAND} -E copy ${SPIRV_BINARY_FILES} ${CMAKE_BINARY_DIR}/bin/Shaders
)


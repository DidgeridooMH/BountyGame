#include "Otter/Render/RenderInstance.h"

#include "Otter/Render/Memory/GpuBuffer.h"
#include "Otter/Render/Uniform/ModelViewProjection.h"

#define VK_VALIDATION_LAYER_NAME   "VK_LAYER_KHRONOS_validation"
#define REQUESTED_NUMBER_OF_FRAMES 3

static void render_instance_request_extensions(const char** requestedExtensions,
    bool** flags, uint32_t requestedExtensionCount,
    const char** enabledExtensions, uint32_t* enabledExtensionsCount,
    VkExtensionProperties* properties, uint32_t extensionPropertiesCount)
{
  while (requestedExtensionCount--)
  {
    if (flags != NULL && *flags != NULL)
    {
      **flags = false;
    }

    for (uint32_t i = 0; i < extensionPropertiesCount; i++)
    {
      if (strcmp(properties[i].extensionName, (*requestedExtensions)) == 0)
      {
        enabledExtensions[*enabledExtensionsCount] = *requestedExtensions;
        *enabledExtensionsCount += 1;

        if (flags != NULL && *flags != NULL)
        {
          **flags = true;
        }
        break;
      }
    }
    requestedExtensions++;
    if (flags != NULL)
    {
      flags++;
    }
  }
}

static bool render_instance_check_extensions(const char* requiredExtensions[],
    uint32_t requiredExtensionsCount, const char* optionalExtensions[],
    bool* optionalExtensionFlags[], uint32_t optionalExtensionsCount,
    const char* enabledExtensions[], uint32_t* enabledExtensionsCount)
{
  *enabledExtensionsCount = 0;

  uint32_t extensionCount = 0;
  if (vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL)
          != VK_SUCCESS
      || extensionCount == 0)
  {
    fprintf(stderr, "Unable to enumerate extensions\n");
    return false;
  }

  VkExtensionProperties* extensionProperties =
      malloc(sizeof(VkExtensionProperties) * extensionCount);
  if (extensionProperties == NULL)
  {
    fprintf(stderr, "OOM\n");
    return false;
  }

  if (vkEnumerateInstanceExtensionProperties(
          NULL, &extensionCount, extensionProperties)
      != VK_SUCCESS)
  {
    fprintf(stderr, "Unable to enumerate extensions\n");
    free(extensionProperties);
    return false;
  }

  render_instance_request_extensions(requiredExtensions, NULL,
      requiredExtensionsCount, enabledExtensions, enabledExtensionsCount,
      extensionProperties, extensionCount);
  if (*enabledExtensionsCount != requiredExtensionsCount)
  {
    fprintf(stderr, "Missing required extensions.\n");
    free(extensionProperties);
    return false;
  }

  render_instance_request_extensions(optionalExtensions, optionalExtensionFlags,
      optionalExtensionsCount, enabledExtensions, enabledExtensionsCount,
      extensionProperties, extensionCount);

  free(extensionProperties);
  return true;
}

static bool render_instance_check_layers(const char** requestedLayers,
    bool** flags, uint32_t layerCount, char** enabledLayers,
    uint32_t* enabledLayersCount)
{
  uint32_t propertyCount;
  if (vkEnumerateInstanceLayerProperties(&propertyCount, NULL) != VK_SUCCESS
      || propertyCount == 0)
  {
    fprintf(stderr, "Error: Unable to enumerate instance layers\n");
    return false;
  }

  VkLayerProperties* properties =
      malloc(sizeof(VkLayerProperties) * propertyCount);
  if (properties == NULL)
  {
    fprintf(
        stderr, "Error: Unable to enumerate instance layers because of OOM.\n");
    return false;
  }

  if (vkEnumerateInstanceLayerProperties(&propertyCount, properties)
      != VK_SUCCESS)
  {
    fprintf(stderr,
        "Error: Unable to enumerate instance layers with count %u\n",
        propertyCount);
    free(properties);
    return false;
  }

  for (uint32_t layer = 0; layer < layerCount; layer++)
  {
    if (*flags != NULL)
    {
      **flags = false;
    }

    for (uint32_t i = 0; i < propertyCount; i++)
    {
      if (strcmp(properties[i].layerName, requestedLayers[layer]) == 0)
      {
        if (*flags != NULL)
        {
          **flags = true;
        }
        break;
      }
    }
  }

  free(properties);
  return true;
}

static bool render_instance_create_instance(RenderInstance* renderInstance)
{
  VkApplicationInfo applicationInfo = {
      .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .apiVersion         = VK_API_VERSION_1_3,
      .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
      .engineVersion      = VK_MAKE_VERSION(0, 1, 0),
      .pApplicationName   = "OtterEngineGame",
      .pEngineName        = "Otter"};

  const char* requiredExtensions[] = {
      VK_KHR_WIN32_SURFACE_EXTENSION_NAME, VK_KHR_SURFACE_EXTENSION_NAME};

  const char* optionalExtensions[] = {
      VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME
#ifdef _DEBUG
      ,
      VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
  };

  bool* optionalExtensionFlags[] = {&renderInstance->capabilities.hdr
#ifdef _DEBUG
      ,
      &renderInstance->capabilities.debugUtils
#endif
  };

  char* enabledExtensions[_countof(requiredExtensions)
                          + _countof(optionalExtensions)] = {NULL};
  uint32_t enabledExtensionCount                          = 0;
  if (!render_instance_check_extensions(requiredExtensions,
          _countof(requiredExtensions), optionalExtensions,
          optionalExtensionFlags, _countof(optionalExtensions),
          enabledExtensions, &enabledExtensionCount))
  {
    return false;
  }

  const char* optionalLayers[] = {
#ifdef _DEBUG
      VK_VALIDATION_LAYER_NAME
#endif
  };
  char* enabledLayers[_countof(optionalLayers)] = {NULL};
  uint32_t enabledLayersCount                   = 0;

#ifdef _DEBUG
  bool validationFound = false;
#endif

  bool* flags[_countof(optionalLayers)] = {
#ifdef _DEBUG
      &validationFound
#endif
  };

  render_instance_check_layers(optionalLayers, flags,
      sizeof(optionalLayers) / sizeof(optionalLayers[0]), enabledLayers,
      &enabledLayersCount);

  VkInstanceCreateInfo createInfo = {
      .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo        = &applicationInfo,
      .enabledExtensionCount   = enabledExtensionCount,
      .ppEnabledExtensionNames = enabledExtensions,
      .enabledLayerCount       = enabledLayersCount,
      .ppEnabledLayerNames     = enabledLayers};

  if (vkCreateInstance(&createInfo, NULL, &renderInstance->instance)
      != VK_SUCCESS)
  {
    fprintf(stderr, "Failed to create Vulkan instance.\n");
    free(renderInstance);
    return false;
  }

  return true;
}

static bool render_instance_fetch_device(RenderInstance* renderInstance)
{
  uint32_t physicalDeviceCount = 0;
  if (vkEnumeratePhysicalDevices(
          renderInstance->instance, &physicalDeviceCount, NULL)
          != VK_SUCCESS
      || physicalDeviceCount == 0)
  {
    fprintf(stderr, "Unable to enumerate physical devices.\n");
    return false;
  }

  VkPhysicalDevice* availableDevices =
      malloc(sizeof(VkPhysicalDevice) * physicalDeviceCount);
  if (availableDevices == NULL)
  {
    fprintf(stderr, "OOM\n");
    return false;
  }

  if (vkEnumeratePhysicalDevices(
          renderInstance->instance, &physicalDeviceCount, availableDevices)
      != VK_SUCCESS)
  {
    fprintf(stderr, "Unable to enumerate physical devices with count %u\n",
        physicalDeviceCount);
    return false;
  }

  renderInstance->physicalDevice = availableDevices[0];
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(availableDevices[0], &properties);
  for (uint32_t i = 1; i < physicalDeviceCount; i++)
  {
    vkGetPhysicalDeviceProperties(availableDevices[i], &properties);
    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
      renderInstance->physicalDevice = availableDevices[i];
      break;
    }
  }

  printf("DEBUG: Using %s for rendering\n", properties.deviceName);

  return true;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL render_instance_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT flags,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData)
{
  if (severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
  {
    printf("Vulkan warning: %s", callbackData->pMessage);
  }
  else if (severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
  {
    fprintf(stderr, "Vulkan error: %s", callbackData->pMessage);
  }
  return VK_FALSE;
}

#ifdef _DEBUG
static bool render_instance_attach_debug_messenger(
    RenderInstance* renderInstance)
{
  VkDebugUtilsMessengerCreateInfoEXT createInfo = {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                       | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
      .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                   | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                   | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      .pfnUserCallback = render_instance_debug_callback,
  };

  PFN_vkCreateDebugUtilsMessengerEXT createDebugUtilsMessengerEXT =
      (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
          renderInstance->instance, "vkCreateDebugUtilsMessengerEXT");

  if (createDebugUtilsMessengerEXT == NULL)
  {
    fprintf(stderr, "Error: Debug messenger extension not available.\n");
    return false;
  }

  if (createDebugUtilsMessengerEXT(renderInstance->instance, &createInfo, NULL,
          &renderInstance->debugMessenger)
      != VK_SUCCESS)
  {
    fprintf(stderr, "Error: Unable to create debug messenger.\n");
    return false;
  }

  return true;
}
#endif

static bool render_instance_find_queue_families(RenderInstance* renderInstance)
{
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(
      renderInstance->physicalDevice, &queueFamilyCount, NULL);

  VkQueueFamilyProperties* queueFamilyProperties =
      malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
  if (queueFamilyProperties == NULL)
  {
    fprintf(stderr, "Unable to get queue families\n.");
    return false;
  }

  vkGetPhysicalDeviceQueueFamilyProperties(
      renderInstance->physicalDevice, &queueFamilyCount, queueFamilyProperties);

  renderInstance->graphicsQueueFamily = UINT32_MAX;
  for (uint32_t i = 0; i < queueFamilyCount; i++)
  {
    if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
    {
      renderInstance->graphicsQueueFamily = i;
      break;
    }
  }

  if (renderInstance->graphicsQueueFamily == UINT32_MAX)
  {
    fprintf(stderr, "Unable to find queue families\n");
    free(queueFamilyProperties);
    return false;
  }

  free(queueFamilyProperties);

  renderInstance->presentQueueFamily = renderInstance->graphicsQueueFamily;
  if (renderInstance->surface != VK_NULL_HANDLE)
  {
    for (uint32_t i = 0; i < queueFamilyCount; ++i)
    {
      VkBool32 supportsPresent = VK_FALSE;
      vkGetPhysicalDeviceSurfaceSupportKHR(renderInstance->physicalDevice, i,
          renderInstance->surface, &supportsPresent);
      if (supportsPresent)
      {
        renderInstance->presentQueueFamily = i;
        break;
      }
    }
  }

  return true;
}

static bool render_instance_create_logical_device(
    RenderInstance* renderInstance)
{
  const char* deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  float queuePriority            = 0.0f;

  VkDeviceQueueCreateInfo deviceQueueCreateInfo = {
      .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = renderInstance->graphicsQueueFamily,
      .queueCount       = 1,
      .pQueuePriorities = &queuePriority,
  };

  VkDeviceCreateInfo deviceCreateInfo = {
      .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .queueCreateInfoCount    = 1,
      .pQueueCreateInfos       = &deviceQueueCreateInfo,
      .enabledExtensionCount   = 1,
      .ppEnabledExtensionNames = deviceExtensions,
  };

  if (vkCreateDevice(renderInstance->physicalDevice, &deviceCreateInfo, NULL,
          &renderInstance->logicalDevice)
      != VK_SUCCESS)
  {
    fprintf(stderr, "Error: Unable to create logical device\n");
    return false;
  }

  return true;
}

static bool render_instance_create_swapchain(RenderInstance* renderInstance)
{
  if (renderInstance->swapchain != NULL)
  {
    render_swapchain_destroy(
        renderInstance->swapchain, renderInstance->logicalDevice);
  }

  VkSurfaceCapabilitiesKHR surfaceCaps;
  if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
          renderInstance->physicalDevice, renderInstance->surface, &surfaceCaps)
      != VK_SUCCESS)
  {
    fprintf(stderr, "Error: Unable to get surface capabilities\n");
    return false;
  }

  VkExtent2D swapchainExtent = surfaceCaps.currentExtent;
  printf("DEBUG: Surface area at %dx%d\n", swapchainExtent.width,
      swapchainExtent.height);

  uint32_t formatCount = 0;
  if (vkGetPhysicalDeviceSurfaceFormatsKHR(renderInstance->physicalDevice,
          renderInstance->surface, &formatCount, NULL)
          != VK_SUCCESS
      || formatCount == 0)
  {
    fprintf(stderr, "Error: Unable to get surface formats\n");
    return false;
  }

  VkSurfaceFormatKHR* formats =
      malloc(formatCount * sizeof(VkSurfaceFormatKHR));
  if (formats == NULL)
  {
    fprintf(stderr, "OOM\n");
    return false;
  }
  if (vkGetPhysicalDeviceSurfaceFormatsKHR(renderInstance->physicalDevice,
          renderInstance->surface, &formatCount, formats)
      != VK_SUCCESS)
  {
    fprintf(stderr, "Error: Unable to get surface formats\n");
    free(formats);
    return false;
  }

  VkSurfaceFormatKHR format = formats[0];
  if (renderInstance->settings.hdr)
  {
    for (uint32_t i = 0; i < formatCount; i++)
    {
      if (formats[i].colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT
          && formats[i].format == VK_FORMAT_A2B10G10R10_UNORM_PACK32)
      {
        format = formats[i];
        break;
      }
    }

    if (format.format != VK_COLOR_SPACE_HDR10_ST2084_EXT)
    {
      renderInstance->capabilities.hdr = false;
      renderInstance->settings.hdr     = false;
    }
  }

  free(formats);

  VkPresentModeKHR presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
  // TODO: Handle VSync settings.
  // if (GetVSync())
  if (true)
  {
    uint32_t presentModeCount = 0;
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(
            renderInstance->physicalDevice, renderInstance->surface,
            &presentModeCount, NULL)
            != VK_SUCCESS
        || presentModeCount == 0)
    {
      fprintf(stderr, "Unable to enumerate present modes.\n");
      return false;
    }

    VkPresentModeKHR* presentModes =
        malloc(sizeof(VkPresentModeKHR) * presentModeCount);
    if (presentModes == NULL)
    {
      fprintf(stderr, "OOM\n");
      return false;
    }

    if (vkGetPhysicalDeviceSurfacePresentModesKHR(
            renderInstance->physicalDevice, renderInstance->surface,
            &presentModeCount, presentModes)
        != VK_SUCCESS)
    {
      fprintf(stderr, "Unable to enumerate present modes.\n");
      free(presentModes);
      return false;
    }

    for (uint32_t i = 0; i < presentModeCount; ++i)
    {
      if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
      {
        presentMode = presentModes[i];
        break;
      }
      else if (presentModes[i] == VK_PRESENT_MODE_FIFO_KHR)
      {
        presentMode = presentModes[i];
      }
    }

    free(presentModes);
  }

  renderInstance->swapchain = render_swapchain_create(
      REQUESTED_NUMBER_OF_FRAMES, swapchainExtent, format, presentMode,
      renderInstance->physicalDevice, renderInstance->logicalDevice,
      renderInstance->surface, renderInstance->graphicsQueueFamily,
      renderInstance->presentQueueFamily);
  if (renderInstance->swapchain == NULL)
  {
    return false;
  }

  return true;
}

static bool render_instance_create_render_pass(RenderInstance* renderInstance)
{
  VkAttachmentDescription colorAttachment[] = {
      {
          .format        = VK_FORMAT_R16G16B16A16_SFLOAT,
          .samples       = VK_SAMPLE_COUNT_1_BIT,
          .loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR,
          .storeOp       = VK_ATTACHMENT_STORE_OP_STORE,
          .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .finalLayout   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      },
      {
          .format        = VK_FORMAT_R16G16B16A16_SFLOAT,
          .samples       = VK_SAMPLE_COUNT_1_BIT,
          .loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR,
          .storeOp       = VK_ATTACHMENT_STORE_OP_STORE,
          .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .finalLayout   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      },
      {
          .format        = VK_FORMAT_R16G16B16A16_SFLOAT,
          .samples       = VK_SAMPLE_COUNT_1_BIT,
          .loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR,
          .storeOp       = VK_ATTACHMENT_STORE_OP_STORE,
          .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .finalLayout   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      },
      {
          .format        = VK_FORMAT_R16G16B16A16_SFLOAT,
          .samples       = VK_SAMPLE_COUNT_1_BIT,
          .loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR,
          .storeOp       = VK_ATTACHMENT_STORE_OP_STORE,
          .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .finalLayout   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      },
      {
          .format        = renderInstance->swapchain->format.format,
          .samples       = VK_SAMPLE_COUNT_1_BIT,
          .loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR,
          .storeOp       = VK_ATTACHMENT_STORE_OP_STORE,
          .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .finalLayout   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      }};

  VkSubpassDependency dependencies[] = {
      {
          .srcSubpass   = VK_SUBPASS_EXTERNAL,
          .dstSubpass   = 0,
          .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
          .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      },
      {
          .srcSubpass   = 0,
          .dstSubpass   = 1,
          .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
          .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      }};

  VkAttachmentReference gbufferAttachmentRef[] = {
      {.attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
      {.attachment = 1, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
      {.attachment = 2, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
      {.attachment = 3, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}};

  VkAttachmentReference lightingBufferAttachmentRef[] = {
      {.attachment = 4, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
      {.attachment = 0, .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};

  VkSubpassDescription subpassDescriptions[] = {
      {
          .pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS,
          .colorAttachmentCount = _countof(gbufferAttachmentRef),
          .pColorAttachments    = gbufferAttachmentRef,
      },
      {
          .pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS,
          .colorAttachmentCount = _countof(lightingBufferAttachmentRef),
          .pColorAttachments    = lightingBufferAttachmentRef,
      }};

  VkRenderPassCreateInfo renderPassInfo = {
      .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .attachmentCount = _countof(colorAttachment),
      .pAttachments    = colorAttachment,
      .subpassCount    = _countof(subpassDescriptions),
      .pSubpasses      = subpassDescriptions,
      .dependencyCount = _countof(dependencies),
      .pDependencies   = dependencies,
  };

  VkResult result = vkCreateRenderPass(renderInstance->logicalDevice,
      &renderPassInfo, NULL, &renderInstance->renderPass);
  if (result != VK_SUCCESS)
  {
    fprintf(stderr, "Unable to create the render pass\n");
    return false;
  }

  return true;
}

static bool render_instance_create_render_surface(
    RenderInstance* renderInstance, HWND window)
{
  VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {
      .sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
      .hinstance = GetModuleHandle(NULL),
      .hwnd      = window};

  if (vkCreateWin32SurfaceKHR(renderInstance->instance, &surfaceCreateInfo,
          NULL, &renderInstance->surface)
      != VK_SUCCESS)
  {
    fprintf(stderr, "Unable to create win32 surface.\n");
    return false;
  }

  if (!render_instance_find_queue_families(renderInstance))
  {
    return false;
  }

  if (!render_instance_create_swapchain(renderInstance))
  {
    return false;
  }

  if (!render_instance_create_render_pass(renderInstance))
  {
    return false;
  }

  if (!render_swapchain_create_render_stacks(renderInstance->swapchain,
          renderInstance->physicalDevice, renderInstance->logicalDevice,
          renderInstance->renderPass))
  {
    return false;
  }

  return true;
}

static bool render_instance_create_command_pool(RenderInstance* renderInstance)
{
  VkCommandPoolCreateInfo poolInfo = {
      .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = renderInstance->graphicsQueueFamily};

  if (vkCreateCommandPool(renderInstance->logicalDevice, &poolInfo, NULL,
          &renderInstance->commandPool)
      != VK_SUCCESS)
  {
    fprintf(stderr, "Error: Failed to create command pool\n");
    return false;
  }

  return true;
}

RenderInstance* render_instance_create(HWND window)
{
  RenderInstance* renderInstance = calloc(1, sizeof(RenderInstance));
  if (renderInstance == NULL)
  {
    fprintf(stderr, "OOM\n");
    return NULL;
  }

  if (!render_instance_create_instance(renderInstance))
  {
    render_instance_destroy(renderInstance);
    return NULL;
  }

  // TODO: Implement system to set system settings and apply those changes.
  renderInstance->settings.hdr = renderInstance->capabilities.hdr;

#ifdef _DEBUG
  if (renderInstance->capabilities.debugUtils)
  {
    if (render_instance_attach_debug_messenger(renderInstance))
    {
      printf("DEBUG: Validation and debug layers enabled\n");
    }
    else
    {
      fprintf(stderr, "WARN: Debug messenger could not be attached. Continuing "
                      "without...\n");
    }
  }
#endif

  if (!render_instance_fetch_device(renderInstance))
  {
    render_instance_destroy(renderInstance);
    return NULL;
  }

  if (!render_instance_find_queue_families(renderInstance))
  {
    render_instance_destroy(renderInstance);
    return NULL;
  }

  if (!render_instance_create_logical_device(renderInstance))
  {
    render_instance_destroy(renderInstance);
    return NULL;
  }

  if (!render_instance_create_render_surface(renderInstance, window))
  {
    render_instance_destroy(renderInstance);
    return NULL;
  }

  renderInstance->framesInFlight =
      min(renderInstance->swapchain->numOfSwapchainImages,
          REQUESTED_NUMBER_OF_FRAMES);

  if (!render_instance_create_command_pool(renderInstance))
  {
    return NULL;
  }

  renderInstance->frames =
      malloc(sizeof(RenderFrame) * renderInstance->framesInFlight);
  if (renderInstance->frames == NULL)
  {
    return NULL;
  }

  for (uint32_t i = 0; i < renderInstance->framesInFlight; i++)
  {
    if (!render_frame_create(&renderInstance->frames[i],
            renderInstance->logicalDevice, renderInstance->commandPool))
    {
      return NULL;
    }
  }

  if (!g_buffer_pipeline_create(renderInstance->logicalDevice,
          renderInstance->renderPass, &renderInstance->gBufferPipeline))
  {
    return NULL;
  }

  if (!pbr_pipeline_create(renderInstance->logicalDevice,
          renderInstance->renderPass, &renderInstance->pbrPipeline))
  {
    return NULL;
  }

  Vec3 vertices[]    = {{-1.0f, 1.0f, 0.0f}, {-1.0f, -1.0f, 0.0f},
         {1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}};
  uint16_t indices[] = {1, 0, 2, 0, 3, 2};
  VkQueue transferQueue;
  vkGetDeviceQueue(renderInstance->logicalDevice,
      renderInstance->graphicsQueueFamily, 0, &transferQueue);
  renderInstance->fullscreenQuad = mesh_create(vertices, sizeof(Vec3),
      _countof(vertices), indices, _countof(indices),
      renderInstance->physicalDevice, renderInstance->logicalDevice,
      renderInstance->commandPool, transferQueue);
  if (renderInstance->fullscreenQuad == NULL)
  {
    return NULL;
  }

  return renderInstance;
}

void render_instance_destroy(RenderInstance* renderInstance)
{
  if (renderInstance->fullscreenQuad != NULL)
  {
    mesh_destroy(renderInstance->fullscreenQuad, renderInstance->logicalDevice);
  }

  pbr_pipeline_destroy(
      &renderInstance->pbrPipeline, renderInstance->logicalDevice);
  g_buffer_pipeline_destroy(
      &renderInstance->gBufferPipeline, renderInstance->logicalDevice);

  if (renderInstance->frames != NULL)
  {
    for (uint32_t i = 0; i < renderInstance->framesInFlight; i++)
    {
      render_frame_destroy(&renderInstance->frames[i],
          renderInstance->commandPool, renderInstance->logicalDevice);
    }
  }

  if (renderInstance->commandPool != NULL)
  {
    vkDestroyCommandPool(
        renderInstance->logicalDevice, renderInstance->commandPool, NULL);
  }

  if (renderInstance->renderPass != NULL)
  {
    vkDestroyRenderPass(
        renderInstance->logicalDevice, renderInstance->renderPass, NULL);
  }

  if (renderInstance->swapchain != NULL)
  {
    render_swapchain_destroy(
        renderInstance->swapchain, renderInstance->logicalDevice);
  }

  if (renderInstance->surface != VK_NULL_HANDLE)
  {
    vkDestroySurfaceKHR(
        renderInstance->instance, renderInstance->surface, NULL);
  }

  if (renderInstance->logicalDevice != VK_NULL_HANDLE)
  {
    vkDestroyDevice(renderInstance->logicalDevice, NULL);
  }

#ifdef _DEBUG
  if (renderInstance->debugMessenger != VK_NULL_HANDLE)
  {
    PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugUtilsMessenger =
        (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
            renderInstance->instance, "vkDestroyDebugUtilsMessengerEXT");
    destroyDebugUtilsMessenger(
        renderInstance->instance, renderInstance->debugMessenger, NULL);
  }
#endif

  if (renderInstance->instance != VK_NULL_HANDLE)
  {
    vkDestroyInstance(renderInstance->instance, NULL);
  }

  free(renderInstance);
}

void render_instance_draw(RenderInstance* renderInstance)
{
  uint32_t image;
  if (!render_swapchain_get_next_image(renderInstance->swapchain,
          renderInstance->logicalDevice,
          renderInstance->frames[renderInstance->currentFrame].inflightFence,
          renderInstance->frames[renderInstance->currentFrame]
              .imageAvailableSemaphore,
          &image))
  {
    fprintf(stderr, "WARN: Swapchain was invalid, but recreation is not "
                    "implemented yet.\n");
    // TODO: Try to recreate the swapchain.
    return;
  }

  for (uint32_t i = 0; i < renderInstance->frames[renderInstance->currentFrame]
                               .perRenderBuffersCount;
       i++)
  {
    gpu_buffer_free(&renderInstance->frames[renderInstance->currentFrame]
                         .perRenderBuffers[i],
        renderInstance->logicalDevice);
  }
  renderInstance->frames[renderInstance->currentFrame].perRenderBuffersCount =
      0;

  VkQueue graphicsQueue;
  vkGetDeviceQueue(renderInstance->logicalDevice,
      renderInstance->graphicsQueueFamily, 0, &graphicsQueue);

  render_frame_draw(&renderInstance->frames[renderInstance->currentFrame],
      &renderInstance->swapchain->renderStacks[image], &renderInstance->command,
      &renderInstance->gBufferPipeline, &renderInstance->pbrPipeline,
      renderInstance->fullscreenQuad, &renderInstance->cameraPosition,
      renderInstance->swapchain->extents, renderInstance->renderPass,
      graphicsQueue, renderInstance->physicalDevice,
      renderInstance->logicalDevice);

  VkPresentInfoKHR presentInfo = {.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .pWaitSemaphores = &renderInstance->frames[renderInstance->currentFrame]
                              .renderFinishedSemaphore,
      .waitSemaphoreCount = 1,
      .pSwapchains        = &renderInstance->swapchain->swapchain,
      .swapchainCount     = 1,
      .pImageIndices      = &image};

  VkQueue presentQueue;
  vkGetDeviceQueue(renderInstance->logicalDevice,
      renderInstance->presentQueueFamily, 0, &presentQueue);

  if (vkQueuePresentKHR(presentQueue, &presentInfo) != VK_SUCCESS)
  {
    fprintf(stderr, "Unable to present frame\n");
    // TODO: Should try to reset the swapchain.
  }

  renderInstance->currentFrame =
      (renderInstance->currentFrame + 1) % renderInstance->framesInFlight;
}

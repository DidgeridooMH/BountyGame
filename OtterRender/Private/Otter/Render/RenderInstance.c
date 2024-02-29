#include "Otter/Render/RenderInstance.h"

#define VK_VALIDATION_LAYER_NAME   "VK_LAYER_KHRONOS_validation"
#define REQUESTED_NUMBER_OF_FRAMES 3

static bool renderer_check_layers_available(
    const char** requestedLayers, uint32_t layerCount)
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
    bool layerFound = false;
    for (uint32_t i = 0; i < propertyCount; i++)
    {
      if (strcmp(properties[i].layerName, requestedLayers[layer]) == 0)
      {
        layerFound = true;
        break;
      }
    }

    if (!layerFound)
    {
      free(properties);
      return false;
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

  const char* requiredExtensions[] = {VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
      VK_KHR_SURFACE_EXTENSION_NAME, VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
  const char* requiredLayers[]     = {VK_VALIDATION_LAYER_NAME};
  int requiredExtensionCount =
      (sizeof(requiredExtensions) / sizeof(requiredExtensions[0])) - 1;
  int requiredLayerCount =
      (sizeof(requiredLayers) / sizeof(requiredLayers[0])) - 1;
#ifdef _DEBUG
  if (renderer_check_layers_available(
          requiredLayers, sizeof(requiredLayers) / sizeof(requiredLayers[0])))
  {
    requiredExtensionCount += 1;
    requiredLayerCount += 1;
  }
#endif

  VkInstanceCreateInfo createInfo = {
      .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo        = &applicationInfo,
      .enabledExtensionCount   = requiredExtensionCount,
      .ppEnabledExtensionNames = requiredExtensions,
      .enabledLayerCount       = requiredLayerCount,
      .ppEnabledLayerNames     = requiredLayers};

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

bool render_instance_create_logical_device(RenderInstance* renderInstance)
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

  VkSurfaceFormatKHR* formats = calloc(formatCount, sizeof(VkSurfaceFormatKHR));
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

  VkFormat format = (formats[0].format == VK_FORMAT_UNDEFINED)
                      ? VK_FORMAT_B8G8R8A8_UNORM
                      : formats[0].format;

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

  renderInstance->framesInFlight =
      min(renderInstance->swapchain->numOfSwapchainImages,
          REQUESTED_NUMBER_OF_FRAMES);

  return true;
}

static bool render_instance_create_render_pass(RenderInstance* renderInstance)
{
  VkAttachmentDescription colorAttachment = {
      .format        = renderInstance->swapchain->format,
      .samples       = VK_SAMPLE_COUNT_1_BIT,
      .loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp       = VK_ATTACHMENT_STORE_OP_STORE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
  };

  VkSubpassDependency dependency = {
      .srcSubpass   = VK_SUBPASS_EXTERNAL,
      .dstSubpass   = 0,
      .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
  };

  VkAttachmentReference colorAttachmentRef = {
      .attachment = 0,
      .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  };

  VkSubpassDescription subpass = {
      .pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .colorAttachmentCount = 1,
      .pColorAttachments    = &colorAttachmentRef,
  };

  VkRenderPassCreateInfo renderPassInfo = {
      .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .attachmentCount = 1,
      .pAttachments    = &colorAttachment,
      .subpassCount    = 1,
      .pSubpasses      = &subpass,
      .dependencyCount = 1,
      .pDependencies   = &dependency,
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

  if (!render_swapchain_create_framebuffers(renderInstance->swapchain,
          renderInstance->logicalDevice, renderInstance->renderPass))
  {
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

#ifdef _DEBUG
  if (!render_instance_attach_debug_messenger(renderInstance))
  {
    fprintf(stderr, "WARN: Debug messenger could not be attached. Continuing "
                    "without...\n");
    return NULL;
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

  return renderInstance;
}

void render_instance_destroy(RenderInstance* renderInstance)
{
  // TODO: Wait for frame to finish.

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

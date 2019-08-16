/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Lucurious Labs
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <lucom.h>
#include <wlu/vlucur/vkall.h>
#include <wlu/utils/log.h>
#include <vlucur/utils.h>
#include <vlucur/devices.h>

static void set_values(vkcomp *app) {
  app->dbg_create_report_callback = VK_NULL_HANDLE;
  app->dbg_destroy_report_callback = VK_NULL_HANDLE;
  app->debug_messenger = VK_NULL_HANDLE;
  app->debug_report_callbacks = VK_NULL_HANDLE;
  app->instance = VK_NULL_HANDLE;
  app->surface = VK_NULL_HANDLE;
  app->vk_layer_props = NULL;
  app->vk_layer_count = VK_NULL_HANDLE;
  app->ep_instance_props = NULL;
  app->ep_instance_count = VK_NULL_HANDLE;
  app->ep_device_props = NULL;
  app->ep_device_count = VK_NULL_HANDLE;
  // app->device_properties;
  // app->device_features;
  // app->memory_properties;
  app->physical_device = VK_NULL_HANDLE;
  app->queue_create_infos = NULL;
  app->queue_families = NULL;
  app->queue_family_count = VK_NULL_HANDLE;
  app->indices.graphics_family = UINT32_MAX;
  app->indices.present_family = UINT32_MAX;
  app->device = VK_FALSE;
  app->graphics_queue = VK_NULL_HANDLE;
  app->present_queue = VK_NULL_HANDLE;
  app->sc_buffs = VK_NULL_HANDLE;
  app->swap_chain = VK_NULL_HANDLE;
  app->sc_buff_size = VK_NULL_HANDLE;
  app->pipeline_layout = VK_NULL_HANDLE;
  app->render_pass = VK_NULL_HANDLE;
  app->sc_frame_buffs = VK_NULL_HANDLE;
  app->cmd_pool = VK_NULL_HANDLE;
  app->cmd_buffs = VK_NULL_HANDLE;
  app->img_semaphore = VK_NULL_HANDLE;
  app->render_semaphore = VK_NULL_HANDLE;
  app->depth.view = VK_NULL_HANDLE;
  app->depth.image = VK_NULL_HANDLE;
  app->depth.mem = VK_NULL_HANDLE;
  app->desc_count = VK_NULL_HANDLE;
  app->desc_layout = VK_NULL_HANDLE;
  app->desc_pool = VK_NULL_HANDLE;
  app->desc_set = VK_NULL_HANDLE;
}

vkcomp *wlu_init_vk() {
  vkcomp *app = calloc(sizeof(vkcomp), sizeof(vkcomp));
  if (!app) return NULL;
  set_values(app);
  return app;
}

/*
 * Gets all you're validation layers extensions
 * that comes installed with the vulkan sdk
 */
VkResult wlu_set_global_layers(vkcomp *app) {
  uint32_t layer_count = 0;
  VkLayerProperties *vk_props = NULL;
  VkResult res = VK_INCOMPLETE;

  do {
    res = vkEnumerateInstanceLayerProperties(&layer_count, NULL);
    if (res) {
      wlu_log_me(WLU_DANGER, "[x] vkEnumerateInstanceLayerProperties, ERROR CODE: %d", res);
      goto finish_vk_props;
    }

    /* layer count will only be zero if vulkan sdk not installed */
    if (layer_count == 0) {
      wlu_log_me(WLU_WARNING, "[x] failed to find Validation Layers, layer_count equals 0");
      goto finish_vk_props;
    }

    vk_props = (VkLayerProperties *) realloc(vk_props, layer_count * sizeof(VkLayerProperties));
    if (!vk_props) {
      res = VK_RESULT_MAX_ENUM;
      wlu_log_me(WLU_DANGER, "[x] realloc VkLayerProperties *vk_props failed");
      goto finish_vk_props;
    }

    res = vkEnumerateInstanceLayerProperties(&layer_count, vk_props);
  } while (res == VK_INCOMPLETE);

  app->vk_layer_props = (VkLayerProperties *) \
    calloc(sizeof(VkLayerProperties), layer_count * sizeof(VkLayerProperties));
  if (!app->vk_layer_props) {
    res = VK_RESULT_MAX_ENUM;
    wlu_log_me(WLU_DANGER, "[x] calloc for app->vk_layer_props failed");
    goto finish_vk_props;
  }

  /* Gather the extension list for each instance layer. */
  for (uint32_t i = 0; i < layer_count; i++) {
    res = get_extension_properties(NULL, &vk_props[i], NULL);
    if (res) {
      wlu_log_me(WLU_DANGER, "[x] get_extension_properties failed, ERROR CODE: %d", res);
      goto finish_vk_props;
    }
    app->vk_layer_props[i] = vk_props[i];
    app->vk_layer_count = i;
  }

finish_vk_props:
  if (vk_props) {
    free(vk_props);
    vk_props = NULL;
  }
  return res;
}

VkResult wlu_create_instance(
  vkcomp *app,
  char *app_name,
  char *engine_name,
  uint32_t enabledLayerCount,
  const char* const* ppEnabledLayerNames,
  uint32_t enabledExtensionCount,
  const char* const* ppEnabledExtensionNames
) {

  VkResult res = VK_RESULT_MAX_ENUM;

  /* initialize the VkApplicationInfo structure */
  VkApplicationInfo app_info = {};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pNext = NULL;
  app_info.pApplicationName = app_name;
  app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.pEngineName = engine_name;
  app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.apiVersion = VK_API_VERSION_1_0;

  /* tells the Vulkan driver which instance extensions
    and global validation layers we want to use */
  VkInstanceCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.pApplicationInfo = &app_info;
  create_info.enabledLayerCount = enabledLayerCount;
  create_info.ppEnabledLayerNames = ppEnabledLayerNames;
  create_info.enabledExtensionCount = enabledExtensionCount;
  create_info.ppEnabledExtensionNames = ppEnabledExtensionNames;

  /* Create the instance */
  res = vkCreateInstance(&create_info, NULL, &app->instance);
  if (res) {
    wlu_log_me(WLU_DANGER, "[x] vkCreateInstance failed, ERROR CODE: %d", res);
    return res;
  }

  res = get_extension_properties(app, NULL, NULL);
  if (res) {
    wlu_log_me(WLU_DANGER, "[x] get_extension_properties failed, ERROR CODE: %d", res);
    return res;
  }

  return res;
}

/* Get user physical device */
VkResult wlu_enumerate_devices(vkcomp *app, VkPhysicalDeviceType vkpdtype) {
  VkResult res = VK_RESULT_MAX_ENUM;
  VkPhysicalDevice *devices = VK_NULL_HANDLE;
  uint32_t device_count = 0;

  if (!app->instance) {
    wlu_log_me(WLU_DANGER, "[x] A VkInstance must be established");
    wlu_log_me(WLU_DANGER, "[x] Must make a call to wlu_create_instance(3)");
    wlu_log_me(WLU_DANGER, "[x] See man pages for further details");
    goto finish_devices;
  }

  res = vkEnumeratePhysicalDevices(app->instance, &device_count, NULL);
  if (res) {
    wlu_log_me(WLU_DANGER, "[x] vkEnumeratePhysicalDevices failed, ERROR CODE: %d", res);
    goto finish_devices;
  }

  if (device_count == 0) {
    res = VK_RESULT_MAX_ENUM;
    wlu_log_me(WLU_DANGER, "[x] failed to find GPUs with Vulkan support!!! device_count equals 0");
    goto finish_devices;
  }

  devices = (VkPhysicalDevice *) calloc(sizeof(VkPhysicalDevice),
      device_count * sizeof(VkPhysicalDevice));
  if (!devices) {
    res = VK_RESULT_MAX_ENUM;
    wlu_log_me(WLU_DANGER, "[x] calloc VkPhysicalDevice *devices failed");
    goto finish_devices;
  }

  res = vkEnumeratePhysicalDevices(app->instance, &device_count, devices);
  if (res) {
    wlu_log_me(WLU_DANGER, "[x] vkEnumeratePhysicalDevices failed, ERROR CODE: %d", res);
    goto finish_devices;
  }

  /*
  * get a physical device that is suitable
  * to do the graphics related task that we need
  */
  for (uint32_t i = 0; i < device_count; i++) {
    if (is_device_suitable(app, devices[i], vkpdtype) &&
        /* Check if current device has swap chain support */
        get_extension_properties(app, NULL, devices[i])) {
      memcpy(&app->physical_device, &devices[i], sizeof(devices[i]));
      /* Query device properties */
      vkGetPhysicalDeviceProperties(app->physical_device, &app->device_properties);
      wlu_log_me(WLU_SUCCESS, "Suitable GPU Found: %s", app->device_properties.deviceName);
      break;
    }
  }

  if (app->physical_device == VK_NULL_HANDLE) {
    res = VK_RESULT_MAX_ENUM;
    wlu_log_me(WLU_DANGER, "[x] failed to find a suitable GPU!!!");
    goto finish_devices;
  }

finish_devices:
  if (devices) {
    free(devices);
    devices = NULL;
  }
  return res;
}

VkResult wlu_create_logical_device(
  vkcomp *app,
  uint32_t enabledLayerCount,
  const char* const* ppEnabledLayerNames,
  uint32_t enabledExtensionCount,
  const char* const* ppEnabledExtensionNames
) {

  VkResult res = VK_RESULT_MAX_ENUM;
  float queue_priorities[1] = {1.0};

  if (!app->physical_device) {
    wlu_log_me(WLU_DANGER, "[x] A physical device must be set");
    wlu_log_me(WLU_DANGER, "[x] Must make a call to wlu_enumerate_devices(3)");
    wlu_log_me(WLU_DANGER, "[x] See man pages for further details");
    return res;
  }

  if (!app->queue_families) {
    wlu_log_me(WLU_DANGER, "[x] At least one queue family should be set");
    wlu_log_me(WLU_DANGER, "[x] Must make a call to wlu_set_queue_family(3)");
    wlu_log_me(WLU_DANGER, "[x] See man pages for further details");
    return res;
  }

  app->queue_create_infos = (VkDeviceQueueCreateInfo *) calloc(sizeof(VkDeviceQueueCreateInfo),
      app->queue_family_count * sizeof(VkDeviceQueueCreateInfo));
  if (!app->queue_create_infos) {
    wlu_log_me(WLU_DANGER, "[x] calloc app->queue_create_infos failed");
    return VK_RESULT_MAX_ENUM;
  }

  /* For creation of the presentation queue */
  for (uint32_t i = 0; i < app->queue_family_count; i++) {
    app->queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    app->queue_create_infos[i].pNext = NULL;
    app->queue_create_infos[i].flags = 0;
    app->queue_create_infos[i].queueFamilyIndex = app->indices.present_family;
    app->queue_create_infos[i].queueCount = app->queue_family_count;
    app->queue_create_infos[i].pQueuePriorities = queue_priorities;
  }

  VkDeviceCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.queueCreateInfoCount = app->queue_create_infos[0].queueCount;
  create_info.pQueueCreateInfos = app->queue_create_infos;
  create_info.enabledLayerCount = enabledLayerCount;
  create_info.ppEnabledLayerNames = ppEnabledLayerNames;
  create_info.enabledExtensionCount = enabledExtensionCount;
  create_info.ppEnabledExtensionNames = ppEnabledExtensionNames;
  create_info.pEnabledFeatures = &app->device_features;

  /* Create logic device */
  res = vkCreateDevice(app->physical_device, &create_info, NULL, &app->device);
  if (res) {
    wlu_log_me(WLU_DANGER, "[x] vkCreateDevice failed, ERROR CODE: %d", res);
    return res;
  }

  /*
   * Queues are automatically created with
   * the logical device, but you need a queue
   * handle to interface with them
   */
  if (app->indices.graphics_family == UINT32_MAX)
    vkGetDeviceQueue(app->device, app->indices.graphics_family, 0, &app->graphics_queue);
  if (app->indices.present_family == UINT32_MAX)
    vkGetDeviceQueue(app->device, app->indices.present_family, 0, &app->present_queue);

  return res;
}

VkResult wlu_create_swap_chain(
  vkcomp *app,
  VkSurfaceCapabilitiesKHR capabilities,
  VkSurfaceFormatKHR surface_fmt,
  VkPresentModeKHR pres_mode,
  VkExtent2D extent2D,
  VkExtent3D extent3D
) {

  VkResult res = VK_RESULT_MAX_ENUM;

  if (!app->surface) {
    wlu_log_me(WLU_DANGER, "[x] app->surface must be initialize");
    wlu_log_me(WLU_DANGER, "[x] Must make a call to wlu_vkconnect_surfaceKHR(3)");
    wlu_log_me(WLU_DANGER, "[x] See man pages for further details");
    return res;
  }

  if (!app->device) {
    wlu_log_me(WLU_DANGER, "[x] app->device must be initialize");
    wlu_log_me(WLU_DANGER, "[x] Must make a call to wlu_create_logical_device(3)");
    wlu_log_me(WLU_DANGER, "[x] See man pages for further details");
    return res;
  }

  /*
   * Don't want to stick to minimum becuase one would have to wait on the
   * drive to complete internal operations before one can acquire another
   * images to render to. So it's recommended to add one to minImageCount
   */
  app->sc_buff_size = capabilities.minImageCount + 1;

  /* Be sure sc_buff_size doesn't exceed the maximum. */
  if (capabilities.maxImageCount > 0 && app->sc_buff_size > capabilities.maxImageCount)
    app->sc_buff_size = capabilities.maxImageCount;

  VkSurfaceTransformFlagBitsKHR pre_transform;
  pre_transform = (capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) ? \
      VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : capabilities.currentTransform;

  VkCompositeAlphaFlagBitsKHR composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  VkCompositeAlphaFlagBitsKHR composite_alpha_flags[4] = {
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
      VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
      VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
  };

  for (uint32_t i = 0; i < sizeof(composite_alpha_flags); i++) {
    if (capabilities.supportedCompositeAlpha & composite_alpha_flags[i]) {
      composite_alpha = composite_alpha_flags[i];
      break;
    }
  }

  VkSwapchainCreateInfoKHR create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_info.pNext = NULL;
  create_info.flags = VK_SWAPCHAIN_CREATE_MUTABLE_FORMAT_BIT_KHR;
  create_info.surface = app->surface;
  create_info.minImageCount = app->sc_buff_size;
  create_info.imageFormat = surface_fmt.format;
  create_info.imageColorSpace = surface_fmt.colorSpace;
  if (extent2D.width != UINT32_MAX) {
    create_info.imageExtent.width = extent2D.width;
    create_info.imageExtent.height = extent2D.height;
  } else {
    create_info.imageExtent.width = extent3D.width;
    create_info.imageExtent.height = extent3D.height;
  }
  create_info.imageArrayLayers = 1;
  create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  /* current transform should be applied to images in the swap chain */
  create_info.preTransform = pre_transform;
  /* specify that I currently do not want any transformation */
  create_info.compositeAlpha = composite_alpha;
  create_info.presentMode = pres_mode;
  create_info.clipped = VK_FALSE;
  create_info.oldSwapchain = VK_NULL_HANDLE;

  if (app->indices.graphics_family != app->indices.present_family) {
    const uint32_t queue_family_indices[2] = {
      app->indices.graphics_family,
      app->indices.present_family
    };
    create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    create_info.queueFamilyIndexCount = 2;
    create_info.pQueueFamilyIndices = queue_family_indices;
  } else {
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 0;
    create_info.pQueueFamilyIndices = NULL;
  }

  res = vkCreateSwapchainKHR(app->device, &create_info, NULL, &app->swap_chain);
  if (res) {
    wlu_log_me(WLU_DANGER, "[x] vkCreateSwapchainKHR failed, ERROR CODE: %d", res);
    return res;
  }

  return res;
}

VkResult wlu_create_img_views(vkcomp *app, VkFormat format, VkImageViewType type) {
  VkResult res = VK_RESULT_MAX_ENUM;
  VkImage *sc_imgs = NULL;

  if (!app->swap_chain) {
    wlu_log_me(WLU_DANGER, "[x] Swap Chain doesn't exists");
    wlu_log_me(WLU_DANGER, "[x] Must make a call to wlu_create_swap_chain(3)");
    wlu_log_me(WLU_DANGER, "[x] See man pages for further details");
    goto finish_create_img_views;
  }

  app->sc_buffs = (swap_chain_buffers *) calloc(sizeof(swap_chain_buffers),
      app->sc_buff_size * sizeof(swap_chain_buffers));
  if (!app->sc_buffs) {
    wlu_log_me(WLU_DANGER, "[x] calloc app->sc_buffs failed");
    goto finish_create_img_views;
  }

  res = vkGetSwapchainImagesKHR(app->device, app->swap_chain, &app->sc_buff_size, NULL);
  if (res) {
    wlu_log_me(WLU_DANGER, "[x] vkGetSwapchainImagesKHR failed, ERROR CODE: %d", res);
    goto finish_create_img_views;
  }

  sc_imgs = (VkImage *) calloc(sizeof(VkImage), app->sc_buff_size * sizeof(VkImage));
  if (!sc_imgs) {
    res = VK_RESULT_MAX_ENUM;
    wlu_log_me(WLU_DANGER, "[x] calloc VkImage *sc_imgs failed");
    goto finish_create_img_views;
  }

  res = vkGetSwapchainImagesKHR(app->device, app->swap_chain, &app->sc_buff_size, sc_imgs);
  if (res) {
    wlu_log_me(WLU_DANGER, "[x] vkGetSwapchainImagesKHR failed, ERROR CODE: %d", res);
    goto finish_create_img_views;
  }

  for (uint32_t i = 0; i < app->sc_buff_size; i++) {
    VkImageViewCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image = app->sc_buffs[i].image = sc_imgs[i];
    create_info.viewType = type;
    create_info.format = format;
    create_info.components.r = VK_COMPONENT_SWIZZLE_R;
    create_info.components.g = VK_COMPONENT_SWIZZLE_G;
    create_info.components.b = VK_COMPONENT_SWIZZLE_B;
    create_info.components.a = VK_COMPONENT_SWIZZLE_A;
    /* describe what the image’s purpose is and which
        part of the image should be accessed */
    create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount = 1;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount = 1;

    res = vkCreateImageView(app->device, &create_info, NULL, &app->sc_buffs[i].view);
    if (res) {
      wlu_log_me(WLU_DANGER, "[x] vkCreateImageView failed, ERROR CODE: %d", res);
      goto finish_create_img_views;
    }
  }

finish_create_img_views:
  if (sc_imgs) {
    free(sc_imgs);
    sc_imgs = NULL;
  }
  return res;
}

VkResult wlu_create_depth_buff(
  vkcomp *app,
  VkFormat depth_format,
  VkFormatFeatureFlags linearTilingFeatures,
  VkFormatFeatureFlags optimalTilingFeatures,
  VkImageType imageType,
  VkExtent3D extent,
  VkImageUsageFlags usage,
  VkSharingMode sharingMode,
  VkImageLayout initialLayout,
  VkImageViewType viewType
) {

  VkResult res = VK_RESULT_MAX_ENUM;
  VkBool32 pass;

  app->depth.format = depth_format;

  VkImageCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.imageType = imageType;
  create_info.format = app->depth.format;
  create_info.extent.width = extent.width;
  create_info.extent.height = extent.height;
  create_info.extent.depth = extent.depth;
  create_info.mipLevels = 1;
  create_info.arrayLayers = 1;
  create_info.samples = VK_SAMPLE_COUNT_1_BIT;

  VkFormatProperties props;
  vkGetPhysicalDeviceFormatProperties(app->physical_device, app->depth.format, &props);
  if (props.linearTilingFeatures & linearTilingFeatures) {
    create_info.tiling = VK_IMAGE_TILING_LINEAR;
  } else if (props.optimalTilingFeatures & optimalTilingFeatures) {
    create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  } else {
    wlu_log_me(WLU_DANGER, "[x] Depth format currently not supported.\n");
    return res;
  }

  create_info.usage = usage;
  create_info.sharingMode = sharingMode;
  /* Come back to me */
  create_info.queueFamilyIndexCount = 0;
  create_info.pQueueFamilyIndices = NULL;
  /* Come back to me */
  create_info.initialLayout = initialLayout;

  VkMemoryAllocateInfo mem_alloc = {};
  mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  mem_alloc.pNext = NULL;
  mem_alloc.allocationSize = 0;
  mem_alloc.memoryTypeIndex = 0;

  VkImageViewCreateInfo create_view_info = {};
  create_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  create_view_info.pNext = NULL;
  create_view_info.flags = 0;
  create_view_info.image = VK_NULL_HANDLE;
  create_view_info.format = app->depth.format;
  create_view_info.components.r = VK_COMPONENT_SWIZZLE_R;
  create_view_info.components.g = VK_COMPONENT_SWIZZLE_G;
  create_view_info.components.b = VK_COMPONENT_SWIZZLE_B;
  create_view_info.components.a = VK_COMPONENT_SWIZZLE_A;
  create_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  create_view_info.subresourceRange.baseMipLevel = 0;
  create_view_info.subresourceRange.levelCount = 1;
  create_view_info.subresourceRange.baseArrayLayer = 0;
  create_view_info.subresourceRange.layerCount = 1;
  create_view_info.viewType = viewType;

  if (app->depth.format == VK_FORMAT_D16_UNORM_S8_UINT ||
      app->depth.format == VK_FORMAT_D24_UNORM_S8_UINT ||
      app->depth.format == VK_FORMAT_D32_SFLOAT_S8_UINT)
      create_view_info.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

  VkMemoryRequirements mem_reqs;

  /* Create image object */
  res = vkCreateImage(app->device, &create_info, NULL, &app->depth.image);
  if (res) {
    wlu_log_me(WLU_DANGER, "[x] vkCreateImage failed, ERROR CODE: %d", res);
    return res;
  }

  /* Although you know the width, height, and the size of a buffer element,
   * there is no way to determine exactly how much memory is needed to allocate.
   * This is because alignment constraints that may be placed by the GPU hardware.
   * This function allows you to find out everything you need to allocate the
   * memory for an image.
   */
  vkGetImageMemoryRequirements(app->device, app->depth.image, &mem_reqs);

  mem_alloc.allocationSize = mem_reqs.size;
  /* Use the memory properties to determine the type of memory required */
  pass = memory_type_from_properties(app, mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &mem_alloc.memoryTypeIndex);
  if (!pass) {
    wlu_log_me(WLU_DANGER, "[x] memory_type_from_properties failed");
    return pass;
  }

  /* Allocate memory */
  res = vkAllocateMemory(app->device, &mem_alloc, NULL, &app->depth.mem);
  if (res) {
    wlu_log_me(WLU_DANGER, "[x] vkAllocateMemory failed, ERROR CODE: %d", res);
    return res;
  }

  /* Associate memory with image object by binding */
  res = vkBindImageMemory(app->device, app->depth.image, app->depth.mem, 0);
  if (res) {
    wlu_log_me(WLU_DANGER, "[x] vkBindImageMemory failed, ERROR CODE: %d", res);
    return res;
  }

  /* Create image view object */
  create_view_info.image = app->depth.image;
  res = vkCreateImageView(app->device, &create_view_info, NULL, &app->depth.view);
  if (res) {
    wlu_log_me(WLU_DANGER, "[x] vkCreateImageView failed, ERROR CODE: %d", res);
    return res;
  }

  return res;
}

VkResult wlu_create_uniform_buff(vkcomp *app, VkBufferCreateFlagBits flags, VkBufferUsageFlags usage) {
  VkResult res = VK_RESULT_MAX_ENUM;
  bool pass = false;

  if (!app->mvp) {
    wlu_log_me(WLU_DANGER, "[x] Modal, View, Projection not setup");
    wlu_log_me(WLU_DANGER, "[x] Must make a call to wlu_set_mvp_matrix(3)");
    wlu_log_me(WLU_DANGER, "[x] See man pages for further details");
    return res;
  }

  VkBufferCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = flags;
  create_info.size = sizeof(app->mvp);
  create_info.usage = usage;
  create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  create_info.queueFamilyIndexCount = 0;
  create_info.pQueueFamilyIndices = NULL;

  res = vkCreateBuffer(app->device, &create_info, NULL, &app->uniform_data.buff);
  if (res) {
    wlu_log_me(WLU_DANGER, "[x] vkCreateBuffer failed, ERROR CODE: %d", res);
    return res;
  }

  VkMemoryRequirements mem_reqs;
  vkGetBufferMemoryRequirements(app->device, app->uniform_data.buff, &mem_reqs);

  VkMemoryAllocateInfo alloc_info = {};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.pNext = NULL;
  alloc_info.allocationSize = mem_reqs.size;
  alloc_info.memoryTypeIndex = 0;

  /*
   * Can Find in vulkan SDK doc/tutorial/html/07-init_uniform_buffer.html
   * The VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT communicates that the memory
   * should be mapped so that the CPU (host) can access it.
   * The VK_MEMORY_PROPERTY_HOST_COHERENT_BIT requests that the
   * writes to the memory by the host are visible to the device
   * (and vice-versa) without the need to flush memory caches.
   */
  pass = memory_type_from_properties(app, mem_reqs.memoryTypeBits,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                     &alloc_info.memoryTypeIndex);
  if (!pass) {
    wlu_log_me(WLU_DANGER, "[x] memory_type_from_properties failed");
    return pass;
  }

  res = vkAllocateMemory(app->device, &alloc_info, NULL, &app->uniform_data.mem);
  if (res) {
    wlu_log_me(WLU_DANGER, "[x] vkAllocateMemory failed, ERROR CODE: %d", res);
    return res;
  }

  /*
   * Can Find in vulkan SDK doc/tutorial/html/07-init_uniform_buffer.html
   * With a uniform buffer, you need to populate it with the data that
   * you want the shader to read. In this case, the data is the MVP matrix.
   * In order to get CPU access to the memory, you need to map it
   */
  uint8_t *p_data;
  res = vkMapMemory(app->device, app->uniform_data.mem, 0, mem_reqs.size, 0, (void **) &p_data);
  if (res) {
    wlu_log_me(WLU_DANGER, "[x] vkMapMemory failed, ERROR CODE: %d", res);
    return res;
  }

  memcpy(p_data, &app->mvp, sizeof(app->mvp));

  vkUnmapMemory(app->device, app->uniform_data.mem);

  /* associate the memory allocated with the buffer object */
  res = vkBindBufferMemory(app->device, app->uniform_data.buff, app->uniform_data.mem, 0);
  if (res) {
    wlu_log_me(WLU_DANGER, "[x] vkBindBufferMemory failed, ERROR CODE: %d", res);
    return res;
  }

  app->uniform_data.buff_info.buffer = app->uniform_data.buff;
  app->uniform_data.buff_info.offset = 0;
  app->uniform_data.buff_info.range = sizeof(app->mvp);

  return res;
}

/*
 * This function creates the framebuffers.
 * Attachments specified when creating the render pass
 * are bounded by wrapping them into a VkFramebuffer object.
 * A framebuffer object references all VkImageView objects this
 * is represented by "attachments"
 */
VkResult wlu_create_framebuffers(vkcomp *app, uint32_t attachment_count, VkExtent2D extent, uint32_t layers) {
  VkResult res = VK_RESULT_MAX_ENUM;

  if (!app->render_pass) {
    wlu_log_me(WLU_DANGER, "[x] render pass not setup");
    wlu_log_me(WLU_DANGER, "[x] Must make a call to wlu_create_render_pass(3)");
    wlu_log_me(WLU_DANGER, "[x] See man pages for further details");
    return res;
  }

  if (!app->sc_buffs) {
    wlu_log_me(WLU_DANGER, "[x] Swap Chain buffers not setup");
    wlu_log_me(WLU_DANGER, "[x] Must make a call to wlu_create_img_views(3)");
    wlu_log_me(WLU_DANGER, "[x] See man pages for further details");
    return res;
  }

  app->sc_frame_buffs = (VkFramebuffer *) calloc(sizeof(VkFramebuffer),
        app->sc_buff_size * sizeof(VkFramebuffer));
  if (!app->sc_frame_buffs) {
    wlu_log_me(WLU_DANGER, "[x] calloc VkFramebuffer *sc_frame_buffs failed");
    return res;
  }

  VkImageView attachments[app->sc_buff_size];

  for (uint32_t i = 0; i < app->sc_buff_size; i++) {
    attachments[i] = app->sc_buffs[i].view;

    VkFramebufferCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    create_info.renderPass = app->render_pass;
    create_info.attachmentCount = attachment_count;
    create_info.pAttachments = attachments;
    create_info.width = extent.width;
    create_info.height = extent.height;
    create_info.layers = layers;

    res = vkCreateFramebuffer(app->device, &create_info, NULL, &app->sc_frame_buffs[i]);
    if (res) {
      wlu_log_me(WLU_DANGER, "[x] vkCreateFramebuffer failed, ERROR CODE: %d", res);
      return res;
    }
  }

  wlu_log_me(WLU_SUCCESS, "Frame Buffers have been successfully created");

  return res;
}

VkResult wlu_create_cmd_pool(vkcomp *app, VkCommandPoolCreateFlagBits flags) {
  VkResult res = VK_RESULT_MAX_ENUM;

  if (app->indices.graphics_family == UINT32_MAX || app->indices.present_family == UINT32_MAX) {
    wlu_log_me(WLU_DANGER, "[x] graphics or present family index not set");
    wlu_log_me(WLU_DANGER, "[x] Must make a call to wlu_set_queue_family(3)");
    wlu_log_me(WLU_DANGER, "[x] See man pages for further details");
    return res;
  }

  VkCommandPoolCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = flags;
  create_info.queueFamilyIndex = app->indices.graphics_family;

  res = vkCreateCommandPool(app->device, &create_info, NULL, &app->cmd_pool);

  return res;
}

VkResult wlu_create_cmd_buffs(vkcomp *app, VkCommandBufferLevel level) {
  VkResult res = VK_RESULT_MAX_ENUM;

  app->cmd_buffs = (VkCommandBuffer *) calloc(sizeof(VkCommandBuffer),
        app->sc_buff_size * sizeof(VkCommandBuffer));
  if (!app->cmd_buffs) {
    wlu_log_me(WLU_DANGER, "[x] calloc VkCommandBuffer *cmd_buffs failed");
    return res;
  }

  VkCommandBufferAllocateInfo alloc_info = {};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.pNext = NULL;
  alloc_info.commandPool = app->cmd_pool;
  alloc_info.level = level;
  alloc_info.commandBufferCount = (uint32_t) app->sc_buff_size;

  res = vkAllocateCommandBuffers(app->device, &alloc_info, app->cmd_buffs);

  return res;
}

VkResult wlu_create_semaphores(vkcomp *app) {
  VkResult res = VK_RESULT_MAX_ENUM;

  VkSemaphoreCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;

  res = vkCreateSemaphore(app->device, &create_info, NULL, &app->img_semaphore);
  if (res) return res;

  res = vkCreateSemaphore(app->device, &create_info, NULL, &app->render_semaphore);

  return res;
}

void wlu_freeup_vk(void *data) {
  vkcomp *app = (vkcomp *) data;

  if (app->debug_report_callbacks)
    free(app->debug_report_callbacks);
  if (app->vk_layer_props)
    free(app->vk_layer_props);
  if (app->ep_instance_props)
    free(app->ep_instance_props);
  if (app->ep_device_props)
    free(app->ep_device_props);
  if (app->queue_families)
    free(app->queue_families);
  if (app->queue_create_infos)
    free(app->queue_create_infos);
  if (app->depth.view)
    vkDestroyImageView(app->device, app->depth.view, NULL);
  if (app->depth.image)
    vkDestroyImage(app->device, app->depth.image, NULL);
  if (app->depth.mem)
    vkFreeMemory(app->device, app->depth.mem, NULL);
  if (app->render_semaphore)
    vkDestroySemaphore(app->device, app->render_semaphore, NULL);
  if (app->img_semaphore)
    vkDestroySemaphore(app->device, app->img_semaphore, NULL);
  if (app->cmd_buffs)
    vkFreeCommandBuffers(app->device, app->cmd_pool, app->sc_buff_size, app->cmd_buffs);
  if (app->cmd_pool)
    vkDestroyCommandPool(app->device, app->cmd_pool, NULL);
  if (app->sc_frame_buffs) {
    for (uint32_t i = 0; i < app->sc_buff_size; i++) {
      vkDestroyFramebuffer(app->device, app->sc_frame_buffs[i], NULL);
      app->sc_frame_buffs[i] = VK_NULL_HANDLE;
    }
    free(app->sc_frame_buffs);
  }
  if (app->graphics_pipeline)
    vkDestroyPipeline(app->device, app->graphics_pipeline, NULL);
  if (app->desc_layout) {
    for (uint32_t i = 0; i < app->desc_count; i++)
      vkDestroyDescriptorSetLayout(app->device, app->desc_layout[i], NULL);
    free(app->desc_layout);
  }
  if (app->desc_set)
    free(app->desc_set);
  if (app->desc_pool)
    vkDestroyDescriptorPool(app->device, app->desc_pool, NULL);
  if (app->uniform_data.buff)
    vkDestroyBuffer(app->device, app->uniform_data.buff, NULL);
  if (app->uniform_data.mem)
    vkFreeMemory(app->device, app->uniform_data.mem, NULL);
  if (app->pipeline_layout)
    vkDestroyPipelineLayout(app->device, app->pipeline_layout, NULL);
  if (app->render_pass)
    vkDestroyRenderPass(app->device, app->render_pass, NULL);
  if (app->sc_buffs) {
    for (uint32_t i = 0; i < app->sc_buff_size; i++) {
      vkDestroyImageView(app->device, app->sc_buffs[i].view, NULL);
      app->sc_buffs[i].view = VK_NULL_HANDLE;
    }
    free(app->sc_buffs);
  }
  if (app->swap_chain)
    vkDestroySwapchainKHR(app->device, app->swap_chain, NULL);
  if (app->device) {
    vkDeviceWaitIdle(app->device);
    vkDestroyDevice(app->device, NULL);
  }
  if (app->surface)
    vkDestroySurfaceKHR(app->instance, app->surface, NULL);
  if (app->instance)
    vkDestroyInstance(app->instance, NULL);

  set_values(app);
  if (app)
    free(app);
  app = NULL;
}

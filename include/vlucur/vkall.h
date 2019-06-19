#ifndef VKALL_H
#define VKALL_H

#include <lucom.h>

#include <cglm/call.h>

#define VK_USE_PLATFORM_WAYLAND_KHR 1
#include <vulkan/vulkan.h>

struct swap_chain_support_details {
  VkSurfaceCapabilitiesKHR capabilities;

  VkSurfaceFormatKHR *formats;
  uint32_t format_count;

  VkPresentModeKHR *present_modes;
  uint32_t pres_mode_count;
};

struct queue_family_indices {
  int graphics_family;
  int present_family;
};

struct vkcomp {
  VkInstance instance;
  VkSurfaceKHR surface;

  /* keep track of all vulkan extensions */
  VkLayerProperties *vk_layer_props;
  uint32_t vk_layer_count;

  VkExtensionProperties *ep_instance_props;
  uint32_t ep_instance_count;

  VkExtensionProperties *ep_device_props;
  uint32_t ep_device_count;

  /* To get device properties like the name, type and supported Vulkan version */
  VkPhysicalDeviceProperties device_properties;
  /* For optional features like texture compression,
    64 bit floats and multi viewport rendering */
  VkPhysicalDeviceFeatures device_features;
  VkPhysicalDeviceMemoryProperties memory_properties;
  VkPhysicalDevice physical_device;
  VkPhysicalDevice *devices;
  uint32_t device_count;

  VkDeviceQueueCreateInfo *queue_create_infos;
  VkQueueFamilyProperties *queue_families;
  uint32_t queue_family_count;
  struct queue_family_indices indices;

  VkDevice device; /* logical device */
  VkQueue graphics_queue;

  struct swap_chain_support_details dets;
  VkSwapchainKHR swap_chain;
  VkImage *swap_chain_imgs;
  VkFormat swap_chain_img_fmt;
  VkExtent2D swap_chain_extent;
  uint32_t image_count;
};

/* Can find in vulkan-sdk samples/API-Samples/utils/util.hpp */
#define GET_INSTANCE_PROC_ADDR(inst, entrypoint)                                    \
  {                                                                                \
    info.fp##entrypoint =                                                          \
        PFN_vk##entrypoint)vkGetInstanceProcAddr(inst, "vk" #entrypoint);          \
    if (info.fp##entrypoint == NULL) {                                             \
      fprintf(stderr, "[x] vkGetDeviceProcAddr failed to find vk %s", #entrypoint); \
      exit(-1);                                                                    \
    }                                                                              \
  }

/* Can find in vulkan-sdk samples/API-Samples/utils/util.hpp */
#define GET_DEVICE_PROC_ADDR(dev, entrypoint)                                          \
  {                                                                                   \
    info.fp##entrypoint =                                                             \
        (PFN_vk##entrypoint)vkGetDeviceProcAddr(dev, "vk" #entrypoint);               \
    if (info.fp##entrypoint == NULL) {                                                \
        fprintf(stderr, "[x] vkGetDeviceProcAddr failed to find vk %s", #entrypoint);  \
        exit(-1);                                                                     \
    }                                                                                 \
  }

/* Function protypes */
struct vkcomp *init_vk();
VkResult set_global_layers(struct vkcomp *app);
VkResult create_instance(struct vkcomp *app, char *app_name, char *engine_name);
VkResult enumerate_devices(struct vkcomp *app);
VkResult set_logical_device(struct vkcomp *app);
VkResult vk_connect_surfaceKHR(struct vkcomp *app, void *wl_display, void *wl_surface);
VkResult create_swap_chain(struct vkcomp *app);
void freeup_vk(void *data);

#endif
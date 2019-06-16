#ifndef VKALL_H
#define VKALL_H

#include <lucom.h>

/* Function protypes */
struct vkcomp *init_vk();
VkResult set_global_layers(struct vkcomp *app);
VkResult create_instance(struct vkcomp *app, char *app_name, char *engine_name);
VkResult enumerate_devices(struct vkcomp *app);
VkResult init_logical_device(struct vkcomp *app);
VkResult vk_connect_surfaceKHR(struct vkcomp *app, void *wl_display, void *wl_surface);
void freeup_vk(void *data);

#endif

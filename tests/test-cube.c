/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 EasyIP2023
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
#include <wlu/wclient/client.h>
#include <wlu/utils/errors.h>
#include <wlu/utils/log.h>
#include <wlu/shader/shade.h>
#include <wlu/vlucur/gp.h>
#include <wlu/vlucur/matrix.h>

#include <signal.h>
#include <check.h>

#include "test-extras.h"
#include "test-shade.h"

#define NUM_DESCRIPTOR_SETS 1
#define WIDTH 800
#define HEIGHT 600
#define DEPTH 1

struct uniform_block_data {
  mat4 proj;
  mat4 view;
  mat4 model;
  mat4 clip;
  mat4 mvp;
} ubd;

void wlu_print_matrices() {
  wlu_log_me(WLU_INFO, "Perspective Matrix");
  wlu_log_me(WLU_INFO, "Projection from camera to screen");
  wlu_print_matrix(ubd.proj, WLU_MAT4);
  wlu_log_me(WLU_INFO, "View Matrix");
  wlu_log_me(WLU_INFO, "View from world space to camera space");
  wlu_print_matrix(ubd.view, WLU_MAT4);
  wlu_log_me(WLU_INFO, "Model Matrix");
  wlu_log_me(WLU_INFO, "Mapping object's local coordinate space into world space");
  wlu_print_matrix(ubd.model, WLU_MAT4);
  wlu_log_me(WLU_INFO, "Clip Matrix");
  wlu_print_matrix(ubd.clip, WLU_MAT4);
  wlu_log_me(WLU_INFO, "MVP Matrix");
  wlu_print_matrix(ubd.mvp, WLU_MAT4);
}

void freeme(vkcomp *app, wclient *wc) {
  wlu_freeup_vk(app);
  wlu_freeup_wc(wc);
  wlu_freeup_watchme();
}

START_TEST(test_vulkan_client_create_3D) {
  VkResult err;

  wclient *wc = wlu_init_wc();
  if (!wc) {
    wlu_log_me(WLU_DANGER, "[x] wlu_init_wc failed!!");
    ck_abort_msg(NULL);
  }

  vkcomp *app = wlu_init_vk();
  if (!app) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_init_vk failed!!");
    ck_abort_msg(NULL);
  }

  /* Signal handler for this process */
  err = wlu_watch_me(SIGSEGV, getpid());
  if (err) {
    freeme(app, wc);
    ck_abort_msg(NULL);
  }

  wlu_add_watchme_info(1, app, 1, wc, 0, NULL);

  err = wlu_set_global_layers(app);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] checking and setting validation layers failed");
    ck_abort_msg(NULL);
  }

  err = wlu_create_instance(app, "Draw Cube", "Desktop Engine", 3, enabled_validation_layers, 4, instance_extensions);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] failed to create vulkan instance");
    ck_abort_msg(NULL);
  }

  err = wlu_set_debug_message(app);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] failed to setup debug message");
    ck_abort_msg(NULL);
  }

  if (wlu_connect_client(wc)) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] failed to connect client");
    ck_abort_msg(NULL);
  }

  /* initialize vulkan app surface */
  err = wlu_vkconnect_surfaceKHR(app, wc->display, wc->surface);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] failed to connect to vulkan surfaceKHR");
    ck_abort_msg(NULL);
  }

  /* This will get the physical device, it's properties, and features */
  VkPhysicalDeviceProperties device_props;
  VkPhysicalDeviceFeatures device_feats;
  err = wlu_create_physical_device(app, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, &device_props, &device_feats);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] failed to find physical device");
    ck_abort_msg(NULL);
  }

  err = wlu_set_queue_family(app, VK_QUEUE_GRAPHICS_BIT);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] failed to set device queue family");
    ck_abort_msg(NULL);
  }

  err = wlu_create_logical_device(app, &device_feats, 3, enabled_validation_layers, 1, device_extensions);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] failed to initialize logical device to physical device");
    ck_abort_msg(NULL);
  }

  VkSurfaceCapabilitiesKHR capabilities = wlu_q_device_capabilities(app);
  if (capabilities.minImageCount == UINT32_MAX) {
    freeme(app, wc);
    ck_abort_msg(NULL);
  }
  /*
   * VK_FORMAT_B8G8R8A8_UNORM will store the B, G, R and alpha channels
   * in that order with an 8 bit unsigned integer and a total of 32 bits per pixel.
   * SRGB if used for colorSpace if available, because it
   * results in more accurate perceived colors
   */
  VkSurfaceFormatKHR surface_fmt = wlu_choose_swap_surface_format(app, VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);
  if (surface_fmt.format == VK_FORMAT_UNDEFINED) {
    freeme(app, wc);
    ck_abort_msg(NULL);
  }

  VkPresentModeKHR pres_mode = wlu_choose_swap_present_mode(app);
  if (pres_mode == VK_PRESENT_MODE_MAX_ENUM_KHR) {
    freeme(app, wc);
    ck_abort_msg(NULL);
  }

  VkExtent3D extent3D = wlu_choose_3D_swap_extent(capabilities, WIDTH, HEIGHT, DEPTH);
  if (extent3D.width == UINT32_MAX) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] choose_swap_extent failed, extent3D.width equals %d", extent3D.width);
    ck_abort_msg(NULL);
  }

  err = wlu_create_swap_chain(app, capabilities, surface_fmt, pres_mode, extent3D.width, extent3D.height);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] failed to create swap chain");
    ck_abort_msg(NULL);
  }

  err = wlu_create_cmd_pool(app, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] failed to create command pool, ERROR CODE: %d", err);
    ck_abort_msg(NULL);
  }

  /* current buff at this point is technically also current image, but they should be different */
  uint32_t cur_buff = 0, cur_sc = 0, cur_pool = 0;
  err = wlu_create_cmd_buffs(app, cur_pool, cur_sc, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] failed to create command buffers, ERROR CODE: %d", err);
    ck_abort_msg(NULL);
  }

  err = wlu_exec_begin_cmd_buffs(app, cur_pool, cur_sc, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT, NULL);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] failed to start command buffer recording");
    ck_abort_msg(NULL);
  }

  err = wlu_create_img_views(app, cur_sc, surface_fmt.format, VK_IMAGE_VIEW_TYPE_2D);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] failed to create image views");
    ck_abort_msg(NULL);
  }

  err = wlu_create_depth_buff(app, cur_sc, VK_FORMAT_D16_UNORM,
    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
    VK_IMAGE_TYPE_2D, extent3D,
    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    VK_SHARING_MODE_EXCLUSIVE, VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_VIEW_TYPE_2D
  );
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_create_depth_buff failed");
    ck_abort_msg(NULL);
  }

  err = wlu_create_semaphores(app, cur_sc);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_create_semaphores failed");
    ck_abort_msg(NULL);
  }

  /* Acquire the swapchain image in order to set its layout */
  err = wlu_retrieve_swapchain_img(app, &cur_buff, cur_sc);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_retrieve_swapchain_img failed");
    ck_abort_msg(NULL);
  }

  float fovy = wlu_set_fovy(45.0f);
  float hw = (float) extent3D.width / (float) extent3D.height;
  if (extent3D.width > extent3D.height) fovy *= hw;
  wlu_set_perspective(ubd.proj, fovy, hw, 0.1f, 100.0f);
  wlu_set_lookat(ubd.view, eye, center, up);
  wlu_set_matrix(ubd.model, model_matrix, sizeof(model_matrix));
  wlu_set_matrix(ubd.clip, clip_matrix, sizeof(clip_matrix));
  wlu_set_mvp_matrix(ubd.mvp, &ubd.clip, &ubd.proj, &ubd.view, &ubd.model);
  wlu_print_matrices();

  /* Create uniform buffer that has the transformation matrices (for the vertex shader) */
  err = wlu_create_buffer(
    app, sizeof(ubd.mvp), ubd.mvp, 0, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    VK_SHARING_MODE_EXCLUSIVE, 0, NULL, "uniform",
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
  );
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_create_buffer failed");
    ck_abort_msg(NULL);
  }

  VkDescriptorSetLayoutBinding desc_set = wlu_set_desc_set(app,
    0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, NUM_DESCRIPTOR_SETS,
    VK_SHADER_STAGE_VERTEX_BIT, NULL
  );

  VkDescriptorSetLayoutCreateInfo desc_set_info = wlu_set_desc_set_info(0, 1, &desc_set);

  err = wlu_create_desc_set_layout(app, &desc_set_info);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_set_desc_set_info failed");
    ck_abort_msg(NULL);
  }

  /* This is where creation of the graphics pipeline begins */
  err = wlu_create_pipeline_layout(app, 0, NULL);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_create_pipeline_layout failed");
    ck_abort_msg(NULL);
  }

  err = wlu_create_desc_set(app, 1, 1, 0, 0, 0,
    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &app->buffs_data[0].buff_info
  );
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_create_desc_set failed");
    ck_abort_msg(NULL);
  }

  /* start of render pass creation */
  wlu_log_me(WLU_INFO, "Start of render pass creation");

  VkAttachmentDescription attachments[2];
  /* Create render pass color attachment for swapchain images */
  attachments[0] = wlu_set_attachment_desc(surface_fmt.format,
    VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
  );

  /* Create render pass stencil/depth attachment for depth buffer */
  attachments[1] = wlu_set_attachment_desc(app->sc[cur_sc].depth.format,
    VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
  );

  VkAttachmentReference color_ref = wlu_set_attachment_ref(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  VkAttachmentReference depth_ref = wlu_set_attachment_ref(1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
  VkSubpassDescription subpass = wlu_set_subpass_desc(0, NULL, 1, &color_ref, NULL, &depth_ref, 0, NULL);

  err = wlu_create_render_pass(app, 2, attachments, 1, &subpass, 0, NULL);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_create_render_pass failed");
    ck_abort_msg(NULL);
  }

  wlu_log_me(WLU_SUCCESS, "Successfully created the render pass!!!");
  /* End of render pass creation */

  wlu_log_me(WLU_INFO, "Start of shader creation");

  wlu_log_me(WLU_WARNING, "Compiling the frag code to spirv shader");
  wlu_shader_info shi_frag = wlu_compile_to_spirv(VK_SHADER_STAGE_FRAGMENT_BIT,
                             fragShaderText, "frag.spv", "main");
  if (!shi_frag.bytes) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_compile_to_spirv failed");
    ck_abort_msg(NULL);
  }

  wlu_log_me(WLU_WARNING, "Compiling the vert code to spirv shader");
  wlu_shader_info shi_vert = wlu_compile_to_spirv(VK_SHADER_STAGE_VERTEX_BIT,
                             vertShaderText, "vert.spv", "main");
  if (!shi_vert.bytes) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_compile_to_spirv failed");
    ck_abort_msg(NULL);
  }

  wlu_log_me(WLU_SUCCESS, "vert.spv and frag.spv officially created");
  wlu_log_me(WLU_INFO, "End of shader creation");

  VkImageView vkimg_attach[2];
  vkimg_attach[1] = app->sc[cur_sc].depth.view;
  err = wlu_create_framebuffers(app, cur_sc, 2, vkimg_attach, extent3D.width, extent3D.height, 1);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_create_framebuffers failed");
    ck_abort_msg(NULL);
  }

  err = wlu_create_pipeline_cache(app, 0, NULL);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_create_pipeline_cache failed");
    ck_abort_msg(NULL);
  }

  /* Start of vertex buffer */
  VkDeviceSize vsize = sizeof(vertices);
  err = wlu_create_buffer(
    app, vsize, vertices, 0, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    VK_SHARING_MODE_EXCLUSIVE, 0, NULL, "vertex",
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
  );
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_create_buffer failed");
    ck_abort_msg(NULL);
  }

  VkVertexInputBindingDescription vi_binding = wlu_set_vertex_input_binding_desc(0, sizeof(vertex_3D), VK_VERTEX_INPUT_RATE_VERTEX);

  VkVertexInputAttributeDescription vi_attribs[2];
  vi_attribs[0] = wlu_set_vertex_input_attrib_desc(0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(vertex_3D, pos));
  vi_attribs[1] = wlu_set_vertex_input_attrib_desc(1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(vertex_3D, color));

  VkPipelineVertexInputStateCreateInfo vertex_input_info = wlu_set_vertex_input_state_info(
    1, &vi_binding, 2, vi_attribs
  );

  /* End of vertex buffer */

  VkShaderModule vert_shader_module = wlu_create_shader_module(app, shi_vert.bytes, shi_vert.byte_size);
  if (!vert_shader_module) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] failed to create shader module");
    ck_abort_msg(NULL);
  }

  VkShaderModule frag_shader_module = wlu_create_shader_module(app, shi_frag.bytes, shi_frag.byte_size);
  if (!frag_shader_module) {
    wlu_freeup_shader(app, &vert_shader_module);
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] failed to create shader module");
    ck_abort_msg(NULL);
  }

  wlu_add_watchme_info(1, app, 0, NULL, 1, &vert_shader_module);
  wlu_add_watchme_info(1, app, 0, NULL, 2, &frag_shader_module);

  VkPipelineShaderStageCreateInfo vert_shader_stage_info = wlu_set_shader_stage_info(
    vert_shader_module, "main", VK_SHADER_STAGE_VERTEX_BIT, NULL
  );

  VkPipelineShaderStageCreateInfo frag_shader_stage_info = wlu_set_shader_stage_info(
    frag_shader_module, "main", VK_SHADER_STAGE_FRAGMENT_BIT, NULL
  );

  VkPipelineShaderStageCreateInfo shader_stages[] = {
    vert_shader_stage_info, frag_shader_stage_info
  };

  VkDynamicState dynamic_states[2] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
  };

  VkPipelineDynamicStateCreateInfo dynamic_state = wlu_set_dynamic_state_info(2, dynamic_states);

  VkPipelineInputAssemblyStateCreateInfo input_assembly = wlu_set_input_assembly_state_info(
    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE
  );

  VkPipelineRasterizationStateCreateInfo rasterizer = wlu_set_rasterization_state_info(
    VK_TRUE, VK_FALSE, VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT,
    VK_FRONT_FACE_CLOCKWISE, VK_FALSE, 0.0f, 0.0f, 0.0f, 1.0f
  );

  VkPipelineColorBlendAttachmentState color_blend_attachment = wlu_set_color_blend_attachment_state(
    VK_FALSE, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD,
    VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD,
    0xf
  );

  float blend_const[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  VkPipelineColorBlendStateCreateInfo color_blending = wlu_set_color_blend_attachment_state_info(
    VK_FALSE, VK_LOGIC_OP_NO_OP, 1, &color_blend_attachment, blend_const
  );

  VkViewport viewport = wlu_set_view_port(0.0f, 0.0f, (float) extent3D.width, (float) extent3D.height, 0.0f, 1.0f);
  VkRect2D scissor = wlu_set_rect2D(0, 0, extent3D.width, extent3D.height);
  VkPipelineViewportStateCreateInfo view_port_info = wlu_set_view_port_state_info(1, &viewport, 1, &scissor);

  VkStencilOpState back = wlu_set_stencil_op_state(
    VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP,
    VK_COMPARE_OP_ALWAYS, 0, 0, 0
  );

  VkPipelineDepthStencilStateCreateInfo ds_info = wlu_set_depth_stencil_state(
    VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL,
    VK_FALSE, VK_FALSE, back, back, 0, 0
  );

  VkPipelineMultisampleStateCreateInfo multisampling = wlu_set_multisample_state_info(
    VK_SAMPLE_COUNT_1_BIT, VK_FALSE, 0.0f, NULL, VK_FALSE, VK_FALSE
  );

  err = wlu_create_graphics_pipeline(app, 2, shader_stages,
    &vertex_input_info, &input_assembly, VK_NULL_HANDLE, &view_port_info,
    &rasterizer, &multisampling, &ds_info, &color_blending,
    &dynamic_state, 0, VK_NULL_HANDLE, UINT32_MAX
  );
  if (err) {
    wlu_freeup_shader(app, &frag_shader_module);
    wlu_freeup_shader(app, &vert_shader_module);
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] failed to create graphics pipeline");
    ck_abort_msg(NULL);
  }

  wlu_log_me(WLU_SUCCESS, "Successfully created graphics pipeline");
  wlu_freeup_shader(app, &frag_shader_module);
  wlu_freeup_shader(app, &vert_shader_module);

  VkClearValue clear_values[2];
  float float32[4] = {0.2f, 0.2f, 0.2f, 0.2f};
  int32_t int32[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  uint32_t uint32[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  clear_values[0] = wlu_set_clear_value(float32, int32, uint32, 0.0f, 0);
  clear_values[1] = wlu_set_clear_value(float32, int32, uint32, 1.0f, 1);

  /* Vertex buffer cannot be binded until we begin a renderpass */
  wlu_exec_begin_render_pass(app, cur_pool, cur_sc, 0, 0, extent3D.width, extent3D.height,
                             2, clear_values, VK_SUBPASS_CONTENTS_INLINE);
  wlu_bind_pipeline(app, cur_pool, cur_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, app->graphics_pipeline);
  wlu_bind_desc_set(app, cur_pool, cur_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, 0, 0, NULL);

  for (uint32_t i = 0; i < app->bdc; i++) {
    wlu_log_me(WLU_INFO, "app->buffs_data[%d].name: %s", i, app->buffs_data[i].name);
    wlu_log_me(WLU_INFO, "app->buffs_data[%d].buff: %p - %p", i, &app->buffs_data[i].buff, app->buffs_data[i].buff);
  }

  const VkDeviceSize offsets[1] = {0};
  wlu_bind_vertex_buffs_to_cmd_buff(app, cur_pool, cur_buff, 0, 1, &app->buffs_data[1].buff, offsets);
  wlu_cmd_set_viewport(app, &viewport, cur_pool, cur_buff, 0, 1);
  wlu_cmd_set_scissor(app, &scissor, cur_pool, cur_buff, 0, 1);

  const uint32_t vertex_count = sizeof(vertices) / sizeof(vertices[0]);
  wlu_cmd_draw(app, cur_pool, cur_buff, vertex_count, 1, 0, 0);

  wlu_exec_stop_render_pass(app, cur_pool, cur_sc);
  err = wlu_exec_stop_cmd_buffs(app, cur_pool, cur_sc);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_exec_queue_cmd_buff failed");
    ck_abort_msg(NULL);
  }

  VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  VkSemaphore wait_semaphores[1] = {app->sc[cur_sc].sems[cur_buff].image};
  VkSemaphore signal_semaphores[1] = {app->sc[cur_sc].sems[cur_buff].render};
  VkCommandBuffer cmd_buffs[1] = {app->cmd_pbs[cur_pool].cmd_buffs[cur_buff]};
  err = wlu_queue_graphics_queue(app, 1, cmd_buffs, 1, wait_semaphores, &pipe_stage_flags, 1, signal_semaphores);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_exec_queue_cmd_buff failed");
    ck_abort_msg(NULL);
  }

  err = wlu_queue_present_queue(app, 0, NULL, 1, &app->sc[cur_sc].swap_chain, &cur_buff, NULL);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_exec_queue_cmd_buff failed");
    ck_abort_msg(NULL);
  }

  wait_seconds(1);
  freeme(app, wc);
} END_TEST;


Suite *main_suite(void) {
  Suite *s = NULL;
  TCase *tc_core = NULL;

  s = suite_create("TestCube");

  /* Core test case */
  tc_core = tcase_create("Core");

  tcase_add_test(tc_core, test_vulkan_client_create_3D);
  suite_add_tcase(s, tc_core);

  return s;
}

int main (void) {
  int number_failed;
  SRunner *sr = NULL;

  sr = srunner_create(main_suite());

  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  sr = NULL;
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
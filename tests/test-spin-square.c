/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Vincent Davis Jr.
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
#include <time.h>

#include "test-extras.h"
#include "test-shade.h"

#define WIDTH 800
#define HEIGHT 600

#define drand48() ((float)(rand() / (RAND_MAX + 1.0)))

struct uniform_block_data {
  mat4 model;
  mat4 view;
  mat4 proj;
} ubd;

void freeme(vkcomp *app, wclient *wc) {
  wlu_freeup_vk(app);
  wlu_freeup_wc(wc);
  wlu_freeup_watchme();
}

VkResult init_buffs(vkcomp *app) {
  VkResult err;

  err = wlu_otba(app, 8, WLU_BUFFS_DATA);
  if (err) return err;

  err = wlu_otba(app, 1, WLU_SC_DATA);
  if (err) return err;

  err = wlu_otba(app, 1, WLU_GP_DATA);
  if (err) return err;

  err = wlu_otba(app, 1, WLU_CMD_DATA);
  if (err) return err;

  err = wlu_otba(app, 1, WLU_DESC_DATA);
  if (err) return err;

  return err;
}

START_TEST(test_vulkan_client_create) {
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

  err = init_buffs(app);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] init_buffs failed!!");
    ck_abort_msg(NULL);
  }

  /* Signal handler for this process */
  err = wlu_watch_me(SIGSEGV, getpid());
  if (err) {
    freeme(app, wc);
    ck_abort_msg(NULL);
  }

  wlu_add_watchme_info(1, app, 1, wc, 0, NULL);

  err = wlu_create_instance(app, "Hello Triangle", "No Engine", 1, enabled_validation_layers, 4, instance_extensions);
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

  err = wlu_create_logical_device(app, &device_feats, 1, 1, enabled_validation_layers, 1, device_extensions);
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

  VkExtent2D extent2D = wlu_choose_2D_swap_extent(capabilities, WIDTH, HEIGHT);
  if (extent2D.width == UINT32_MAX) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] choose_swap_extent failed, extent2D.width equals %d", extent2D.width);
    ck_abort_msg(NULL);
  }

  uint32_t cur_buff = 0, cur_scd = 0, cur_pool = 0, cur_gpd = 0, cur_bd = 0, cur_cmd = 0, cur_dd = 0;
  err = wlu_create_swap_chain(app, cur_scd, capabilities, surface_fmt, pres_mode, extent2D.width, extent2D.height);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] failed to create swap chain");
    ck_abort_msg(NULL);
  }

  err = wlu_create_cmd_pool(app, cur_scd, cur_cmd, app->indices.graphics_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] failed to create command pool, ERROR CODE: %d", err);
    ck_abort_msg(NULL);
  }

  err = wlu_create_cmd_buffs(app, cur_pool, cur_scd, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] failed to create command buffers, ERROR CODE: %d", err);
    ck_abort_msg(NULL);
  }

  err = wlu_create_img_views(app, cur_scd, surface_fmt.format, VK_IMAGE_VIEW_TYPE_2D);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] failed to create image views");
    ck_abort_msg(NULL);
  }

  /* This is where creation of the graphics pipeline begins */
  err = wlu_create_semaphores(app, cur_scd);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_create_semaphores failed");
    ck_abort_msg(NULL);
  }

  /* Acquire the swapchain image in order to set its layout */
  err = wlu_retrieve_swapchain_img(app, &cur_buff, cur_scd);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_retrieve_swapchain_img failed");
    ck_abort_msg(NULL);
  }

  /* Starting point for render pass creation */
  VkAttachmentDescription attachment = wlu_set_attachment_desc(surface_fmt.format,
    VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
  );

  VkAttachmentReference color_ref = wlu_set_attachment_ref(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

  VkSubpassDescription subpass = wlu_set_subpass_desc(
    0, NULL, 1, &color_ref, NULL, NULL, 0, NULL
  );

  VkSubpassDependency subdep = wlu_set_subpass_dep(
    VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0
  );

  err = wlu_create_render_pass(app, cur_gpd, 1, &attachment, 1, &subpass, 1, &subdep);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_create_render_pass failed");
    ck_abort_msg(NULL);
  }
  wlu_log_me(WLU_SUCCESS, "Successfully created render pass");
  /* ending point for render pass creation */

  wlu_log_me(WLU_WARNING, "Compiling the frag code to spirv shader");

  wlu_shader_info shi_frag = wlu_compile_to_spirv(VK_SHADER_STAGE_FRAGMENT_BIT,
                             shader_frag_src, "frag.spv", "main");
  if (!shi_frag.bytes) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_compile_to_spirv failed");
    ck_abort_msg(NULL);
  }

  wlu_log_me(WLU_WARNING, "Compiling the vert code to spirv shader");
  wlu_shader_info shi_vert = wlu_compile_to_spirv(VK_SHADER_STAGE_VERTEX_BIT,
                             shader_vert_src, "vert.spv", "main");
  if (!shi_vert.bytes) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_compile_to_spirv failed");
    ck_abort_msg(NULL);
  }

  VkImageView vkimg_attach[1];
  err = wlu_create_framebuffers(app, cur_scd, cur_gpd, 1, vkimg_attach, extent2D.width, extent2D.height, 1);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_create_framebuffers failed");
    ck_abort_msg(NULL);
  }

  /* Start of vertex buffer */
  vertex_2D vertices[4] = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
  };
  VkDeviceSize vsize = sizeof(vertices);
  const uint32_t vertex_count = vsize / sizeof(vertex_2D);
  for (uint32_t i = 0; i < vertex_count; i++) {
    wlu_print_vector(&vertices[i].pos, WLU_VEC2);
    wlu_print_vector(&vertices[i].color, WLU_VEC3);
  }

  /*
   * Can Find in vulkan SDK doc/tutorial/html/07-init_uniform_buffer.html
   * The VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT communicates that the memory
   * should be mapped so that the CPU (host) can access it.
   * The VK_MEMORY_PROPERTY_HOST_COHERENT_BIT requests that the
   * writes to the memory by the host are visible to the device
   * (and vice-versa) without the need to flush memory caches.
   */
  err = wlu_create_buffer(
    app, cur_bd, vsize, vertices, 0, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    VK_SHARING_MODE_EXCLUSIVE, 0, NULL, "vertex",
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
  );
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_create_uniform_buff failed");
    ck_abort_msg(NULL);
  }
  cur_bd++;

  VkDeviceSize isize = sizeof(indices);
  const uint32_t index_count = isize / sizeof(uint16_t);
  err = wlu_create_buffer(
    app, cur_bd, isize, indices, 0, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_SHARING_MODE_EXCLUSIVE, 0, NULL, "staging_two",
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
  );
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_create_uniform_buff failed");
    ck_abort_msg(NULL);
  }
  cur_bd++;

  err = wlu_create_buffer(
    app, cur_bd, isize, NULL, 0,
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    VK_SHARING_MODE_EXCLUSIVE, 0, NULL, "index",
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
  );
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_create_uniform_buff failed");
    ck_abort_msg(NULL);
  }
  cur_bd++;

  err = wlu_copy_buffer(app, cur_pool, app->buffs_data[1].buff, app->buffs_data[2].buff, isize);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_copy_buffer failed");
    ck_abort_msg(NULL);
  }

  float fovy = wlu_set_fovy(45.0f);
  float hw = (float) extent2D.width / (float) extent2D.height;
  if (extent2D.width > extent2D.height) fovy *= hw;
  wlu_set_lookat(ubd.view, spin_eye, spin_center, spin_up);
  wlu_set_perspective(ubd.proj, fovy, hw, 0.1f, 10.0f);
  ubd.proj[1][1] *= -1;

  VkDeviceSize usize[app->sc_data[cur_scd].sic];
  for (uint32_t i = cur_bd; i < (cur_bd+app->sc_data[cur_scd].sic); i++) {
    srand((unsigned int)time(NULL));
    wlu_set_matrix(ubd.model, model_matrix_default, WLU_MAT4);
    wlu_set_rotate(ubd.model, ubd.model, drand48() * wlu_set_fovy(90.0f), WLU_Z);
    usize[i-cur_bd] = sizeof(ubd);
    err = wlu_create_buffer(
      app, i, usize[i-cur_bd], &ubd, 0, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_SHARING_MODE_EXCLUSIVE, 0, NULL, "uniform",
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    if (err) {
      freeme(app, wc);
      wlu_log_me(WLU_DANGER, "[x] wlu_create_uniform_buff failed");
      ck_abort_msg(NULL);
    }
    wlu_log_me(WLU_SUCCESS, "Just Allocated!!!");
    wlu_log_me(WLU_INFO, "app->buffs_data[%d].name: %s", i, app->buffs_data[i].name);
    wlu_log_me(WLU_INFO, "app->buffs_data[%d].buff: %p - %p", i, &app->buffs_data[i].buff, app->buffs_data[i].buff);
  }

  cur_bd += app->sc_data[cur_scd].sic;

  /* 0 is the binding # this is bytes between successive structs */
  VkVertexInputBindingDescription vi_binding = wlu_set_vertex_input_binding_desc(0, sizeof(vertex_2D), VK_VERTEX_INPUT_RATE_VERTEX);

  VkVertexInputAttributeDescription vi_attribs[2];
  vi_attribs[0] = wlu_set_vertex_input_attrib_desc(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(vertex_2D, pos));
  vi_attribs[1] = wlu_set_vertex_input_attrib_desc(1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vertex_2D, color));

  VkPipelineVertexInputStateCreateInfo vertex_input_info = wlu_set_vertex_input_state_info(1, &vi_binding, 2, vi_attribs);

  /* End of vertex buffer */
  app->desc_data[cur_dd].dc = app->sc_data[cur_scd].sic;
  VkDescriptorSetLayoutBinding desc_set = wlu_set_desc_set(
    0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, app->sc_data[cur_scd].sic,
    VK_SHADER_STAGE_VERTEX_BIT, NULL
  );

  VkDescriptorSetLayoutCreateInfo desc_set_info = wlu_set_desc_set_info(0, 1, &desc_set);

  /* Using same layout for all obects for now */
  err = wlu_create_desc_set_layouts(app, cur_dd, &desc_set_info);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_set_desc_set_info failed");
    ck_abort_msg(NULL);
  }

  /* Using same layout for all obects for now */
  err = wlu_create_desc_set_layouts(app, cur_dd, &desc_set_info);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_create_desc_set_layouts failed, ERROR CODE: %d", err);
    ck_abort_msg(NULL);
  }

  VkDescriptorPoolSize pool_sizes[app->desc_data[cur_dd].dc];
  for (uint32_t i = 0; i < app->desc_data[cur_dd].dc; i++)
    pool_sizes[i] = wlu_set_desc_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, app->desc_data[cur_dd].dc);

  err = wlu_create_desc_pool(app, cur_dd, 0, app->desc_data[cur_dd].dc, pool_sizes);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_create_desc_pool failed");
    ck_abort_msg(NULL);
  }

  err = wlu_create_desc_set(app, cur_dd, app->desc_data[cur_dd].dc);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_create_desc_set failed");
    ck_abort_msg(NULL);
  }

  /* set uniform buffer VKBufferInfos */
  VkDescriptorBufferInfo buff_infos[app->desc_data[cur_dd].dc];
  for (uint32_t i = 0; i < app->desc_data[cur_dd].dc; i++)
    buff_infos[i] = wlu_set_desc_buff_info(app->buffs_data[i+3].buff, 0, usize[i]);

  VkWriteDescriptorSet writes[app->desc_data[cur_dd].dc];
  for (uint32_t i = 0; i < app->desc_data[cur_dd].dc; i++)
    writes[i] = wlu_write_desc_set(app->desc_data[cur_dd].desc_set[i], 0, 0, app->desc_data[cur_dd].dc,
                                   VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, NULL, buff_infos, NULL);

  wlu_update_desc_sets(app, app->desc_data[cur_dd].dc, writes, 0, NULL);

  err = wlu_create_pipeline_layout(app, cur_gpd, app->desc_data[cur_dd].dc, app->desc_data[cur_dd].desc_layouts, 0, NULL);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_create_pipeline_layout failed");
    ck_abort_msg(NULL);
  }

  VkShaderModule frag_shader_module = wlu_create_shader_module(app, shi_frag.bytes, shi_frag.byte_size);
  if (!frag_shader_module) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] failed to create shader module");
    ck_abort_msg(NULL);
  }

  VkShaderModule vert_shader_module = wlu_create_shader_module(app, shi_vert.bytes, shi_vert.byte_size);
  if (!vert_shader_module) {
    wlu_freeup_shader(app, &vert_shader_module);
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] failed to create shader module");
    ck_abort_msg(NULL);
  }

  wlu_add_watchme_info(1, app, 0, NULL, 1, &frag_shader_module);
  wlu_add_watchme_info(1, app, 0, NULL, 2, &vert_shader_module);

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
    VK_DYNAMIC_STATE_LINE_WIDTH
  };

  VkPipelineDynamicStateCreateInfo dynamic_state = wlu_set_dynamic_state_info(2, dynamic_states);

  VkPipelineInputAssemblyStateCreateInfo input_assembly = wlu_set_input_assembly_state_info(
    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE
  );

  VkViewport viewport = wlu_set_view_port(0.0f, 0.0f, (float) extent2D.width, (float) extent2D.height, 0.0f, 1.0f);
  VkRect2D scissor = wlu_set_rect2D(0, 0, extent2D.width, extent2D.height);
  VkPipelineViewportStateCreateInfo view_port_info = wlu_set_view_port_state_info(1, &viewport, 1, &scissor);

  VkPipelineRasterizationStateCreateInfo rasterizer = wlu_set_rasterization_state_info(
    VK_FALSE, VK_FALSE, VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT,
    VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, 0.0f, 0.0f, 0.0f, 1.0f
  );

  VkPipelineMultisampleStateCreateInfo multisampling = wlu_set_multisample_state_info(
    VK_SAMPLE_COUNT_1_BIT, VK_FALSE, 1.0f, NULL, VK_FALSE, VK_FALSE
  );

  VkPipelineColorBlendAttachmentState color_blend_attachment = wlu_set_color_blend_attachment_state(
    VK_FALSE, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
    VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD,
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
  );

  float blend_const[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  VkPipelineColorBlendStateCreateInfo color_blending = wlu_set_color_blend_attachment_state_info(
    VK_TRUE, VK_LOGIC_OP_COPY, 1, &color_blend_attachment, blend_const
  );

  err = wlu_create_pipeline_cache(app, 0, NULL);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_create_pipeline_cache failed");
    ck_abort_msg(NULL);
  }

  err = wlu_create_graphics_pipelines(app, 2, shader_stages,
    &vertex_input_info, &input_assembly, VK_NULL_HANDLE, &view_port_info,
    &rasterizer, &multisampling, VK_NULL_HANDLE, &color_blending,
    &dynamic_state, 0, VK_NULL_HANDLE, UINT32_MAX, cur_gpd, 1
  );
  if (err) {
    wlu_freeup_shader(app, &frag_shader_module);
    wlu_freeup_shader(app, &vert_shader_module);
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] failed to create graphics pipeline");
    ck_abort_msg(NULL);
  }

  wlu_log_me(WLU_SUCCESS, "graphics pipeline creation successfull");
  wlu_freeup_shader(app, &frag_shader_module);
  wlu_freeup_shader(app, &vert_shader_module);

  /* Ending setup for graphics pipeline */

  float float32[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  int32_t int32[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  uint32_t uint32[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  VkClearValue clear_value = wlu_set_clear_value(float32, int32, uint32, 0.0f, 0);

  /* Set command buffers into recording state */
  err = wlu_exec_begin_cmd_buffs(app, cur_pool, cur_scd, 0, NULL);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] failed to start command buffer recording");
    ck_abort_msg(NULL);
  }

  for (uint32_t i = 0; i < app->bdc; i++) {
    wlu_log_me(WLU_INFO, "app->buffs_data[%d].name: %s", i, app->buffs_data[i].name);
    wlu_log_me(WLU_INFO, "app->buffs_data[%d].buff: %p - %p", i, &app->buffs_data[i].buff, app->buffs_data[i].buff);
  }

  /* Drawing will start when you begin a render pass */
  wlu_exec_begin_render_pass(app, cur_pool, cur_scd, cur_gpd, 0, 0, extent2D.width,
                             extent2D.height, 2, &clear_value, VK_SUBPASS_CONTENTS_INLINE);
  wlu_cmd_set_viewport(app, &viewport, cur_pool, cur_buff, 0, 1);
  wlu_bind_pipeline(app, cur_pool, cur_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, app->gp_data[cur_gpd].graphics_pipelines[0]);
  wlu_bind_desc_sets(app, cur_pool, cur_buff, cur_dd, cur_gpd, VK_PIPELINE_BIND_POINT_GRAPHICS, 0, 0, NULL);

  const VkDeviceSize offsets[1] = {0};
  wlu_bind_vertex_buffs_to_cmd_buff(app, cur_pool, cur_buff, 0, 1, &app->buffs_data[0].buff, offsets);
  wlu_bind_index_buff_to_cmd_buff(app, cur_pool, cur_buff, app->buffs_data[2].buff, offsets[0], VK_INDEX_TYPE_UINT16);

  wlu_cmd_draw_indexed(app, cur_pool, cur_buff, index_count, 1, 0, offsets[0], 0);

  wlu_exec_stop_render_pass(app, cur_pool, cur_scd);
  err = wlu_exec_stop_cmd_buffs(app, cur_pool, cur_scd);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_exec_stop_cmd_buffs failed");
    ck_abort_msg(NULL);
  }

  VkPipelineStageFlags wait_stages[1] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  VkSemaphore wait_semaphores[1] = {app->sc_data[cur_scd].sems[cur_buff].image};
  VkSemaphore signal_semaphores[1] = {app->sc_data[cur_scd].sems[cur_buff].render};
  VkCommandBuffer cmd_buffs[1] = {app->cmd_data[cur_pool].cmd_buffs[cur_buff]};
  err = wlu_queue_graphics_queue(app, 1, cmd_buffs, 1, wait_semaphores, wait_stages, 1, signal_semaphores);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_queue_graphics_queue failed");
    ck_abort_msg(NULL);
  }

  err = wlu_queue_present_queue(app, 1, signal_semaphores, 1, &app->sc_data[cur_scd].swap_chain, &cur_buff, NULL);
  if (err) {
    freeme(app, wc);
    wlu_log_me(WLU_DANGER, "[x] wlu_queue_present_queue failed");
    ck_abort_msg(NULL);
  }

  wait_seconds(3);
  freeme(app, wc);
} END_TEST;

Suite *main_suite(void) {
  Suite *s = NULL;
  TCase *tc_core = NULL;

  s = suite_create("TestSquare");

  /* Core test case */
  tc_core = tcase_create("Core");

  tcase_add_test(tc_core, test_vulkan_client_create);
  suite_add_tcase(s, tc_core);

  return s;
}

int main (void) {
  int number_failed;
  SRunner *sr = NULL;

  sr = srunner_create(main_suite());

  // wait_seconds(6);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  sr = NULL;
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
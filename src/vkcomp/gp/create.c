/**
* The MIT License (MIT)
*
* Copyright (c) 2019-2020 Vincent Davis Jr.
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

#define LUCUR_VKCOMP_API
#include <lucom.h>

VkShaderModule dlu_create_shader_module(vkcomp *app, uint32_t cur_ld, char *code, size_t code_size) {

  VkResult err = VK_RESULT_MAX_ENUM;
  VkShaderModule shader_module = VK_NULL_HANDLE;

  if (!app->ld_data[cur_ld].device) { PERR(DLU_VKCOMP_DEVICE, 0, NULL); return shader_module; }

  VkShaderModuleCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.codeSize = code_size;
  create_info.pCode = (const uint32_t *) code;

  err = vkCreateShaderModule(app->ld_data[cur_ld].device, &create_info, NULL, &shader_module);
  if (err) PERR(DLU_VK_FUNC_ERR, err, "vkCreateShaderModule");

  if (err == VK_SUCCESS) dlu_log_me(DLU_SUCCESS, "Shader module successfully created");

  return shader_module;
}

VkResult dlu_create_render_pass(
  vkcomp *app,
  uint32_t cur_gpd,
  uint32_t attachmentCount,
  const VkAttachmentDescription *pAttachments,
  uint32_t subpassCount,
  const VkSubpassDescription *pSubpasses,
  uint32_t dependencyCount,
  const VkSubpassDependency *pDependencies
) {

  VkResult res = VK_RESULT_MAX_ENUM;

  if (app->gp_data[cur_gpd].ldi == UINT32_MAX) { PERR(DLU_VKCOMP_DEVICE_NOT_ASSOC, 0, "dlu_create_pipeline_layout(3)"); return res; }
  if (!app->ld_data[app->gp_data[cur_gpd].ldi].device) { PERR(DLU_VKCOMP_DEVICE, 0, NULL); return res; }
  if (!app->gp_data) { PERR(DLU_BUFF_NOT_ALLOC, 0, "DLU_GP_DATA"); return res; }

  VkRenderPassCreateInfo render_pass_info = {};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_info.pNext = NULL;
  render_pass_info.flags = 0;
  render_pass_info.attachmentCount = attachmentCount;
  render_pass_info.pAttachments = pAttachments;
  render_pass_info.subpassCount = subpassCount;
  render_pass_info.pSubpasses = pSubpasses;
  render_pass_info.dependencyCount = dependencyCount;
  render_pass_info.pDependencies = pDependencies;

  res = vkCreateRenderPass(app->ld_data[app->gp_data[cur_gpd].ldi].device, &render_pass_info, NULL, &app->gp_data[cur_gpd].render_pass);
  if (res) PERR(DLU_VK_FUNC_ERR, res, "vkCreateRenderPass");

  return res;
}

VkResult dlu_create_graphics_pipelines(
  vkcomp *app,
  uint32_t cur_gpd,
  uint32_t stageCount,
  const VkPipelineShaderStageCreateInfo *pStages,
  const VkPipelineVertexInputStateCreateInfo *pVertexInputState,
  const VkPipelineInputAssemblyStateCreateInfo *pInputAssemblyState,
  const VkPipelineTessellationStateCreateInfo *pTessellationState,
  const VkPipelineViewportStateCreateInfo *pViewportState,
  const VkPipelineRasterizationStateCreateInfo *pRasterizationState,
  const VkPipelineMultisampleStateCreateInfo *pMultisampleState,
  const VkPipelineDepthStencilStateCreateInfo *pDepthStencilState,
  const VkPipelineColorBlendStateCreateInfo *pColorBlendState,
  const VkPipelineDynamicStateCreateInfo *pDynamicState,
  uint32_t subpass,
  VkPipeline basePipelineHandle,
  uint32_t basePipelineIndex
) {

  VkResult res = VK_RESULT_MAX_ENUM;

  if (!app->gp_data[cur_gpd].render_pass) { PERR(DLU_VKCOMP_RENDER_PASS, 0, NULL); return res; }
  if (!app->gp_data[cur_gpd].pipeline_layout) { PERR(DLU_VKCOMP_PIPELINE_LAYOUT, 0, NULL); return res; }
  if (!app->gp_data[cur_gpd].graphics_pipelines) { PERR(DLU_BUFF_NOT_ALLOC, 0, "DLU_GP_DATA_MEMS"); return res; }

  VkGraphicsPipelineCreateInfo pipeline_info = {};
  pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipeline_info.pNext = NULL;
  pipeline_info.stageCount = stageCount;
  pipeline_info.pStages = pStages;
  pipeline_info.pVertexInputState = pVertexInputState;
  pipeline_info.pInputAssemblyState = pInputAssemblyState;
  pipeline_info.pTessellationState = pTessellationState;
  pipeline_info.pViewportState = pViewportState;
  pipeline_info.pRasterizationState = pRasterizationState;
  pipeline_info.pMultisampleState = pMultisampleState;
  pipeline_info.pDepthStencilState = pDepthStencilState;
  pipeline_info.pColorBlendState = pColorBlendState;
  pipeline_info.pDynamicState = pDynamicState;
  pipeline_info.layout = app->gp_data[cur_gpd].pipeline_layout;
  pipeline_info.renderPass = app->gp_data[cur_gpd].render_pass;
  pipeline_info.subpass = subpass;
  pipeline_info.basePipelineHandle = basePipelineHandle;
  pipeline_info.basePipelineIndex = basePipelineIndex;

  res = vkCreateGraphicsPipelines(app->ld_data[app->gp_data[cur_gpd].ldi].device, app->gp_cache.pipe_cache, 1, &pipeline_info, NULL, app->gp_data[cur_gpd].graphics_pipelines);
  if (res) { PERR(DLU_VK_FUNC_ERR, res, "vkCreateGraphicsPipelines"); }

  return res;
}

VkResult dlu_create_pipeline_cache(vkcomp *app, uint32_t cur_ld, size_t initialDataSize, const void *pInitialData) {

  VkResult res = VK_RESULT_MAX_ENUM;

  VkPipelineCacheCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.initialDataSize = initialDataSize;
  create_info.pInitialData = pInitialData;

  res = vkCreatePipelineCache(app->ld_data[cur_ld].device, &create_info, NULL, &app->gp_cache.pipe_cache);
  if (res) PERR(DLU_VK_FUNC_ERR, res, "vkCreatePipelineCache");

  /* Associate a VkPipelineCache with a VkDevice */
  app->gp_cache.ldi = cur_ld;

  return res;
}

VkResult dlu_create_pipeline_layout(
  vkcomp *app,
  uint32_t cur_ld,
  uint32_t cur_gpd,
  uint32_t layout_count,
  VkDescriptorSetLayoutCreateInfo *layout_infos,
  uint32_t pushConstantRangeCount,
  const VkPushConstantRange *pPushConstantRanges,
  VkPipelineLayoutCreateFlags flags
) {

  VkResult res = VK_RESULT_MAX_ENUM;

  if (!app->gp_data) { PERR(DLU_BUFF_NOT_ALLOC, 0, "DLU_GP_DATA"); return res; }

  VkDescriptorSetLayout  *pSetLayouts  = (layout_infos) ? alloca(layout_count * sizeof(VkDescriptorSetLayout)) :  NULL;
  for (uint32_t i = 0; i < layout_count; i++) {
    res = vkCreateDescriptorSetLayout(app->ld_data[cur_ld].device, &layout_infos[i], NULL, &pSetLayouts[i]);
    if (res) { PERR(DLU_VK_FUNC_ERR, res, "vkCreateDescriptorSetLayout"); goto end_func; }
  }

  VkPipelineLayoutCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = flags;
  create_info.setLayoutCount = layout_count;
  create_info.pSetLayouts = pSetLayouts;
  create_info.pushConstantRangeCount = pushConstantRangeCount;
  create_info.pPushConstantRanges = pPushConstantRanges;

  res = vkCreatePipelineLayout(app->ld_data[cur_ld].device, &create_info, NULL, &app->gp_data[cur_gpd].pipeline_layout);
  if (res) PERR(DLU_VK_FUNC_ERR, res, "vkCreatePipelineLayout")

  /* Associate a logical device with a graphics pipeline */
  app->gp_data[cur_gpd].ldi = cur_ld;

end_func:
  for (uint32_t i = 0; i < layout_count; i++)
    if (pSetLayouts[i])
      vkDestroyDescriptorSetLayout(app->ld_data[cur_ld].device, pSetLayouts[i], NULL);

  return res;
}

VkResult dlu_create_desc_set_layout(
  vkcomp *app,
  uint32_t cur_dd,
  uint32_t cur_dl,
  VkDescriptorSetLayoutCreateInfo *desc_set_info
) {
  VkResult res = VK_RESULT_MAX_ENUM;

  if (!app->desc_data[cur_dd].layouts) { PERR(DLU_BUFF_NOT_ALLOC, 0, "DLU_DESC_DATA_MEMS"); return res; }
  if (app->desc_data[cur_dd].ldi == UINT32_MAX) { PERR(DLU_VKCOMP_DEVICE_NOT_ASSOC, 0, "dlu_create_desc_pool(3)"); return res; }

  res = vkCreateDescriptorSetLayout(app->ld_data[app->desc_data[cur_dd].ldi].device, desc_set_info, NULL, &app->desc_data[cur_dd].layouts[cur_dl]);
  if (res) PERR(DLU_VK_FUNC_ERR, res, "vkCreateDescriptorSetLayout")

  return res;
}

VkResult dlu_create_desc_pool(
  vkcomp *app,
  uint32_t cur_ld,
  uint32_t cur_dd,
  VkDescriptorPoolCreateFlags flags,
  uint32_t psize,
  VkDescriptorPoolSize *pool_sizes
) {

  VkResult res = VK_RESULT_MAX_ENUM;

  VkDescriptorPoolCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = flags;
  create_info.maxSets = app->desc_data[cur_dd].dlsc;
  create_info.poolSizeCount = psize;
  create_info.pPoolSizes = pool_sizes;

  res = vkCreateDescriptorPool(app->ld_data[cur_ld].device, &create_info, NULL, &app->desc_data[cur_dd].desc_pool);
  if (res) PERR(DLU_VK_FUNC_ERR, res, "vkCreateDescriptorPool");

  app->desc_data[cur_dd].ldi = cur_ld;

  return res;
}

VkResult dlu_create_desc_sets(
  vkcomp *app,
  uint32_t cur_dd
) {

  VkResult res = VK_RESULT_MAX_ENUM;

  if (!app->desc_data[cur_dd].desc_pool) { PERR(DLU_VKCOMP_DESC_POOL, 0, NULL); return res; }
  if (!app->desc_data[cur_dd].layouts) { PERR(DLU_BUFF_NOT_ALLOC, 0, "DLU_DESC_DATA_MEMS"); return res; }
  if (!app->desc_data[cur_dd].desc_set) { PERR(DLU_BUFF_NOT_ALLOC, 0, "DLU_DESC_DATA_MEMS"); return res; }
  if (app->desc_data[cur_dd].ldi == UINT32_MAX) { PERR(DLU_VKCOMP_DEVICE_NOT_ASSOC, 0, "dlu_create_desc_pool(3)"); return res; }
  /* Be Sure all VkDescriptorSetLayouts are present */
  for (uint32_t i = 0; i < app->desc_data[cur_dd].dlsc; i++) {
    if (!app->desc_data[cur_dd].layouts[i]) {
      PERR(DLU_VKCOMP_DESC_LAYOUT, i , NULL);
      return res;
    }
  }

  VkDescriptorSetAllocateInfo alloc_info;
  alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  alloc_info.pNext = NULL;
  alloc_info.descriptorPool = app->desc_data[cur_dd].desc_pool;
  alloc_info.descriptorSetCount = app->desc_data[cur_dd].dlsc;
  alloc_info.pSetLayouts = app->desc_data[cur_dd].layouts;

  res = vkAllocateDescriptorSets(app->ld_data[app->desc_data[cur_dd].ldi].device, &alloc_info, app->desc_data[cur_dd].desc_set);
  if (res) PERR(DLU_VK_FUNC_ERR, res, "vkAllocateDescriptorSets")

  return res;
}

VkResult dlu_create_texture_image(
  vkcomp *app,
  uint32_t cur_ld,
  uint32_t cur_tex,
  VkImageCreateInfo *img_info,
  VkFlags requirements_mask
) {

  VkResult res = VK_RESULT_MAX_ENUM;

  if (!app->text_data) { PERR(DLU_BUFF_NOT_ALLOC, 0, "DLU_TEXT_DATA"); return res; }

  res = vkCreateImage(app->ld_data[cur_ld].device, img_info, NULL, &app->text_data[cur_tex].image);
  if (res) { PERR(DLU_VK_FUNC_ERR, res, "vkCreateImage"); }

  /**
  * Although you know the width, height, and the size of a image element,
  * there is no way to determine exactly how much memory is needed to allocate.
  * This is because alignment constraints that may be placed by the GPU hardware.
  * This function allows you to find out everything you need to allocate the
  * memory for an image.
  */
  VkMemoryRequirements mem_reqs;
  vkGetImageMemoryRequirements(app->ld_data[cur_ld].device, app->text_data[cur_tex].image, &mem_reqs);

  VkMemoryAllocateInfo alloc_info = {};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.pNext = NULL;
  alloc_info.allocationSize = mem_reqs.size;
  alloc_info.memoryTypeIndex = 0;

  /* find a suitable memory type for image */
  res = memory_type_from_properties(app, app->ld_data[cur_ld].pdi, mem_reqs.memoryTypeBits, requirements_mask, &alloc_info.memoryTypeIndex);
  if (!res) { PERR(DLU_MEM_TYPE_ERR, 0, NULL); return res; }

  res = vkAllocateMemory(app->ld_data[cur_ld].device, &alloc_info, NULL, &app->text_data[cur_tex].mem);
  if (res) { PERR(DLU_VK_FUNC_ERR, res, "vkAllocateMemory"); return res; }

  /**
  * Associate the memory allocated with the VkBuffer resource.
  * It is easier to attach the entire VkDeviceMemory to the VkBuffer resource
  * So offset will always be zero whenever a call to vkBind*Memory is called
  */
  res = vkBindImageMemory(app->ld_data[cur_ld].device, app->text_data[cur_tex].image, app->text_data[cur_tex].mem, 0);
  if (res) PERR(DLU_VK_FUNC_ERR, res, "vkBindImageMemory")

  /* Associate a texture with a given VkDevice */
  app->text_data[cur_tex].ldi = cur_ld;

  return res;
}

VkResult dlu_create_texture_sampler(vkcomp *app, uint32_t cur_tex, VkSamplerCreateInfo *sample_info) {
  VkResult res = VK_RESULT_MAX_ENUM;

  if (!app->text_data) { PERR(DLU_BUFF_NOT_ALLOC, 0, "DLU_TEXT_DATA"); return res; }

  res = vkCreateSampler(app->ld_data[app->text_data[cur_tex].ldi].device, sample_info, NULL, &app->text_data[cur_tex].sampler);
  if (res) PERR(DLU_VK_FUNC_ERR, res, "vkCreateSampler")

  return res;
}

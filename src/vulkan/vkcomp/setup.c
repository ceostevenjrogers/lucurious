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
#include <wlu/utils/log.h>
#include <vlucur/values.h>

vkcomp *wlu_init_vk() {
  vkcomp *app = (vkcomp *) calloc(sizeof(vkcomp), sizeof(vkcomp));
  if (!app) {
    wlu_log_me(WLU_DANGER, "calloc vkcomp *app failed");
    return NULL;
  }
  set_vkcomp_init_values(app);
  return app;
}

void wlu_freeup_sc(void *data) {
  vkcomp *app = (vkcomp *) data;

  if (app->cmd_pbs) {
    for (uint32_t i = 0; i < app->cpc; i++)
      if (app->cmd_pbs[i].cmd_buffs)
        vkFreeCommandBuffers(app->device, app->cmd_pbs[i].cmd_pool, app->sc[i].sic, app->cmd_pbs[i].cmd_buffs);
  }
  if (app->pipeline_cache)  /* leave like this for now */
    vkDestroyPipelineCache(app->device, app->pipeline_cache, NULL);
  if (app->gp_data) { /* leave like this for now */
    for (uint32_t i = 0; i < app->gpc; i++) {
      if (app->gp_data[i].pipeline_layout)
        vkDestroyPipelineLayout(app->device, app->gp_data[i].pipeline_layout, NULL);
      if (app->gp_data[i].render_pass)
        vkDestroyRenderPass(app->device, app->gp_data[i].render_pass, NULL);
      uint32_t gps_size = sizeof(app->gp_data[i].graphics_pipelines) / sizeof(VkPipeline*);
      for (uint32_t j = 0; j < gps_size; j++) {
        if (app->gp_data[i].graphics_pipelines[j])
          vkDestroyPipeline(app->device, app->gp_data[i].graphics_pipelines[j], NULL);
      }
    }
  }
  for (uint32_t i = 0; i < app->scc; i++) {
    if (app->sc[i].sc_buffs && app->sc[i].frame_buffs) {
      for (uint32_t j = 0; j < app->sc[i].sic; j++) {
        vkDestroyFramebuffer(app->device, app->sc[i].frame_buffs[j], NULL);
        vkDestroyImageView(app->device, app->sc[i].sc_buffs[j].view, NULL);
        app->sc[i].frame_buffs[j] = VK_NULL_HANDLE;
        app->sc[i].sc_buffs[j].view = VK_NULL_HANDLE;
      }
      FREE(app->sc[i].sc_buffs);
    }
    if (app->sc[i].swap_chain)
      vkDestroySwapchainKHR(app->device, app->sc[i].swap_chain, NULL);
  }
}

void wlu_freeup_vk(void *data) {
  vkcomp *app = (vkcomp *) data;

  if (app->debug_report_callbacks) {
    for (uint32_t i = 0; i < app->dbg_size; i++) {
      app->dbg_destroy_report_callback(app->instance, app->debug_report_callbacks[i], NULL);
      app->debug_report_callbacks[i] = NULL;
    }
    free(app->debug_report_callbacks);
  }
  if (app->vl_props)
    free(app->vl_props);
  if (app->cmd_pbs) {
    for (uint32_t i = 0; i < app->cpc; i++) {
      if (app->cmd_pbs[i].cmd_buffs) {
        vkFreeCommandBuffers(app->device, app->cmd_pbs[i].cmd_pool, app->sc[i].sic, app->cmd_pbs[i].cmd_buffs);
        FREE(app->cmd_pbs[i].cmd_buffs);
      }
      if (app->cmd_pbs[i].cmd_pool) {
        vkDestroyCommandPool(app->device, app->cmd_pbs[i].cmd_pool, NULL);
        app->cmd_pbs[i].cmd_pool = VK_NULL_HANDLE;
      }
    }
    free(app->cmd_pbs);
  }
  if (app->pipeline_cache)  /* leave like this for now */
    vkDestroyPipelineCache(app->device, app->pipeline_cache, NULL);
  if (app->gp_data) { /* leave like this for now */
    for (uint32_t i = 0; i < app->gpc; i++) {
      if (app->gp_data[i].pipeline_layout) {
        vkDestroyPipelineLayout(app->device, app->gp_data[i].pipeline_layout, NULL);
        app->gp_data[i].pipeline_layout = VK_NULL_HANDLE;
      }
      if (app->gp_data[i].render_pass) {
        vkDestroyRenderPass(app->device, app->gp_data[i].render_pass, NULL);
        app->gp_data[i].render_pass = VK_NULL_HANDLE;
      }
      uint32_t gps_size = sizeof(app->gp_data[i].graphics_pipelines) / sizeof(VkPipeline*);
      for (uint32_t j = 0; j < gps_size; j++) {
        if (app->gp_data[i].graphics_pipelines[j])
          vkDestroyPipeline(app->device, app->gp_data[i].graphics_pipelines[j], NULL);
        app->gp_data[i].graphics_pipelines = VK_NULL_HANDLE;
      }
    }
    free(app->gp_data);
  }
  if (app->desc_data) {
    for (uint32_t i = 0; i < app->ddc; i++) {
      if (app->desc_data[i].desc_layouts) {
        for (uint32_t j = 0; j < app->desc_data[i].dc; j++)
          vkDestroyDescriptorSetLayout(app->device, app->desc_data[i].desc_layouts[j], NULL);
        FREE(app->desc_data[i].desc_layouts);
      }
      if (app->desc_data[i].desc_set)
        FREE(app->desc_data[i].desc_set);
      if (app->desc_data[i].desc_pool) {
        vkDestroyDescriptorPool(app->device, app->desc_data[i].desc_pool, NULL);
        app->desc_data[i].desc_pool = VK_NULL_HANDLE;
      }
    }
    free(app->desc_data);
  }
  if (app->buffs_data) {
    for (uint32_t i = 0; i < app->bdc; i++) {
      if (app->buffs_data[i].buff) {
        vkDestroyBuffer(app->device, app->buffs_data[i].buff, NULL);
        app->buffs_data[i].buff = VK_NULL_HANDLE;
      }
      if (app->buffs_data[i].mem) {
        vkFreeMemory(app->device, app->buffs_data[i].mem, NULL);
        app->buffs_data[i].mem = VK_NULL_HANDLE;
      }
    }
    free(app->buffs_data);
  }
  if (app->sc) { /* Annihilate All Swap Chain Objects */
    for (uint32_t i = 0; i < app->scc; i++) {
      if (app->sc[i].depth.view)
        vkDestroyImageView(app->device, app->sc[i].depth.view, NULL);
      if (app->sc[i].depth.image)
        vkDestroyImage(app->device, app->sc[i].depth.image, NULL);
      if (app->sc[i].depth.mem)
        vkFreeMemory(app->device, app->sc[i].depth.mem, NULL);
      if (app->sc[i].sc_buffs && app->sc[i].frame_buffs && app->sc[i].sems) {
        for (uint32_t j = 0; j < app->sc[i].sic; j++) {
          if (app->sc[i].sems[j].image && app->sc[i].sems[j].render) {
            vkDestroySemaphore(app->device, app->sc[i].sems[j].image, NULL);
            vkDestroySemaphore(app->device, app->sc[i].sems[j].render, NULL);
          }
          vkDestroyFramebuffer(app->device, app->sc[i].frame_buffs[j], NULL);
          vkDestroyImageView(app->device, app->sc[i].sc_buffs[j].view, NULL);
        }
        FREE(app->sc[i].sc_buffs); FREE(app->sc[i].frame_buffs); FREE(app->sc[i].sems);
      }
      if (app->sc[i].swap_chain) {
        vkDestroySwapchainKHR(app->device, app->sc[i].swap_chain, NULL);
        app->sc[i].swap_chain = VK_NULL_HANDLE;
      }
    }
    free(app->sc);
  }
  if (app->device) {
    vkDeviceWaitIdle(app->device);
    vkDestroyDevice(app->device, NULL);
  }
  if (app->surface)
    vkDestroySurfaceKHR(app->instance, app->surface, NULL);
  if (app->instance)
    vkDestroyInstance(app->instance, NULL);

  set_vkcomp_init_values(app);
  FREE(app);
}

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

#ifndef DLU_DRM_CREATE_H
#define DLU_DRM_CREATE_H

/**
* Function sets up a VT/TTY so the process can in graphical mode.
* It also sets a process up so that it can handle its own input.
* If dlu_drm_create_kms_node somehow fails you can run just use this function, but sudo permission required.
*/
bool dlu_drm_create_vt(dlu_drm_core *core);
bool dlu_drm_create_kms_node(dlu_drm_core *core, const char *preferred_dev);
bool dlu_drm_create_gbm_device(dlu_drm_core *core);

/* One should query using the lucur(1) cmd tool to see if there device supports framebuffer modifiers */
bool dlu_drm_create_fb(
  dlu_drm_bo_type type,
  dlu_drm_core *core,
  uint32_t cur_bi,
  uint32_t cur_od,
  uint32_t format,
  uint32_t depth,
  uint32_t bpp,
  uint32_t bo_flags,
  uint32_t flags
);

bool dlu_drm_create_input_handle(dlu_drm_core *core);

#ifdef INAPI_CALLS
void dlu_drm_reset_vt(dlu_drm_core *core);
#endif
#endif
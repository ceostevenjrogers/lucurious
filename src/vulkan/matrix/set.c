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
#include <wlu/vlucur/matrix.h>
#include <wlu/utils/log.h>
#include <cglm/call.h>

float wlu_set_fovy(float fovy) {
  return glm_rad(fovy);
}

void wlu_set_perspective(
  vkcomp *app,
  float fovy,
  float aspect,
  float nearVal,
  float farVal
) {
  glmc_perspective(fovy, aspect, nearVal, farVal, app->ubd.proj);
}

void wlu_set_lookat(vkcomp *app, vec3 eye, vec3 center, vec3 up) {
  glm_lookat(eye, center, up, app->ubd.view);
}

/* made void * to check if memcpy worked */
void *wlu_set_matrix(void *matrix, void *model, uint32_t size) {
  matrix = memcpy(matrix, model, size);
  return (matrix) ? matrix : NULL;
}

/* made void * to check if memcpy worked */
void *wlu_set_vector(void *vector, float *vec, uint32_t size) {
  vector = memcpy(vector, vec, size);
  return (vector) ? vector : NULL;
}

void wlu_set_mvp_matrix(vkcomp *app) {
  glm_mat4_mulN((mat4 *[]){&app->ubd.clip, &app->ubd.proj,
                &app->ubd.view, &app->ubd.model}, 4, app->ubd.mvp);
}

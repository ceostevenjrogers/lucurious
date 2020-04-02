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

#define LUCUR_MATH_API
#include <lucom.h>

#include <cglm/call.h>

float wlu_set_radian(float fovy) {
  return glm_rad(fovy);
}

void wlu_set_perspective(
  mat4 proj,
  float fovy,
  float aspect,
  float nearPlane,
  float farPlane
) {
  glmc_perspective(fovy, aspect, nearPlane, farPlane, proj);
}

void wlu_set_lookat(mat4 view, vec3 eye, vec3 center, vec3 up) {
  glm_lookat(eye, center, up, view);
}

void wlu_set_mvp_matrix(mat4 mvp, mat4 *clip, mat4 *proj, mat4 *view, mat4 *model) {
  glm_mat4_mulN((mat4 *[]){clip, proj, view, model}, 4, mvp);
}

void wlu_set_matrix(wlu_matrix_type type, void *dest, void *src) {
  switch (type) {
    case WLU_MAT3: glm_mat3_copy(src, dest); break;
    case WLU_MAT4: glm_mat4_copy(src, dest); break;
    case WLU_MAT3_IDENTITY: glm_mat3_identity(dest); break;
    case WLU_MAT4_IDENTITY: glm_mat4_identity(dest); break;
    default: break;
  }
}

void wlu_set_vector(wlu_vec_type type, void *dest, void *src) {
  switch(type) {
    case WLU_VEC2:
      memmove(dest, src, sizeof(vec2)); /* cglm doesn't have a vec2 to copy func */
      break;
    case WLU_VEC3:
      glm_vec3_copy(src, dest);
      break;
    case WLU_VEC4:
      glm_vec4_copy(src, dest);
      break;
    default: break;
  }
}

void wlu_set_rotate(wlu_rotate_type type, void *dest, float rad, void *src) {
  switch (type) {
    case WLU_X: glm_rotate_x(src, rad, dest); break;
    case WLU_Y: glm_rotate_y(src, rad, dest); break;
    case WLU_Z: glm_rotate_z(src, rad, dest); break;
    case WLU_AXIS_Z: glm_rotate(dest, rad, src); break;
    default: break;
  }
}

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

#ifndef DLU_SPIRV_SHADE_H
#define DLU_SPIRV_SHADE_H

dlu_shader_info dlu_preprocess_shader(
  unsigned int kind,
  const char *source,
  const char *input_file_name,
  const char *entry_point_name
);

dlu_shader_info dlu_compile_to_assembly(
  unsigned int kind,
  const char *source,
  const char *input_file_name,
  const char *entry_point_name
);

/* Compile GLSL/HLSL into spirv byte code */
dlu_shader_info dlu_compile_to_spirv(
  unsigned int kind,
  const char *source,
  const char *input_file_name,
  const char *entry_point_name
);

#endif

/*
 * Copyright (c) 2014 Roman Kuznetsov 
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#pragma warning(disable:4996)

#include <string>
#include <list>
#include <map>
#include <vector>
#include <algorithm>
#include <memory>
#include <mutex>
#include <functional>
#include <sstream>

#include "matrix.h"
#include "vector.h"
#include "quaternion.h"
#include "ncamera2.h"
#include "bbox.h"

#include <windows.h>
#include "GL/gl3w.h"
#include "GL/wglext.h"

#include "window.h"

#include "openglcontext.h"

#include "logger.h"
#include "utils.h"
#include "timer.h"
#include "profiler.h"
#include "inputkeys.h"
#include "fpscounter.h"
#include "profiler.h"

#include "destroyable.h"
#include "geometry3D.h"
#include "line3D.h"
#include "texture.h"
#include "uniformBuffer.h"
#include "storageBuffer.h"
#include "atomicCounter.h"
#include "renderTarget.h"
#include "gpuprogram.h"
#include "standardGpuPrograms.h"

#include "freeCamera.h"
#include "lightManager.h"

#include "materialmanager.h"

#include "uimanager.h"
#include "uifactory.h"

#include "pipelinestate.h"

#include "application.h"

#undef min
#undef max
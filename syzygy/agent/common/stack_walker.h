// Copyright 2016 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SYZYGY_AGENT_COMMON_STACK_WALKER_H_
#define SYZYGY_AGENT_COMMON_STACK_WALKER_H_
#include "syzygy/common/asan_parameters.h"

// Defines an X86 stack walker for standard frames that contain a saved EBP
// value at the top, generated by the common preamble:
//
//   push ebp      // Save the previous EBP. ESP now points at the saved EBP.
//   mov ebp, esp  // EBP now points at the saved EBP.
//
// The algorithms expects the stack to be laid out as follows:
//
// +-------------+  <-- top of stack
// | ...data.... |
// | ret address |
// +-------------+
// |  saved ebp  |
// | ...data.... |  <-- frame 0 (root)
// | ret address |
// +-------------+
// |  saved ebp  |
// | ...data.... |  <-- frame 1
// | ret address |
// +-------------+
// .      .      .
// .      .      .
// +-------------+
// |  saved ebp  |
// | ...data.... |  <-- frame n (leaf)
// | ret address |
// +-------------+  <-- bottom of stack
//
// If the stack is truly laid out as above then there are a few invariants that
// must hold:
//
// - The return address of the previous frame is at EBP + 4. This must be a
//   sensible return address (non-null, and not in the stack itself).
// - Frames are laid out linearly on the stack, so successive EBP values must
//   be monotonically increasing.
// - Push aligns the stack to the machine word size, so all EBP values must be
//   4-byte aligned.
// - There must be the content of the saved EBP and a return pointer between
//   any two successive EBP values, so they must be at least 8 bytes apart.
// - The frames must be entirely contained within the stack itself, so strictly
//   between the known bottom and top of the stack.
//
// The algorithm walks the stack as far as it can while the above invariants
// hold true, saving the value of the return pointer for each valid frame
// encountered. Note that it can quickly derail if frame pointer optimization
// is enabled, or at any frame that uses a non-standard layout.
//
// All the information above is valid for X86. As for Win64, the only major
// difference is the register names and sizes, all the other principles
// still apply.


namespace agent {
namespace common {

using StackId = ::common::AsanStackId;

// Heuristically walks the current stack. Does not consider its own stack
// frame. Frames are expected to have a standard layout with the top of the
// frame being a saved frame pointer, and the bottom of a frame being a return
// address.
// @param bottom_frames_to_skip The number of frames to skip from the bottom
//     of the stack.
// @param max_frame_count The maximum number of frames that can be written to
//     @p frames.
// @param frames The array to be populated with the computed frames.
// @param absolute_stack_id Pointer to the stack ID that will be calculated as
//     we are walking the stack.
// @returns the number of frames successfully walked and stored in @p frames.
size_t WalkStack(uint32_t bottom_frames_to_skip,
                 uint32_t max_frame_count,
                 void** frames,
                 StackId* absolute_stack_id);

#ifndef _WIN64
// Implementation of WalkStack, with explicitly provided @p current_ebp,
// @p stack_bottom and @p stack_top. Exposed for much easier unittesting.
// @param current_ebp The current stack frame base to start walking from.
//     This must be a valid stack location from which to start walking.
// @param stack_bottom The bottom of the stack to walk. (Lower address.)
// @param stack_top The top of the stack to walk. (Higher address.)
// @param bottom_frames_to_skip The number of frames to skip from the bottom
//     of the stack.
// @param max_frame_count The maximum number of frames that can be written to
//     @p frames.
// @param frames The array to be populated with the computed frames.
// @param absolute_stack_id Pointer to the stack ID that will be calculated as
//     we are walking the stack.
// @returns the number of frames successfully walked and stored in @p frames.
size_t WalkStackImpl(const void* current_ebp,
                     const void* stack_bottom,
                     const void* stack_top,
                     uint32_t bottom_frames_to_skip,
                     uint32_t max_frame_count,
                     void** frames,
                     StackId* absolute_stack_id);
#endif  // !defined _WIN64

}  // namespace common
}  // namespace agent

#endif  // SYZYGY_AGENT_COMMON_STACK_WALKER_H_

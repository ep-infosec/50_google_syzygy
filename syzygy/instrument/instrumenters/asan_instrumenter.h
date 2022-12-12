// Copyright 2013 Google Inc. All Rights Reserved.
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
//
// Declares the Asan instrumenter.
#ifndef SYZYGY_INSTRUMENT_INSTRUMENTERS_ASAN_INSTRUMENTER_H_
#define SYZYGY_INSTRUMENT_INSTRUMENTERS_ASAN_INSTRUMENTER_H_

#include <string>

#include "base/command_line.h"
#include "syzygy/common/asan_parameters.h"
#include "syzygy/instrument/instrumenters/instrumenter_with_agent.h"
#include "syzygy/instrument/transforms/allocation_filter_transform.h"
#include "syzygy/instrument/transforms/asan_transform.h"
#include "syzygy/pe/image_filter.h"
#include "syzygy/pe/pe_relinker.h"

namespace instrument {
namespace instrumenters {

class AsanInstrumenter : public InstrumenterWithAgent {
 public:
  typedef InstrumenterWithAgent Super;

  AsanInstrumenter();
  ~AsanInstrumenter() { }

 protected:
  // @name InstrumenterWithAgent overrides.
  // @{
  bool ImageFormatIsSupported(ImageFormat image_format) override;
  bool InstrumentPrepare() override;
  bool InstrumentImpl() override;
  const char* InstrumentationMode() override { return "asan"; }
  // @}

  // @name Super overrides.
  // @{
  bool DoCommandLineParse(const base::CommandLine* command_line) override;
  // @}

  // @name Command-line parameters.
  // @{
  base::FilePath filter_path_;
  bool use_interceptors_;
  bool remove_redundant_checks_;
  bool use_liveness_analysis_;
  double instrumentation_rate_;
  bool asan_rtl_options_;
  bool hot_patching_;
  // @}

  // Valid if asan_rtl_options_ is true.
  common::InflatedAsanParameters asan_params_;

  // The transform for this agent.
  std::unique_ptr<instrument::transforms::AsanTransform> asan_transform_;

  // The image filter (optional).
  std::unique_ptr<pe::ImageFilter> filter_;

  // Path to the JSON configuration file for the AllocationFilter transform.
  // The AllocationFilter tranform is only applied if this config file is
  // specified.
  base::FilePath allocation_filter_config_file_path_;

  // The AllocationFilter transform.
  // TODO(sebmarchand): Write some integration tests for this.
  std::unique_ptr<instrument::transforms::AllocationFilterTransform>
      af_transform_;
};

}  // namespace instrumenters
}  // namespace instrument

#endif  // SYZYGY_INSTRUMENT_INSTRUMENTERS_ASAN_INSTRUMENTER_H_

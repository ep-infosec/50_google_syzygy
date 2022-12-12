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

#include "syzygy/instrument/instrumenters/asan_instrumenter.h"

#include <algorithm>

#include "base/logging.h"
#include "base/files/file_util.h"
#include "syzygy/application/application.h"
#include "syzygy/instrument/transforms/allocation_filter_transform.h"

namespace {
  using instrument::transforms::AllocationFilterTransform;
}

namespace instrument {
namespace instrumenters {

AsanInstrumenter::AsanInstrumenter()
    : use_interceptors_(true),
      remove_redundant_checks_(true),
      use_liveness_analysis_(true),
      instrumentation_rate_(1.0),
      asan_rtl_options_(false),
      hot_patching_(false) {
}

bool AsanInstrumenter::ImageFormatIsSupported(ImageFormat image_format) {
  if (image_format == BlockGraph::PE_IMAGE ||
      image_format == BlockGraph::COFF_IMAGE) {
    return true;
  }
  return false;
}

bool AsanInstrumenter::InstrumentPrepare() {
  return true;
}

bool AsanInstrumenter::InstrumentImpl() {
  // Parse the filter if one was provided.
  std::unique_ptr<pe::ImageFilter> filter;
  if (!filter_path_.empty()) {
    filter.reset(new pe::ImageFilter());
    if (!filter->LoadFromJSON(filter_path_)) {
      LOG(ERROR) << "Failed to parse filter file: " << filter_path_.value();
      return false;
    }

    // Ensure it is for the input module.
    if (!filter->IsForModule(input_image_path_)) {
      LOG(ERROR) << "Filter does not match the input module.";
      return false;
    }
  }

  asan_transform_.reset(new instrument::transforms::AsanTransform());
  asan_transform_->set_instrument_dll_name(agent_dll_);
  asan_transform_->set_use_interceptors(use_interceptors_);
  asan_transform_->set_use_liveness_analysis(use_liveness_analysis_);
  asan_transform_->set_remove_redundant_checks(remove_redundant_checks_);
  asan_transform_->set_instrumentation_rate(instrumentation_rate_);
  asan_transform_->set_hot_patching(hot_patching_);

  // Set up the filter if one was provided.
  if (filter.get()) {
    filter_.reset(filter.release());
    asan_transform_->set_filter(&filter_->filter);
  }

  // Set overwrite source range flag in the Asan transform. The Asan
  // transformation will overwrite the source range of created instructions to
  // the source range of corresponding instrumented instructions.
  asan_transform_->set_debug_friendly(debug_friendly_);

  // If RTL options were provided then pass them to the transform.
  if (asan_rtl_options_)
    asan_transform_->set_asan_parameters(&asan_params_);

  if (!relinker_->AppendTransform(asan_transform_.get()))
    return false;

  // Append the AllocationFilter transform if necessary.
  if (af_transform_.get() != nullptr) {
    if (!relinker_->AppendTransform(af_transform_.get()))
      return false;
  }

  return true;
}

bool AsanInstrumenter::DoCommandLineParse(
    const base::CommandLine* command_line) {
  if (!Super::DoCommandLineParse(command_line))
    return false;

  // Parse the additional command line arguments.
  filter_path_ = command_line->GetSwitchValuePath("filter");
  use_liveness_analysis_ = !command_line->HasSwitch("no-liveness-analysis");
  remove_redundant_checks_ = !command_line->HasSwitch("no-redundancy-analysis");
  use_interceptors_ = !command_line->HasSwitch("no-interceptors");
  hot_patching_ = command_line->HasSwitch("hot-patching");

  // Parse the instrumentation rate if one has been provided.
  static const char kInstrumentationRate[] = "instrumentation-rate";
  if (command_line->HasSwitch(kInstrumentationRate)) {
    std::string s = command_line->GetSwitchValueASCII(kInstrumentationRate);
    double d = 0;
    if (!base::StringToDouble(s, &d)) {
      LOG(ERROR) << "Failed to parse floating point value: " << s;
      return false;
    }
    // Cap the rate to the range of valid values [0, 1].
    instrumentation_rate_ = std::max(0.0, std::min(1.0, d));
  }

  // Parse Asan RTL options if present.
  asan_rtl_options_ = command_line->HasSwitch(common::kAsanRtlOptions);
  if (asan_rtl_options_) {
    std::wstring options =
        command_line->GetSwitchValueNative(common::kAsanRtlOptions);
    // The Asan RTL options string might be encapsulated in quotes, remove them
    // if it's the case.
    if (!options.empty() && options[0] == L'\"') {
      CHECK_EQ(L'\"', options.back()) << "If the asan-rtl-options string "
          << "starts with a quote it should also end with one.";
      options.erase(options.begin());
      if (!options.empty())
        options.pop_back();
    }
    common::SetDefaultAsanParameters(&asan_params_);
    if (!common::ParseAsanParameters(options, &asan_params_))
      return false;
  }

  // Parse the allocation-filter flag.
  static const char kAsanAllocationFilter[] = "allocation-filter-config-file";
  allocation_filter_config_file_path_ = command_line->GetSwitchValuePath(
      kAsanAllocationFilter);

  // Setup the AllocationFilter transform if a configuration file was specified.
  if (!allocation_filter_config_file_path_.empty()) {
    std::string json_string;
    AllocationFilterTransform::FunctionNameOffsetMap target_calls;
    if (!AllocationFilterTransform::ReadFromJSON(
      allocation_filter_config_file_path_, &target_calls)) {
      LOG(ERROR) << "Failed to parse allocation-filter configuration file: "
                 << allocation_filter_config_file_path_.value();
      return false;
    }

    if (!target_calls.empty()) {
      // Setup the allocation-filter transform.
      af_transform_.reset(new AllocationFilterTransform(target_calls));

      // Set overwrite source range flag in the AllocationFilter transform.
      // It will overwrite the source range of created instructions to the
      // source range of corresponding instrumented instructions. The
      // AllocationFilter transform shares the Asan flag.
      af_transform_->set_debug_friendly(debug_friendly_);
    }
  }

  // Set default agent dll name if none provided. This has to be done here
  // because ParseCommandLine expects agent_dll_ to be filled.
  if (agent_dll_.empty()) {
    if (!hot_patching_) {
      agent_dll_ = instrument::transforms::AsanTransform::kSyzyAsanDll;
    } else {
      agent_dll_ = instrument::transforms::AsanTransform::kSyzyAsanHpDll;
    }
  }

  return true;
}

}  // namespace instrumenters
}  // namespace instrument

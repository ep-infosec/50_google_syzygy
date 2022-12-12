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

#ifndef SYZYGY_REFINERY_ANALYZERS_TEB_ANALYZER_H_
#define SYZYGY_REFINERY_ANALYZERS_TEB_ANALYZER_H_

#include "base/macros.h"
#include "syzygy/refinery/analyzers/analyzer.h"
#include "syzygy/refinery/symbols/symbol_provider.h"

namespace refinery {

// The TEB analyzer types the TEBs assoicated to the threads in the dump.
class TebAnalyzer : public Analyzer {
 public:
  TebAnalyzer();

  const char* name() const override { return kTebAnalyzerName; }

  AnalysisResult Analyze(const minidump::Minidump& minidump,
                         const ProcessAnalysis& process_analysis) override;

  ANALYZER_INPUT_LAYERS(ProcessState::ModuleLayer)
  ANALYZER_OUTPUT_LAYERS(ProcessState::TypedBlockLayer)

 private:
  static const char kTebAnalyzerName[];

  DISALLOW_COPY_AND_ASSIGN(TebAnalyzer);
};

}  // namespace refinery

#endif  // SYZYGY_REFINERY_ANALYZERS_TEB_ANALYZER_H_

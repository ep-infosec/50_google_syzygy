@echo off
:: Copyright 2015 Google Inc. All Rights Reserved.
::
:: Licensed under the Apache License, Version 2.0 (the "License");
:: you may not use this file except in compliance with the License.
:: You may obtain a copy of the License at
::
::     http://www.apache.org/licenses/LICENSE-2.0
::
:: Unless required by applicable law or agreed to in writing, software
:: distributed under the License is distributed on an "AS IS" BASIS,
:: WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
:: See the License for the specific language governing permissions and
:: limitations under the License.
set SYZYGY_PYTHON="%~dp0..\..\..\third_party\python_26\python.exe"

%SYZYGY_PYTHON% "%~dp0system_interceptor_generator.py" ^
  --overwrite --output-base="%~dp0gen\\system_interceptors" ^
  --def-file="%~dp0syzyasan_rtl.def.template" ^
  "%~dp0system_interceptors_function_list.txt"
%SYZYGY_PYTHON% "%~dp0generate_memory_interceptors.py"

set SYZYGY_PYTHON=

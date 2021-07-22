# Copyright (c) 2021 pongasoft
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
#
# @author Yan Pujante

# Defines the location of the sources
set(RE_MOCK_CPP_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/src/cpp")

# Defines the headers if you want to include them in your project (optional)
set(re-mock_BUILD_HEADERS
    ${RE_MOCK_CPP_SRC_DIR}/re/mock/fmt.h
    ${RE_MOCK_CPP_SRC_DIR}/re/mock/stl.h
)

# Defines the sources
set(re-mock_BUILD_SOURCES
    ${RE_MOCK_CPP_SRC_DIR}/re/mock/Rack.cpp
    ${RE_MOCK_CPP_SRC_DIR}/re/mock/Jukebox.cpp
    ${RE_MOCK_CPP_SRC_DIR}/re/mock/LuaJBox.cpp
    ${RE_MOCK_CPP_SRC_DIR}/re/mock/MockDevices.cpp
    ${RE_MOCK_CPP_SRC_DIR}/re/mock/Motherboard.cpp
    )

# Defines the include directories
set(re-mock_INCLUDE_DIRECTORIES "${RE_MOCK_CPP_SRC_DIR}")


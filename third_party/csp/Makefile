#
# Copyright (c) 2017-present Samsung Electronics Co., Ltd
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#



BUILD_DIR = build
OUT_DIR = out


OS:=$(shell uname -s)
SRCS:=$(shell find source -type f -name "*.cpp" | sed 's/source\///')
OBJS:=$(SRCS:%.cpp=$(BUILD_DIR)/%.o)
INCLUDES = -Iinclude -Iconfig
CFLAGS= -Wall -g -O2 -fpermissive -D_LINUX -D_USE_PRINT -D_USE_ASSERT
TEST_SRC = AppMain.cpp
TEST_EXE = AppMain
TARGET_LIB = libcsp.a


$(BUILD_DIR)/%.o : source/%.cpp
	@mkdir -p $(BUILD_DIR)
	g++ $(CFLAGS) $(INCLUDES) -c -o $@ $<


$(OUT_DIR)/$(TEST_EXE): $(TEST_SRC) $(OUT_DIR)/$(TARGET_LIB)
	@mkdir -p $(OUT_DIR)
	g++ $(CFLAGS) -I./include -o $@ $(TEST_SRC) $(OUT_DIR)/$(TARGET_LIB) -lpthread


$(OUT_DIR)/$(TARGET_LIB): $(OBJS)
	@mkdir -p $(OUT_DIR)
	ar cru $@ $(OBJS)


all: $(OUT_DIR)/$(TEST_EXE) 


clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(OUT_DIR)





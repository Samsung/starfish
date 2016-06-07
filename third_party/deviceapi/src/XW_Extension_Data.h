/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef XWALK_EXTENSIONS_PUBLIC_XW_EXTENSION_DATA_H_
#define XWALK_EXTENSIONS_PUBLIC_XW_EXTENSION_DATA_H_

#ifndef XWALK_EXTENSIONS_PUBLIC_XW_EXTENSION_H_
#error "You should include XW_Extension.h before this file"
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// XW_INTERNAL_DATA_INTERFACE: allow extensions to exchange binary chunk data
// between extension and loader.
//

#define XW_INTERNAL_DATA_INTERFACE_1 \
  "XW_InternalDataInterface_1"
#define XW_INTERNAL_DATA_INTERFACE \
  XW_INTERNAL_DATA_INTERFACE_1

// Synchronous / Asynchronous data exchanging interface
typedef void (*XW_HandleDataCallback)(XW_Instance instance,
                                      const char* message,
                                      uint8_t* buffer, size_t len);

struct XW_Internal_DataInterface_1 {
  void (*RegisterSync)(XW_Extension extension,
                       XW_HandleDataCallback handle_data);

  void (*RegisterAsync)(XW_Extension extension,
                        XW_HandleDataCallback handle_data);

  void (*SetSyncReply)(XW_Instance instance, const char* reply,
                       uint8_t* buffer, size_t len);

  void (*PostData)(XW_Instance instance, const char* message,
                   uint8_t* buffer, size_t len);
};

typedef struct XW_Internal_DataInterface_1
    XW_Internal_DataInterface;

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // XWALK_EXTENSIONS_PUBLIC_XW_EXTENSION_DATA_H_

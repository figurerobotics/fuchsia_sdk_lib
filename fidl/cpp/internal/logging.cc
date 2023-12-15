// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "lib/fidl/cpp/internal/logging.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __Fuchsia__

#include <zircon/status.h>

#endif

namespace fidl {
namespace internal {

void ReportEncodingError(const HLCPPOutgoingMessage& message, const fidl_type_t* type,
                         const char* error_msg, const char* file, int line) {
  char type_name[1024];
  size_t type_name_length = fidl_format_type_name(type, type_name, sizeof(type_name));
  fprintf(stderr,
          "ERROR: [%s(%d)] fidl encoding error: %s. "
          "message: %.*s, %" PRIu32 " bytes, %" PRIu32 " handles\n",
          file, line, error_msg, static_cast<int>(type_name_length), type_name,
          message.bytes().actual(), message.handles().actual());
}

void ReportDecodingError(const HLCPPIncomingMessage& message, const fidl_type_t* type,
                         const char* error_msg, const char* file, int line) {
  char type_name[1024];
  size_t type_name_length = fidl_format_type_name(type, type_name, sizeof(type_name));
  fprintf(stderr,
          "ERROR: [%s(%d)] fidl decoding error: %s. "
          "message: %.*s, %" PRIu32 " bytes, %" PRIu32 " handles\n",
          file, line, error_msg, static_cast<int>(type_name_length), type_name,
          message.bytes().actual(), message.handles().actual());
}

void ReportValidatingError(const HLCPPOutgoingMessage& message, const fidl_type_t* type,
                           const char* error_msg, const char* file, int line) {
  char type_name[1024];
  size_t type_name_length = fidl_format_type_name(type, type_name, sizeof(type_name));
  fprintf(stderr,
          "ERROR: [%s(%d)] fidl validating error: %s. "
          "message: %.*s, %" PRIu32 " bytes, %" PRIu32 " handles\n",
          file, line, error_msg, static_cast<int>(type_name_length), type_name,
          message.bytes().actual(), message.handles().actual());
}

void ReportChannelWritingError(const HLCPPOutgoingMessage& message, const fidl_type_t* type,
                               zx_status_t status, const char* file, int line) {
  char type_name[1024];
  size_t type_name_length = fidl_format_type_name(type, type_name, sizeof(type_name));

#ifdef __Fuchsia__

  fprintf(stderr,
          "ERROR: [%s(%d)] fidl channel writing error: zx_status_t %d (%s). "
          "message: %.*s, %" PRIu32 " bytes, %" PRIu32 " handles\n",
          file, line, status, zx_status_get_string(status), static_cast<int>(type_name_length),
          type_name, message.bytes().actual(), message.handles().actual());

#else

  fprintf(stderr,
          "ERROR: [%s(%d)] fidl channel writing error: zx_status_t %d. "
          "message: %.*s, %" PRIu32 " bytes, %" PRIu32 " handles\n",
          file, line, status, static_cast<int>(type_name_length), type_name,
          message.bytes().actual(), message.handles().actual());

#endif
}

}  // namespace internal
}  // namespace fidl

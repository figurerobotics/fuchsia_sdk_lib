// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <lib/fdio/watcher.h>
#include <lib/zxio/types.h>
#include <zircon/types.h>

#include "sdk/lib/fdio/fdio_unistd.h"
#include "sdk/lib/fdio/internal.h"

namespace {

struct ZxioCallbackAdapterContext {
  int dirfd;
  watchdir_func_t cb;
  void* cookie;
};

}  // namespace

__EXPORT
zx_status_t fdio_watch_directory(int dirfd, watchdir_func_t cb, zx_time_t deadline, void* cookie) {
  const fbl::RefPtr<fdio> io = fd_to_io(dirfd);
  if (io == nullptr || cb == nullptr) {
    return ZX_ERR_INVALID_ARGS;
  }

  ZxioCallbackAdapterContext context{.dirfd = dirfd, .cb = cb, .cookie = cookie};

  zxio_watch_directory_cb zxio_cb = [](zxio_watch_directory_event_t event, const char* name,
                                       void* context) {
    auto* adapter_context = static_cast<ZxioCallbackAdapterContext*>(context);
    return adapter_context->cb(adapter_context->dirfd, event, name, adapter_context->cookie);
  };

  return io->watch_directory(zxio_cb, deadline, &context);
}

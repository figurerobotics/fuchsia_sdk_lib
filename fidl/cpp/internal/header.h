// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIB_FIDL_CPP_INTERNAL_HEADER_H_
#define LIB_FIDL_CPP_INTERNAL_HEADER_H_

// This header includes the necessary definitions to declare the high-level
// C++ FIDL binding proxies and stubs.

#include <lib/fit/function.h>
#include <lib/fpromise/result.h>
#include <lib/stdcompat/optional.h>

#include <array>
#include <functional>
#include <map>
#include <ostream>
#include <type_traits>

#ifdef __Fuchsia__
#include <lib/zx/bti.h>
#include <lib/zx/channel.h>
#include <lib/zx/clock.h>
#include <lib/zx/debuglog.h>
#include <lib/zx/event.h>
#include <lib/zx/eventpair.h>
#include <lib/zx/exception.h>
#include <lib/zx/fifo.h>
#include <lib/zx/guest.h>
#include <lib/zx/handle.h>
#include <lib/zx/interrupt.h>
#include <lib/zx/job.h>
#include <lib/zx/msi.h>
#include <lib/zx/object.h>
#include <lib/zx/pager.h>
#include <lib/zx/pmt.h>
#include <lib/zx/port.h>
#include <lib/zx/process.h>
#include <lib/zx/profile.h>
#include <lib/zx/resource.h>
#include <lib/zx/socket.h>
#include <lib/zx/stream.h>
#include <lib/zx/suspend_token.h>
#include <lib/zx/task.h>
#include <lib/zx/thread.h>
#include <lib/zx/time.h>
#include <lib/zx/timer.h>
#include <lib/zx/vcpu.h>
#include <lib/zx/vmar.h>
#include <lib/zx/vmo.h>
#endif

#include "lib/fidl/cpp/internal/logging.h"
#include "lib/fidl/cpp/internal/natural_types_header.h"
#include "lib/fidl/cpp/unknown_interactions_hlcpp.h"

#ifdef __Fuchsia__
#include "lib/fidl/cpp/interface_ptr.h"
#include "lib/fidl/cpp/internal/proxy_controller.h"
#include "lib/fidl/cpp/internal/stub_controller.h"
#include "lib/fidl/cpp/internal/synchronous_proxy.h"
#include "lib/fidl/cpp/member_connector.h"
#include "lib/fidl/cpp/service_handler_base.h"
#include "lib/fidl/cpp/synchronous_interface_ptr.h"
#endif

#endif  // LIB_FIDL_CPP_INTERNAL_HEADER_H_

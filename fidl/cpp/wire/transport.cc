// Copyright 2021 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <lib/fidl/cpp/wire/internal/transport.h>

namespace fidl {
namespace internal {

void* TransportContextBase::release(const TransportVTable* vtable) {
  ZX_DEBUG_ASSERT(vtable);
  ZX_ASSERT_MSG(vtable_, "context must be assigned a transport");
  ZX_ASSERT_MSG(vtable_->type == vtable->type,
                "cannot release context for different transport than used for creation");

  void* data = data_;
  vtable_ = nullptr;
  data_ = nullptr;
  return data;
}

OutgoingTransportContext::~OutgoingTransportContext() {
  if (vtable_ && vtable_->close_outgoing_transport_context) {
    vtable_->close_outgoing_transport_context(data_);
  }
}

AnyUnownedTransport MakeAnyUnownedTransport(const AnyTransport& transport) {
  return transport.borrow();
}

}  // namespace internal
}  // namespace fidl

// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <lib/fidl/cpp/wire/async_binding.h>
#include <lib/fidl/cpp/wire/async_transaction.h>
#include <lib/fidl/cpp/wire/message.h>
#include <lib/fidl/cpp/wire/server.h>
#include <zircon/assert.h>

namespace fidl {

namespace internal {

//
// Synchronous transaction methods
//

std::optional<DispatchError> SyncTransaction::Dispatch(
    fidl::IncomingHeaderAndMessage&& msg, internal::MessageStorageViewBase* storage_view) {
  ZX_ASSERT(binding_);
  binding_->interface()->dispatch_message(std::move(msg), this, storage_view);
  return error_;
}

zx_status_t SyncTransaction::Reply(fidl::OutgoingMessage* message, WriteOptions write_options) {
  ZX_ASSERT(txid_ != 0);
  auto txid = txid_;
  txid_ = 0;

  ZX_ASSERT(binding_);
  message->set_txid(txid);
  message->Write(binding_->transport(), std::move(write_options));
  return message->status();
}

void SyncTransaction::EnableNextDispatch() {
  if (!binding_)
    return;
  // Only allow one |EnableNextDispatch| call per transaction instance.
  if (binding_lifetime_extender_)
    return;

  // Keeping another strong reference to the binding ensures that binding
  // teardown will not complete until this |SyncTransaction| destructs, i.e.
  // until the server method handler returns.
  binding_lifetime_extender_ = binding_->shared_from_this();
  if (binding_->CheckForTeardownAndBeginNextWait() == ZX_OK) {
    *next_wait_begun_early_ = true;
  } else {
    // Propagate a placeholder error, such that the message handler will
    // terminate dispatch right after the processing of this transaction.
    error_ = DispatchError{UnbindInfo::Unbind(), ErrorOrigin::kReceive};
  }
}

void SyncTransaction::Close(zx_status_t epitaph) {
  if (!binding_)
    return;
  binding_ = nullptr;

  // If |EnableNextDispatch| was called, the dispatcher will not monitor
  // our |unbind_info_|; we should asynchronously request teardown.
  if (binding_lifetime_extender_) {
    binding_lifetime_extender_->Close(std::move(binding_lifetime_extender_), epitaph);
    return;
  }

  error_ = DispatchError{UnbindInfo::Close(epitaph), ErrorOrigin::kReceive};
}

void SyncTransaction::InternalError(UnbindInfo error, ErrorOrigin origin) {
  if (!binding_)
    return;
  binding_ = nullptr;

  // If |EnableNextDispatch| was called, the dispatcher will not monitor
  // our |unbind_info_|; we should asynchronously request teardown.
  if (binding_lifetime_extender_) {
    binding_lifetime_extender_->HandleError(std::move(binding_lifetime_extender_),
                                            DispatchError{error, origin});
    return;
  }

  error_ = DispatchError{error, origin};
}

std::unique_ptr<Transaction> SyncTransaction::TakeOwnership() {
  ZX_ASSERT(binding_);
  auto transaction = std::make_unique<AsyncTransaction>(std::move(*this));
  binding_ = nullptr;
  return transaction;
}

bool SyncTransaction::DidOrGoingToUnbind() {
  ZX_ASSERT(binding_);
  return binding_->IsDestructionImminent();
}

//
// Asynchronous transaction methods
//

zx_status_t AsyncTransaction::Reply(fidl::OutgoingMessage* message, WriteOptions write_options) {
  ZX_ASSERT(txid_ != 0);
  auto txid = txid_;
  txid_ = 0;

  std::shared_ptr<AsyncServerBinding> binding = binding_.lock();
  if (!binding)
    return ZX_ERR_CANCELED;

  message->set_txid(txid);
  message->Write(binding->transport(), std::move(write_options));
  return message->status();
}

void AsyncTransaction::EnableNextDispatch() {
  // Unreachable. Async completers don't expose |EnableNextDispatch|.
  __builtin_abort();
}

void AsyncTransaction::Close(zx_status_t epitaph) {
  if (auto binding = binding_.lock()) {
    binding->Close(std::move(binding), epitaph);
  }
}

void AsyncTransaction::InternalError(UnbindInfo error, ErrorOrigin origin) {
  if (auto binding = binding_.lock()) {
    binding->HandleError(std::move(binding), {error, origin});
  }
}

std::unique_ptr<Transaction> AsyncTransaction::TakeOwnership() {
  // Unreachable. Async completers don't expose |ToAsync|.
  __builtin_abort();
}

bool AsyncTransaction::DidOrGoingToUnbind() {
  if (auto binding = binding_.lock()) {
    return binding->IsDestructionImminent();
  }
  return true;
}

}  // namespace internal

}  // namespace fidl

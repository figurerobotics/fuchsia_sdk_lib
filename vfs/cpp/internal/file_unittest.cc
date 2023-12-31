// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <lib/async-loop/cpp/loop.h>
#include <lib/async-loop/default.h>
#include <lib/fdio/directory.h>
#include <lib/fdio/fd.h>
#include <lib/fdio/fdio.h>
#include <lib/fdio/limits.h>
#include <lib/vfs/cpp/internal/file.h>
#include <unistd.h>
#include <zircon/processargs.h>

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include <fbl/unique_fd.h>
#include <gtest/gtest.h>

namespace {

class TestFile : public vfs::internal::File {
 public:
  explicit TestFile(std::vector<uint8_t>* buffer) : buffer_(buffer) {}
  ~TestFile() override = default;

  zx_status_t ReadAt(uint64_t count, uint64_t offset, std::vector<uint8_t>* out_data) override {
    if (offset >= buffer_->size()) {
      return ZX_OK;
    }
    size_t actual = std::min(count, buffer_->size() - offset);
    out_data->resize(actual);
    std::copy_n(buffer_->begin() + offset, actual, out_data->begin());
    return ZX_OK;
  }

  zx_status_t WriteAt(std::vector<uint8_t> data, uint64_t offset, uint64_t* out_actual) override {
    if (offset >= buffer_->size()) {
      *out_actual = 0u;
      return ZX_OK;
    }
    size_t actual = std::min(data.size(), buffer_->size() - offset);
    std::copy_n(data.begin(), actual, buffer_->begin() + offset);
    *out_actual = actual;
    return ZX_OK;
  }

  uint64_t GetLength() override { return buffer_->size(); }

  size_t GetCapacity() override { return buffer_->size(); }

 protected:
  fuchsia::io::OpenFlags GetAllowedFlags() const override {
    return File::GetAllowedFlags() | fuchsia::io::OpenFlags::RIGHT_READABLE |
           fuchsia::io::OpenFlags::RIGHT_WRITABLE;
  }

 private:
  std::vector<uint8_t>* buffer_;
};

fbl::unique_fd OpenAsFD(vfs::internal::Node* node, async_dispatcher_t* dispatcher) {
  zx::channel local, remote;
  EXPECT_EQ(ZX_OK, zx::channel::create(0, &local, &remote));
  EXPECT_EQ(ZX_OK, node->Serve(fuchsia::io::OpenFlags::RIGHT_READABLE |
                                   fuchsia::io::OpenFlags::RIGHT_WRITABLE,
                               std::move(remote), dispatcher));
  fbl::unique_fd fd;
  EXPECT_EQ(ZX_OK, fdio_fd_create(local.release(), fd.reset_and_get_address()));
  return fd;
}

TEST(File, Control) {
  std::vector<uint8_t> store(12u);
  TestFile file(&store);

  async::Loop loop(&kAsyncLoopConfigNoAttachToCurrentThread);
  loop.StartThread("vfs test thread");

  fbl::unique_fd fd = OpenAsFD(&file, loop.dispatcher());
  ASSERT_TRUE(fd);

  ASSERT_EQ(4, write(fd.get(), "abcd", 4));
  EXPECT_EQ(0, strcmp("abcd", reinterpret_cast<char*>(store.data())));
  ASSERT_EQ(5, write(fd.get(), "exxxi", 5));
  EXPECT_EQ(0, strcmp("abcdexxxi", reinterpret_cast<char*>(store.data())));
  ASSERT_EQ(3, pwrite(fd.get(), "fgh", 3, 5));
  EXPECT_EQ(0, strcmp("abcdefghi", reinterpret_cast<char*>(store.data())));

  char buffer[1024];
  memset(buffer, 0, sizeof(buffer));
  ASSERT_EQ(7, pread(fd.get(), buffer, 7, 1));
  EXPECT_EQ(0, strcmp("bcdefgh", buffer));

  ASSERT_EQ(3, write(fd.get(), "jklmn", 5));
  EXPECT_EQ('l', store[store.size() - 1]);

  memset(buffer, 0, sizeof(buffer));
  ASSERT_EQ(4, pread(fd.get(), buffer, 10, 8));
  EXPECT_EQ(0, strcmp("ijkl", buffer));

  ASSERT_GE(0, close(fd.release()));
}

TEST(File, Clone) {
  std::vector<uint8_t> store(12u);
  TestFile file(&store);

  async::Loop loop(&kAsyncLoopConfigNoAttachToCurrentThread);
  loop.StartThread("vfs test thread");

  fbl::unique_fd fd = OpenAsFD(&file, loop.dispatcher());
  ASSERT_TRUE(fd);

  ASSERT_EQ(4, write(fd.get(), "abcd", 4));
  EXPECT_EQ(0, strcmp("abcd", reinterpret_cast<char*>(store.data())));

  zx::handle handle;
  ASSERT_EQ(ZX_OK, fdio_fd_clone(fd.get(), handle.reset_and_get_address()));

  fbl::unique_fd cloned;
  EXPECT_EQ(ZX_OK, fdio_fd_create(handle.release(), cloned.reset_and_get_address()));
  ASSERT_TRUE(cloned);

  ASSERT_EQ(3, write(cloned.get(), "xyz", 3));
  EXPECT_EQ(0, strcmp("xyzd", reinterpret_cast<char*>(store.data())));

  ASSERT_GE(0, close(fd.release()));
  ASSERT_GE(0, close(cloned.release()));
}

}  // namespace

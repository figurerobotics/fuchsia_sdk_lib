// Copyright 2021 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fcntl.h>
#include <lib/device-watcher/cpp/device-watcher.h>

#include <fbl/unique_fd.h>
#include <gtest/gtest.h>

TEST(PlatformDriverTestRealmTest, DriversExist) {
  fbl::unique_fd dev(open("/dev", O_RDONLY));
  ASSERT_TRUE(dev);

  ASSERT_EQ(ZX_OK, device_watcher::RecursiveWaitForFile(dev.get(), "sys/platform").status_value());
}

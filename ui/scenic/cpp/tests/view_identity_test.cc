// Copyright 2021 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fuchsia/ui/views/cpp/fidl.h>
#include <lib/ui/scenic/cpp/view_identity.h>
#include <zircon/rights.h>
#include <zircon/syscalls/object.h>
#include <zircon/types.h>

#include <gtest/gtest.h>

namespace {

TEST(ViewIdentityTest, ViewRefPairIsRelated) {
  auto [view_ref, control_ref] = scenic::NewViewIdentityOnCreation();

  // Get info about the |control_ref|.
  zx_info_handle_basic_t c_info;
  zx_status_t c_status = zx_object_get_info(control_ref.reference.get(), ZX_INFO_HANDLE_BASIC,
                                            &c_info, sizeof(c_info), nullptr, nullptr);
  EXPECT_EQ(c_status, ZX_OK);

  // Get info about the |view_ref|.
  zx_info_handle_basic_t v_info;
  zx_status_t v_status = zx_object_get_info(view_ref.reference.get(), ZX_INFO_HANDLE_BASIC, &v_info,
                                            sizeof(v_info), nullptr, nullptr);
  EXPECT_EQ(v_status, ZX_OK);

  // The refs' koids should always be related.
  EXPECT_EQ(c_info.koid, v_info.related_koid);
  EXPECT_EQ(c_info.related_koid, v_info.koid);
}

TEST(ViewIdentityTest, ViewRefPairRights) {
  auto [view_ref, control_ref] = scenic::NewViewIdentityOnCreation();

  // Get info about the |control_ref|.
  zx_info_handle_basic_t c_info;
  zx_status_t c_status = zx_object_get_info(control_ref.reference.get(), ZX_INFO_HANDLE_BASIC,
                                            &c_info, sizeof(c_info), nullptr, nullptr);
  EXPECT_EQ(c_status, ZX_OK);

  // Control ref has most eventpair rights, except for duplication.
  EXPECT_EQ(c_info.rights, ZX_DEFAULT_EVENTPAIR_RIGHTS & (~ZX_RIGHT_DUPLICATE));

  // Get info about the |view_ref|.
  zx_info_handle_basic_t v_info;
  zx_status_t v_status = zx_object_get_info(view_ref.reference.get(), ZX_INFO_HANDLE_BASIC, &v_info,
                                            sizeof(v_info), nullptr, nullptr);
  EXPECT_EQ(v_status, ZX_OK);

  // View ref has basic rights (but no signaling).
  EXPECT_EQ(v_info.rights, ZX_RIGHTS_BASIC);
}

}  // namespace

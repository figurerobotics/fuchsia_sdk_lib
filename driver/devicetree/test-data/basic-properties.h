// Copyright 2023 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIB_DRIVER_DEVICETREE_TEST_DATA_BASIC_PROPERTIES_H_
#define LIB_DRIVER_DEVICETREE_TEST_DATA_BASIC_PROPERTIES_H_

#define TEST_REG_A_BASE 0xAAAAAAAA
#define TEST_REG_A_LENGTH 0x2000

#define TEST_REG_B_BASE_WORD0 0xFAFAFAFA
#define TEST_REG_B_BASE_WORD1 0xBBBBBBBB
#define TEST_REG_B_LENGTH_WORD0 0
#define TEST_REG_B_LENGTH_WORD1 0x3000

#define TEST_REG_C_BASE_WORD0 0
#define TEST_REG_C_BASE_WORD1 0xCCCCCCCC
#define TEST_REG_C_LENGTH_WORD0 0x10000010
#define TEST_REG_C_LENGTH_WORD1 0x4000

// phandles used in testing.
#define TEST_IOMMU_PHANDLE 0x1

#define TEST_BTI_ID 0x2

#define DEVICE_SPECIFIC_PROP_VALUE 3

#endif  // LIB_DRIVER_DEVICETREE_TEST_DATA_BASIC_PROPERTIES_H_

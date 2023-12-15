// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fuchsia/inspect/cpp/fidl.h>
#include <lib/async/cpp/executor.h>
#include <lib/inspect/cpp/inspect.h>
#include <lib/inspect/service/cpp/reader.h>
#include <lib/inspect/service/cpp/service.h>
#include <lib/inspect/testing/cpp/inspect.h>

#include <gmock/gmock.h>

#include "src/lib/testing/loop_fixture/real_loop_fixture.h"

using inspect::Inspector;

namespace {

class InspectServiceTest : public gtest::RealLoopFixture {
 public:
  InspectServiceTest()
      : inspector_(),
        executor_(dispatcher()),
        handler_(inspect::MakeTreeHandler(&inspector_, dispatcher())) {}

  Inspector inspector_;

 protected:
  inspect::Node& root() { return inspector_.GetRoot(); }

  fuchsia::inspect::TreePtr Connect() {
    fuchsia::inspect::TreePtr ret;
    handler_(ret.NewRequest());
    return ret;
  }

  fuchsia::inspect::TreePtr ConnectFrozenThenLive() {
    handler_ = inspect::MakeTreeHandler(
        &inspector_, dispatcher(),
        inspect::TreeHandlerSettings{.snapshot_behavior = inspect::TreeServerSendPreference::Frozen(
                                         inspect::TreeServerSendPreference::Type::Live)});

    return Connect();
  }

  fuchsia::inspect::TreePtr ConnectFrozenThenDeepCopy() {
    handler_ = inspect::MakeTreeHandler(
        &inspector_, dispatcher(),
        inspect::TreeHandlerSettings{.snapshot_behavior = inspect::TreeServerSendPreference::Frozen(
                                         inspect::TreeServerSendPreference::Type::DeepCopy)});

    return Connect();
  }

  fuchsia::inspect::TreePtr ConnectPrivate() {
    handler_ = inspect::MakeTreeHandler(
        &inspector_, dispatcher(),
        inspect::TreeHandlerSettings{.snapshot_behavior =
                                         inspect::TreeServerSendPreference::DeepCopy()});

    return Connect();
  }

  fuchsia::inspect::TreePtr ConnectLive() {
    handler_ = inspect::MakeTreeHandler(
        &inspector_, dispatcher(),
        inspect::TreeHandlerSettings{.snapshot_behavior =
                                         inspect::TreeServerSendPreference::Live()});

    return Connect();
  }

  void MakeHandler(inspect::TreeHandlerSettings settings) {
    handler_ = inspect::MakeTreeHandler(&inspector_, dispatcher(), settings);
  }

  async::Executor executor_;

 private:
  fidl::InterfaceRequestHandler<fuchsia::inspect::Tree> handler_;
};

// The failure tests below are not perfect. They don't make any assertions
// about the the type fallback behavior that is specified. This is because
// triggering the failure of the primary behavior also causes the failure
// of DeepCopy fallback behavior, meaning that all the tests bottom out in
// Live duplicates (Live duplicate is the only behavior that never fails).
// Still, the tests demonstrate that even on failure, data is served via
// fallback.

TEST_F(InspectServiceTest, SingleTreeFailLive) {
  auto val = root().CreateInt("val", 1);

  auto ptr = ConnectLive();
  ptr.set_error_handler(
      [](zx_status_t status) { ASSERT_TRUE(false) << "Error detected on connection"; });

  fpromise::result<fuchsia::inspect::TreeContent> content;

  inspector_.AtomicUpdate([&](inspect::Node& n) {
    ptr->GetContent([&](fuchsia::inspect::TreeContent returned_content) {
      content = fpromise::ok(std::move(returned_content));
    });

    RunLoopUntil([&] { return !!content; });
  });

  auto hierarchy = inspect::ReadFromVmo(std::move(content.take_value().buffer().vmo));
  ASSERT_TRUE(hierarchy.is_ok());

  const inspect::IntPropertyValue* val_prop =
      hierarchy.value().node().get_property<inspect::IntPropertyValue>("val");
  ASSERT_NE(nullptr, val_prop);
  EXPECT_EQ(1, val_prop->value());
}

TEST_F(InspectServiceTest, SingleTreeFailDeepCopy) {
  auto val = root().CreateInt("val", 1);

  auto ptr = ConnectPrivate();
  ptr.set_error_handler(
      [](zx_status_t status) { ASSERT_TRUE(false) << "Error detected on connection"; });

  fpromise::result<fuchsia::inspect::TreeContent> content;

  inspector_.AtomicUpdate([&](inspect::Node& n) {
    ptr->GetContent([&](fuchsia::inspect::TreeContent returned_content) {
      content = fpromise::ok(std::move(returned_content));
    });

    RunLoopUntil([&] { return !!content; });
  });

  auto hierarchy = inspect::ReadFromVmo(std::move(content.take_value().buffer().vmo));
  ASSERT_TRUE(hierarchy.is_ok());

  const inspect::IntPropertyValue* val_prop =
      hierarchy.value().node().get_property<inspect::IntPropertyValue>("val");
  ASSERT_NE(nullptr, val_prop);
  EXPECT_EQ(1, val_prop->value());
}

TEST_F(InspectServiceTest, SingleTreeFailFrozenThenDeep) {
  auto val = root().CreateInt("val", 1);

  auto ptr = ConnectFrozenThenDeepCopy();
  ptr.set_error_handler(
      [](zx_status_t status) { ASSERT_TRUE(false) << "Error detected on connection"; });

  fpromise::result<fuchsia::inspect::TreeContent> content;

  inspector_.AtomicUpdate([&](inspect::Node& n) {
    ptr->GetContent([&](fuchsia::inspect::TreeContent returned_content) {
      content = fpromise::ok(std::move(returned_content));
    });

    RunLoopUntil([&] { return !!content; });
  });

  auto hierarchy = inspect::ReadFromVmo(std::move(content.take_value().buffer().vmo));
  ASSERT_TRUE(hierarchy.is_ok());

  const inspect::IntPropertyValue* val_prop =
      hierarchy.value().node().get_property<inspect::IntPropertyValue>("val");
  ASSERT_NE(nullptr, val_prop);
  EXPECT_EQ(1, val_prop->value());
}

TEST_F(InspectServiceTest, SingleTreeFailFrozenThenLive) {
  auto val = root().CreateInt("val", 1);

  auto ptr = ConnectFrozenThenLive();
  ptr.set_error_handler(
      [](zx_status_t status) { ASSERT_TRUE(false) << "Error detected on connection"; });

  fpromise::result<fuchsia::inspect::TreeContent> content;

  inspector_.AtomicUpdate([&](inspect::Node& n) {
    ptr->GetContent([&](fuchsia::inspect::TreeContent returned_content) {
      content = fpromise::ok(std::move(returned_content));
    });

    RunLoopUntil([&] { return !!content; });
  });

  auto hierarchy = inspect::ReadFromVmo(std::move(content.take_value().buffer().vmo));
  ASSERT_TRUE(hierarchy.is_ok());

  const inspect::IntPropertyValue* val_prop =
      hierarchy.value().node().get_property<inspect::IntPropertyValue>("val");
  ASSERT_NE(nullptr, val_prop);
  EXPECT_EQ(1, val_prop->value());
}

TEST_F(InspectServiceTest, SingleTree) {
  inspect::ValueList values;
  root().CreateInt("val", 1, &values);

  auto ptr = ConnectFrozenThenLive();
  ptr.set_error_handler(
      [](zx_status_t status) { ASSERT_TRUE(false) << "Error detected on connection"; });

  std::vector<std::string> names;
  bool done = false;
  fuchsia::inspect::TreeNameIteratorPtr name_iter;
  ptr->ListChildNames(name_iter.NewRequest());

  executor_.schedule_task(inspect::ReadAllChildNames(std::move(name_iter))
                              .and_then([&](std::vector<std::string>& promised_names) {
                                names = std::move(promised_names);
                                done = true;
                              }));

  RunLoopUntil([&] { return done; });

  EXPECT_TRUE(names.empty());
}

TEST_F(InspectServiceTest, SingleTreeGetContentCoW) {
  auto val = root().CreateInt("val", 1);

  auto ptr = ConnectFrozenThenLive();
  ptr.set_error_handler(
      [](zx_status_t status) { ASSERT_TRUE(false) << "Error detected on connection"; });

  fpromise::result<fuchsia::inspect::TreeContent> content;

  ptr->GetContent([&](fuchsia::inspect::TreeContent returned_content) {
    content = fpromise::ok(std::move(returned_content));
  });

  RunLoopUntil([&] { return !!content; });

  // copy-on-write -- this value won't appear in VMO obtained over FIDL
  val.Add(1);

  auto hierarchy = inspect::ReadFromVmo(std::move(content.take_value().buffer().vmo));
  ASSERT_TRUE(hierarchy.is_ok());

  const inspect::IntPropertyValue* val_prop =
      hierarchy.value().node().get_property<inspect::IntPropertyValue>("val");
  ASSERT_NE(nullptr, val_prop);
  EXPECT_EQ(1, val_prop->value());
}

TEST_F(InspectServiceTest, SingleTreeGetContentPrivate) {
  auto val = root().CreateInt("val", 1);

  auto ptr = ConnectPrivate();
  ptr.set_error_handler(
      [](zx_status_t status) { ASSERT_TRUE(false) << "Error detected on connection"; });

  fpromise::result<fuchsia::inspect::TreeContent> content;

  ptr->GetContent([&](fuchsia::inspect::TreeContent returned_content) {
    content = fpromise::ok(std::move(returned_content));
  });

  RunLoopUntil([&] { return !!content; });

  val.Add(1);

  auto hierarchy = inspect::ReadFromVmo(std::move(content.take_value().buffer().vmo));
  ASSERT_TRUE(hierarchy.is_ok());

  const inspect::IntPropertyValue* val_prop =
      hierarchy.value().node().get_property<inspect::IntPropertyValue>("val");
  ASSERT_NE(nullptr, val_prop);
  EXPECT_EQ(1, val_prop->value());
}

TEST_F(InspectServiceTest, ListChildNames) {
  inspect::ValueList values;
  root().CreateLazyNode(
      "a", []() { return fpromise::make_result_promise<Inspector>(fpromise::error()); }, &values);
  root().CreateLazyNode(
      "b", []() { return fpromise::make_result_promise<Inspector>(fpromise::error()); }, &values);

  auto ptr = ConnectFrozenThenLive();
  ptr.set_error_handler(
      [](zx_status_t status) { ASSERT_TRUE(false) << "Error detected on connection"; });

  std::vector<std::string> names;
  bool done = false;
  fuchsia::inspect::TreeNameIteratorPtr name_iter;
  ptr->ListChildNames(name_iter.NewRequest());

  executor_.schedule_task(inspect::ReadAllChildNames(std::move(name_iter))
                              .and_then([&](std::vector<std::string>& promised_names) {
                                names = std::move(promised_names);
                                done = true;
                              }));

  RunLoopUntil([&] { return done; });

  EXPECT_EQ(names, std::vector<std::string>({"a-0", "b-1"}));
}

TEST_F(InspectServiceTest, OpenChild) {
  inspect::ValueList values;
  root().CreateInt("val", 20, &values);
  root().CreateLazyNode(
      "valid",
      []() {
        Inspector insp;
        insp.GetRoot().CreateInt("val", 10, &insp);
        return fpromise::make_ok_promise(insp);
      },
      &values);

  root().CreateLazyNode(
      "invalid", [] { return fpromise::make_result_promise<Inspector>(fpromise::error()); },
      &values);

  auto ptr = ConnectFrozenThenLive();
  ptr.set_error_handler(
      [](zx_status_t status) { ASSERT_TRUE(false) << "Error detected on connection"; });

  std::vector<std::string> names;
  bool list_done = false;
  fuchsia::inspect::TreeNameIteratorPtr name_iter;
  fpromise::result<fuchsia::inspect::TreeContent> root, child;
  ptr->ListChildNames(name_iter.NewRequest());

  executor_.schedule_task(inspect::ReadAllChildNames(std::move(name_iter))
                              .and_then([&](std::vector<std::string>& promised_names) {
                                names = std::move(promised_names);
                                list_done = true;
                              }));

  ptr->GetContent(
      [&](fuchsia::inspect::TreeContent content) { root = fpromise::ok(std::move(content)); });
  fuchsia::inspect::TreePtr child_ptr;
  ptr->OpenChild("valid-0", child_ptr.NewRequest());
  child_ptr->GetContent(
      [&](fuchsia::inspect::TreeContent content) { child = fpromise::ok(std::move(content)); });

  bool read_error_done = false;
  bool missing_error_done = false;
  fuchsia::inspect::TreePtr read_error_ptr, missing_error_ptr;
  read_error_ptr.set_error_handler([&](zx_status_t status) { read_error_done = true; });
  missing_error_ptr.set_error_handler([&](zx_status_t status) { missing_error_done = true; });
  ptr->OpenChild("invalid-1", read_error_ptr.NewRequest());
  ptr->OpenChild("missing", missing_error_ptr.NewRequest());
  read_error_ptr->GetContent([](fuchsia::inspect::TreeContent unused) {});
  missing_error_ptr->GetContent([](fuchsia::inspect::TreeContent unused) {});

  RunLoopUntil([&] {
    return list_done && root.is_ok() && child.is_ok() && read_error_done && missing_error_done;
  });

  EXPECT_EQ(names, std::vector<std::string>({"invalid-1", "valid-0"}));
  auto root_hierarchy = inspect::ReadFromVmo(root.take_value().buffer().vmo).take_value();
  ASSERT_EQ(1u, root_hierarchy.node().properties().size());
  EXPECT_EQ("val", root_hierarchy.node().properties()[0].name());
  auto child_hierarchy = inspect::ReadFromVmo(child.take_value().buffer().vmo).take_value();
  ASSERT_EQ(1u, child_hierarchy.node().properties().size());
  EXPECT_EQ("val", child_hierarchy.node().properties()[0].name());
}

}  // namespace

// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fuchsia/inspect/cpp/fidl.h>
#include <lib/async/cpp/executor.h>
#include <lib/fpromise/single_threaded_executor.h>
#include <lib/inspect/cpp/inspect.h>
#include <lib/inspect/service/cpp/reader.h>
#include <lib/inspect/service/cpp/service.h>
#include <lib/inspect/testing/cpp/inspect.h>

#include <gmock/gmock.h>

#include "src/lib/testing/loop_fixture/real_loop_fixture.h"

using inspect::Hierarchy;
using inspect::Inspector;
using ::testing::AllOf;
using ::testing::UnorderedElementsAre;
using namespace inspect::testing;

namespace {

class InspectReaderTest : public gtest::RealLoopFixture {
 public:
  InspectReaderTest()
      : executor_(dispatcher()),
        inspector_(),
        handler_(inspect::MakeTreeHandler(&inspector_, dispatcher())) {}

 protected:
  inspect::Node& root() { return inspector_.GetRoot(); }

  fuchsia::inspect::TreePtr Connect(async_dispatcher_t* dispatcher = nullptr) {
    fuchsia::inspect::TreePtr ret;
    handler_(ret.NewRequest(dispatcher));
    return ret;
  }

  void ResetHandler(async_dispatcher_t* dispatcher) {
    handler_ = inspect::MakeTreeHandler(&inspector_, dispatcher);
  }

  async::Executor executor_;

 private:
  Inspector inspector_;
  fidl::InterfaceRequestHandler<fuchsia::inspect::Tree> handler_;
};

std::unique_ptr<inspect::ValueList> RecordValues(inspect::Node& root) {
  auto values = std::make_unique<inspect::ValueList>();
  root.CreateInt("val", 1, values.get());
  root.CreateLazyNode(
      "test",
      [] {
        Inspector insp;
        insp.GetRoot().CreateInt("val2", 2, &insp);
        insp.GetRoot().CreateLazyValues(
            "tempvals",
            [] {
              Inspector insp;
              insp.GetRoot().CreateInt("val3", 3, &insp);
              return fpromise::make_ok_promise(std::move(insp));
            },
            &insp);
        return fpromise::make_ok_promise(std::move(insp));
      },
      values.get());
  root.CreateLazyNode(
      "next",
      [] {
        Inspector insp;
        insp.GetRoot().CreateInt("val4", 4, &insp);
        return fpromise::make_ok_promise(std::move(insp));
      },
      values.get());
  root.CreateLazyNode(
      "node_error", [] { return fpromise::make_result_promise<Inspector>(fpromise::error()); },
      values.get());
  root.CreateLazyNode(
      "values_error", [] { return fpromise::make_result_promise<Inspector>(fpromise::error()); },
      values.get());

  return values;
}

TEST_F(InspectReaderTest, ReadHierarchy) {
  inspect::ValueList values;
  fpromise::result<Hierarchy> hierarchy;
  auto value_list = RecordValues(root());
  bool done = false;

  executor_.schedule_task(
      inspect::ReadFromTree(Connect()).then([&](fpromise::result<Hierarchy>& result) {
        hierarchy = std::move(result);
        done = true;
      }));

  RunLoopUntil([&] { return done; });

  ASSERT_TRUE(hierarchy.is_ok());

  EXPECT_THAT(
      hierarchy.value(),
      AllOf(NodeMatches(NameMatches("root")),
            ChildrenMatch(UnorderedElementsAre(
                AllOf(NodeMatches(
                    AllOf(NameMatches("test"),
                          PropertyList(UnorderedElementsAre(IntIs("val2", 2), IntIs("val3", 3)))))),
                AllOf(NodeMatches(AllOf(NameMatches("next"),
                                        PropertyList(UnorderedElementsAre(IntIs("val4", 4))))))))));
}

TEST_F(InspectReaderTest, ReadHierarchyWithDifferentDispatcher) {
  async::Loop loop(&kAsyncLoopConfigNoAttachToCurrentThread);
  ResetHandler(loop.dispatcher());
  inspect::ValueList values;
  fpromise::result<Hierarchy> hierarchy;
  auto value_list = RecordValues(root());
  bool done = false;

  async::Executor local_executor(loop.dispatcher());
  local_executor.schedule_task(inspect::ReadFromTree(Connect(loop.dispatcher()))
                                   .then([&](fpromise::result<Hierarchy>& result) {
                                     hierarchy = std::move(result);
                                     done = true;
                                   }));

  while (true) {
    loop.Run(zx::deadline_after(zx::msec(10)));
    if (done) {
      break;
    }
  }

  ASSERT_TRUE(hierarchy.is_ok());

  EXPECT_THAT(
      hierarchy.value(),
      AllOf(NodeMatches(NameMatches("root")),
            ChildrenMatch(UnorderedElementsAre(
                AllOf(NodeMatches(
                    AllOf(NameMatches("test"),
                          PropertyList(UnorderedElementsAre(IntIs("val2", 2), IntIs("val3", 3)))))),
                AllOf(NodeMatches(AllOf(NameMatches("next"),
                                        PropertyList(UnorderedElementsAre(IntIs("val4", 4))))))))));
}

}  // namespace

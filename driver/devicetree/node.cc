// Copyright 2023 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sdk/lib/driver/devicetree/node.h"

#include <fidl/fuchsia.driver.framework/cpp/fidl.h>
#include <fidl/fuchsia.hardware.platform.bus/cpp/fidl.h>
#include <lib/driver/logging/cpp/logger.h>

#include <string>

#include <bind/fuchsia/platform/cpp/bind.h>
#include <sdk/lib/driver/component/cpp/composite_node_spec.h>
#include <sdk/lib/driver/legacy-bind-constants/legacy-bind-constants.h>

namespace fdf {
using namespace fuchsia_driver_framework;
}

namespace fdf_devicetree {

Node::Node(const std::string_view name, devicetree::Properties properties, uint32_t id)
    : name_(name), id_(id) {
  pbus_node_.did() = bind_fuchsia_platform::BIND_PLATFORM_DEV_DID_DEVICETREE;
  pbus_node_.vid() = bind_fuchsia_platform::BIND_PLATFORM_DEV_VID_GENERIC;
  pbus_node_.instance_id() = id;
  pbus_node_.name() = std::string(name_);

  for (auto property : properties) {
    properties_.emplace(property.name, property.value);
  }
}

void Node::AddBindProperty(fuchsia_driver_framework::NodeProperty prop) {
  node_properties_.emplace_back(std::move(prop));
}

void Node::AddMmio(fuchsia_hardware_platform_bus::Mmio mmio) {
  if (!pbus_node_.mmio()) {
    pbus_node_.mmio() = std::vector<fuchsia_hardware_platform_bus::Mmio>();
  }
  pbus_node_.mmio()->emplace_back(std::move(mmio));
}

void Node::AddBti(fuchsia_hardware_platform_bus::Bti bti) {
  if (!pbus_node_.bti()) {
    pbus_node_.bti() = std::vector<fuchsia_hardware_platform_bus::Bti>();
  }
  pbus_node_.bti()->emplace_back(std::move(bti));
}

void Node::AddMetadata(fuchsia_hardware_platform_bus::Metadata metadata) {
  if (!pbus_node_.metadata()) {
    pbus_node_.metadata() = std::vector<fuchsia_hardware_platform_bus::Metadata>();
  }
  pbus_node_.metadata()->emplace_back(std::move(metadata));
}

zx::result<> Node::Publish(fdf::WireSyncClient<fuchsia_hardware_platform_bus::PlatformBus> &pbus,
                           fidl::SyncClient<fuchsia_driver_framework::CompositeNodeManager> &mgr) {
  if (node_properties_.empty()) {
    FDF_LOG(DEBUG, "Not publishing node '%.*s' because it has no bind properties.",
            (int)name().size(), name().data());
    return zx::ok();
  }

  // Pass properties to pbus node directly if we are not adding a composite spec.
  if (!composite_) {
    pbus_node_.properties() = node_properties_;
  }

  FDF_LOG(DEBUG, "Adding node '%.*s' to pbus.", (int)name().size(), name().data());
  fdf::Arena arena('PBUS');
  fidl::Arena fidl_arena;
  auto result = pbus.buffer(arena)->NodeAdd(fidl::ToWire(fidl_arena, pbus_node_));
  if (!result.ok()) {
    FDF_LOG(ERROR, "NodeAdd request failed: %s", result.FormatDescription().data());
    return zx::error(result.status());
  }
  if (result->is_error()) {
    FDF_LOG(ERROR, "NodeAdd failed: %s", zx_status_get_string(result->error_value()));
    return zx::error(result->error_value());
  }

  // Add composite node spec if composite.
  if (composite_) {
    // Construct the platform bus node.
    fdf::ParentSpec platform_node;
    platform_node.properties() = std::vector<fdf::NodeProperty>(node_properties_);
    platform_node.properties().emplace_back(fdf::NodeProperty{{
        .key = fdf::NodePropertyKey::WithIntValue(BIND_PROTOCOL),
        .value = fdf::NodePropertyValue::WithIntValue(bind_fuchsia_platform::BIND_PROTOCOL_DEVICE),
    }});
    platform_node.bind_rules() = std::vector<fdf::BindRule>{
        fdf::MakeAcceptBindRule(BIND_PLATFORM_DEV_VID,
                                bind_fuchsia_platform::BIND_PLATFORM_DEV_VID_GENERIC),
        fdf::MakeAcceptBindRule(BIND_PLATFORM_DEV_DID,
                                bind_fuchsia_platform::BIND_PLATFORM_DEV_DID_DEVICETREE),
        fdf::MakeAcceptBindRule(BIND_PLATFORM_DEV_INSTANCE_ID, id_),
    };

    // pbus node is always the primary parent for now.
    parents_.insert(parents_.begin(), std::move(platform_node));

    fdf::CompositeNodeSpec group;
    std::string name_final(name());
    // '@' is not a valid character in Node names as per driver framework.
    std::replace(name_final.begin(), name_final.end(), '@', '-');
    group.name() = name_final;
    group.parents() = std::move(parents_);

    auto devicegroup_result = mgr->AddSpec({std::move(group)});
    if (devicegroup_result.is_error()) {
      FDF_LOG(ERROR, "Failed to create composite node: %s",
              devicegroup_result.error_value().FormatDescription().data());
      return zx::error(devicegroup_result.error_value().is_framework_error()
                           ? devicegroup_result.error_value().framework_error().status()
                           : ZX_ERR_INVALID_ARGS);
    }
  }

  return zx::ok();
}

}  // namespace fdf_devicetree

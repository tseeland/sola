// Copyright 2023 The SOLA authors
//
// This file is part of DAISI.
//
// DAISI is free software: you can redistribute it and/or modify it under the terms of the GNU
// General Public License as published by the Free Software Foundation; version 2.
//
// DAISI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
// the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
// Public License for more details.
//
// You should have received a copy of the GNU General Public License along with DAISI. If not, see
// <https://www.gnu.org/licenses/>.
//
// SPDX-License-Identifier: GPL-2.0-only

#include "logical_agent.h"

#include <functional>

#include "cpps/common/uuid_generator.h"
#include "minhton/utils/config_reader.h"

namespace daisi::cpps::logical {

LogicalAgent::LogicalAgent(uint32_t device_id, std::shared_ptr<CppsLoggerNs3> logger,
                           AlgorithmConfig config_algo, bool first_node)
    : device_id_(device_id),
      logger_(std::move(logger)),
      algorithm_config_(std::move(config_algo)),
      first_node_(first_node) {}

void LogicalAgent::initCommunication() {
  const std::string config_file =
      first_node_ ? "configurations/root.yml" : "configurations/join.yml";

  sola::ManagementOverlayMinhton::Config config_mo = minhton::config::readConfig(config_file);

  if (!first_node_) {
    sola_ns3::SOLAWrapperNs3::setJoinIp(config_mo);
  }

  // Nothing to configure (yet)
  sola::EventDisseminationMinhcast::Config config_ed;

  uuid_ = UUIDGenerator::get()();
  logger_->setApplicationUUID(uuid_);

  auto message_recv_fct = [this](const sola::Message &msg) { this->messageReceiveFunction(msg); };
  auto topic_message_recv_fct = [this](const sola::TopicMessage &msg) {
    this->topicMessageReceiveFunction(msg);
  };

  sola_ = std::make_unique<sola_ns3::SOLAWrapperNs3>(
      config_mo, config_ed, message_recv_fct, topic_message_recv_fct, logger_, uuid_, device_id_);
}

void LogicalAgent::processMessage(const Message &msg) {
  for (auto &algorithm : algorithms_) {
    bool processed = std::visit([&](auto &&msg) { return algorithm->process(msg); }, msg);
    if (processed) {
      return;
    }
  }

  throw std::runtime_error("Failed to process message");
}

void LogicalAgent::messageReceiveFunction(const sola::Message &msg) {
  auto logical_message = deserialize(msg.content);
  processMessage(logical_message);
}

void LogicalAgent::topicMessageReceiveFunction(const sola::TopicMessage &msg) {
  auto logical_message = deserialize(msg.content);
  processMessage(logical_message);
}

bool LogicalAgent::isRunning() { return sola_->isStorageRunning(); }

bool LogicalAgent::canStop() { return sola_->canStop(); }

void LogicalAgent::prepareStop() { sola_->stop(); }

}  // namespace daisi::cpps::logical

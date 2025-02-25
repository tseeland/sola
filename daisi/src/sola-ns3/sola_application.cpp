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

#include "sola_application.h"

#include "minhton/utils/config_reader.h"
#include "utils/sola_utils.h"

using namespace ns3;

namespace daisi::sola_ns3 {

TypeId SolaApplication::GetTypeId() {
  static TypeId tid =
      TypeId("NatterApp")
          .SetParent<Application>()
          .AddConstructor<SolaApplication>()
          .AddAttribute("LocalIpAddress", "LocalIpAddress", Ipv4AddressValue(),
                        MakeIpv4AddressAccessor(&SolaApplication::local_ip_address_),
                        MakeIpv4AddressChecker())
          .AddAttribute("ListeningPort", "ListeningPort", UintegerValue(0),
                        MakeUintegerAccessor(&SolaApplication::listening_port_),
                        MakeUintegerChecker<uint16_t>())
          .AddAttribute("ID", "ID", UintegerValue(0), MakeUintegerAccessor(&SolaApplication::id_),
                        MakeUintegerChecker<uint32_t>());
  return tid;
}

void SolaApplication::DoDispose() {
  StopApplication();
  Application::DoDispose();
}

void SolaApplication::StartApplication() {}

void SolaApplication::StopApplication() {}

uint16_t SolaApplication::getPort() const { return listening_port_; }

std::string SolaApplication::getIP() const {
  std::stringstream ss;
  local_ip_address_.Print(ss);
  return ss.str();
}

void SolaApplication::startSOLA() {
  SolaNetworkUtils::get().registerNode(local_ip_address_, GetNode(), listening_port_);
  SolaNetworkUtils::get().createSockets(getIpv4AddressString(local_ip_address_), 3);

  thread_local static uint32_t number_created = 0;

  const std::string config_file =
      (number_created == 0) ? "configurations/root.yml" : "configurations/join.yml";

  sola::ManagementOverlayMinhton::Config config_mo = minhton::config::readConfig(config_file);

  // Nothing to configure (yet)
  sola::EventDisseminationMinhcast::Config config_ed;
  number_created++;

  auto single_receive = [](const sola::Message &msg) {
    throw std::runtime_error("not implemented");
  };
  auto topic_receive = [id = id_](const sola::TopicMessage &msg) {
    std::cout << "[" << Simulator::Now().GetMilliSeconds() << "] RECEIVE TOPIC MESSAGE "
              << msg.topic << " ON NODE " << id << " WITH LENGTH " << msg.content.size() << " FROM "
              << msg.sender << std::endl;
  };

  auto logger = daisi::global_logger_manager->createSolaLogger(GetNode()->GetId());
  sola_ = std::make_unique<sola_ns3::SOLAWrapperNs3>(
      config_mo, config_ed, single_receive, topic_receive, logger, "",
      GetNode()->GetId());  // Already joining overlay network
}

void SolaApplication::subscribeTopic(const std::string &topic) { sola_->subscribeTopic(topic); }

void SolaApplication::publishTopic(const std::string &topic, const std::string &msg) {
  sola_->publishMessage(topic, msg);
}
}  // namespace daisi::sola_ns3

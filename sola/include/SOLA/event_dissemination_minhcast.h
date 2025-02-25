// Copyright The SOLA Contributors
//
// Licensed under the MIT License.
// For details on the licensing terms, see the LICENSE file.
// SPDX-License-Identifier: MIT

#ifndef SOLA_EVENT_DISSEMINATION_MINHCAST_H_
#define SOLA_EVENT_DISSEMINATION_MINHCAST_H_

#include "SOLA/message.h"
#include "event_dissemination/event_dissemination.h"
#include "minhton/core/minhton.h"
#include "natter/natter_minhcast.h"
#include "storage/storage.h"

struct Info {
  std::string ip;
  uint16_t port;

  bool operator==(const Info &other) const { return ip == other.ip && port == other.port; }
  bool operator<(const Info &other) const { return ip < other.ip || port < other.port; }
};

namespace std {
template <> struct hash<Info> {
  size_t operator()(const Info &p) const {
    return std::hash<std::string>()(p.ip) ^ std::hash<uint16_t>()(p.port);
  }
};
}  // namespace std

namespace sola {

struct EventDisseminationMinhcastConfig {
  std::vector<natter::logging::LoggerPtr> logger;
};

class EventDisseminationMinhcast final : public EventDissemination<minhton::Logger::LoggerPtr> {
public:
  using Config = EventDisseminationMinhcastConfig;
  using Logger = natter::logging::LoggerPtr;

  EventDisseminationMinhcast(TopicMessageReceiveFct msgRecvFct, std::shared_ptr<Storage> storage_,
                             std::string ip, const Config &config);
  ~EventDisseminationMinhcast() override = default;
  void publish(const std::string &topic, const std::string &message) final;
  void subscribe(const std::string &topic, std::vector<minhton::Logger::LoggerPtr> logger) final;
  void unsubscribe(const std::string &topic) final;

  void stop() final;
  bool canStop() const final;

private:
  void waitForResults(const std::string &topic);
  void joinMinhton(minhton::FindResult result, const std::string &topic,
                   const std::vector<minhton::Logger::LoggerPtr> &logger);

  // Implementation in event_dissemination_minhcast_impl.cpp
  void getResult(const std::string &topic, const std::function<void()> &on_result);
  void checkTopicJoin(const std::string &topic, bool should_exist);

  std::unique_ptr<natter::minhcast::NatterMinhcast> minhcast_;

  using Minhton = std::unique_ptr<minhton::Minhton>;
  using Topic = std::string;
  using MinhtonFuture = std::future<minhton::FindResult>;

  std::unordered_map<Topic, MinhtonFuture> result_;

  struct MinhtonTopicLogger {
    Topic topic;
    std::vector<minhton::Logger::LoggerPtr> logger;
  };

  std::vector<MinhtonTopicLogger> minhton_loggers_;
  std::string ip_;  // IP of ourself

  std::unordered_map<Topic, Minhton> topic_trees_;
  std::unordered_map<Info, uint32_t> peers_added_natter_;
  std::shared_ptr<Storage> storage_;

  bool stopping_ = false;
};
}  // namespace sola

#endif

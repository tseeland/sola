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

#include "iterated_auction_disposition_initiator.h"

#include "cpps/amr/model/amr_fleet.h"
#include "ns3/simulator.h"

namespace daisi::cpps::logical {

IteratedAuctionDispositionInitiator::IteratedAuctionDispositionInitiator(
    std::shared_ptr<sola_ns3::SOLAWrapperNs3> sola, std::shared_ptr<CppsLoggerNs3> logger)
    : DispositionInitiator(sola, logger) {
  // assuming that sola is fully initialized at this point

  auto preparation_duration = prepareInteraction();

  ns3::Simulator::Schedule(ns3::Seconds(preparation_duration),
                           &IteratedAuctionDispositionInitiator::setPreparationFinished, this);
}

void IteratedAuctionDispositionInitiator::addMaterialFlow(
    std::shared_ptr<material_flow::MFDLScheduler> scheduler) {
  if (layered_precedence_graph_) {
    throw std::runtime_error("A material flow is already processed currently. Support of multiple "
                             "is not implemented yet.");
  }

  layered_precedence_graph_ = std::make_shared<LayeredPrecedenceGraph>(scheduler);
  auction_initiator_state_ = std::make_unique<AuctionInitiatorState>(layered_precedence_graph_);

  auto sim_time = (double)ns3::Simulator::Now().GetMilliSeconds();
  for (const auto &task : layered_precedence_graph_->getAuctionableTasks()) {
    layered_precedence_graph_->setEarliestValidStartTime(task.getUuid(), sim_time);
  }

  if (preparation_finished_) {
    startIteration();
  }
}

daisi::util::Duration IteratedAuctionDispositionInitiator::prepareInteraction() {
  auto available_abilities = AmrFleet::get().getAllExistingAbilities();
  uint8_t topic_counter = 0;

  for (const auto &ability : available_abilities) {
    std::string topic_for_ability = AmrFleet::get().getTopicForAbility(ability);

    ability_topic_mapping_[ability] = topic_for_ability;

    ns3::Simulator::Schedule(ns3::Seconds(delays_.subscribe_topic * topic_counter++),
                             &daisi::sola_ns3::SOLAWrapperNs3::subscribeTopic, sola_.get(),
                             topic_for_ability);
  }

  return delays_.subscribe_topic * topic_counter;
}

void IteratedAuctionDispositionInitiator::setPreparationFinished() { preparation_finished_ = true; }

void IteratedAuctionDispositionInitiator::startIteration() {
  // Sending CallForProposal messages to initiate the auction.
  callForProposal();

  // Starting loop to assign all auctionable tasks
  ns3::Simulator::Schedule(ns3::Seconds(delays_.waiting_to_receive_bids),
                           &IteratedAuctionDispositionInitiator::bidProcessing, this);
}

void IteratedAuctionDispositionInitiator::finishIteration() {
  // Moving all free tasks from the free layer to the scheduled layer
  // and updating the other layers accordingly
  layered_precedence_graph_->next();

  // Clearing all stored bid submissions and winner responses
  auction_initiator_state_->clearIterationInfo();

  // Starting next iteration if there are still tasks left to be scheduled
  if (!layered_precedence_graph_->areAllTasksScheduled()) {
    startIteration();
  } else {
    layered_precedence_graph_ = nullptr;
    auction_initiator_state_ = nullptr;
  }
}

void IteratedAuctionDispositionInitiator::bidProcessing() {
  // Receiving bids in the meantime
  auction_initiator_state_->countBidSubmissionProcessing();

  // Selecting winners
  const auto winners = auction_initiator_state_->selectWinner();
  if (!winners.empty()) {
    // Sending WinnerResponse messages to winners
    notifyWinners(winners);

    // Scheduling the processing of winner responses
    ns3::Simulator::Schedule(ns3::Seconds(delays_.waiting_to_receive_winner_responses),
                             &IteratedAuctionDispositionInitiator::winnerResponseProcessing, this);
  } else {
    // If no winners were found, we renotify the participants with IterationNotifcations
    iterationNotification(layered_precedence_graph_->getAuctionableTasks());

    // Continuing the loop
    ns3::Simulator::Schedule(ns3::Seconds(delays_.waiting_to_receive_bids),
                             &IteratedAuctionDispositionInitiator::bidProcessing, this);
  }
}

void IteratedAuctionDispositionInitiator::winnerResponseProcessing() {
  // Receiving WinnerResponse messages in the meantime
  auction_initiator_state_->countWinnerResponseProcessing();

  auto auctioned_tasks = auction_initiator_state_->processWinnerAcceptions();

  // Sending IterationNotifications to notify other participants
  iterationNotification(auctioned_tasks);

  if (layered_precedence_graph_->areAllFreeTasksScheduled()) {
    // If no tasks are left in this iteration, finishing the iteration
    finishIteration();
  } else {
    // Continuing the loop as there are still unscheduled tasks left in this iteration
    ns3::Simulator::Schedule(ns3::Seconds(delays_.waiting_to_receive_bids),
                             &IteratedAuctionDispositionInitiator::bidProcessing, this);
  }
}

void IteratedAuctionDispositionInitiator::callForProposal() {
  auto initiator_connection = sola_->getConectionString();

  // Mapping of which tasks should be published with a CallForProposal on which ability topic.
  auto task_ability_mapping =
      getTaskAbilityMapping(layered_precedence_graph_->getAuctionableTasks());

  for (const auto &pair : task_ability_mapping) {
    auto topic = ability_topic_mapping_[pair.first];

    CallForProposal cfp(initiator_connection, pair.second);
    sola_->publishMessage(topic, serialize(cfp), "TODO log cfp");
  }
}

void IteratedAuctionDispositionInitiator::iterationNotification(
    const std::vector<material_flow::Task> &tasks) {
  auto initiator_connection = sola_->getConectionString();

  // Mapping of which tasks should be published with an IterationNotification on which ability
  // topic.
  auto task_ability_mapping = getTaskAbilityMapping(tasks);

  for (const auto &[ability, tasks_for_ability] : task_ability_mapping) {
    auto topic = ability_topic_mapping_[ability];

    std::vector<std::string> task_uuids;
    std::transform(tasks_for_ability.begin(), tasks_for_ability.end(),
                   std::back_inserter(task_uuids),
                   [&](const auto &task) { return task.getUuid(); });

    IterationNotification notification(initiator_connection, task_uuids);
    sola_->publishMessage(topic, serialize(notification), "TODO log iteration notification");
  }
}

void IteratedAuctionDispositionInitiator::notifyWinners(
    const std::vector<AuctionInitiatorState::Winner> &winners) {
  auction_initiator_state_->clearWinnerAcceptions();
  auto initiator_connection = sola_->getConectionString();

  for (const auto &winner : winners) {
    WinnerNotification notification(winner.task_uuid, initiator_connection,
                                    winner.latest_finish_time);

    sola_->sendData(serialize(notification), sola::Endpoint(winner.winner_connection));
  }
}

std::unordered_map<amr::AmrStaticAbility, std::vector<material_flow::Task>,
                   amr::AmrStaticAbilityHasher>
IteratedAuctionDispositionInitiator::getTaskAbilityMapping(
    const std::vector<material_flow::Task> &tasks) {
  std::unordered_map<amr::AmrStaticAbility, std::vector<material_flow::Task>,
                     amr::AmrStaticAbilityHasher>
      task_ability_mapping;

  for (const auto &task : tasks) {
    auto fitting_abilities =
        AmrFleet::get().getFittingExistingAbilities(task.getAbilityRequirement());
    for (const auto &ability : fitting_abilities) {
      task_ability_mapping[ability].push_back(task);
    }
  }

  return task_ability_mapping;
}

void IteratedAuctionDispositionInitiator::logMaterialFlowContent(
    const std::string &material_flow_uuid) {
  for (const auto &task : layered_precedence_graph_->getTasks()) {
    logger_->logMaterialFlowTask(task, material_flow_uuid);

    for (const auto &order : task.getOrders()) {
      logger_->logMaterialFlowOrder(order, task.getUuid());
    }

    logMaterialFlowOrderStatesOfTask(task, OrderStates::kCreated);
  }
}

void IteratedAuctionDispositionInitiator::logMaterialFlowOrderStatesOfTask(
    const material_flow::Task &task, const OrderStates &order_state) {
  for (auto i = 0; i < task.getOrders().size(); i++) {
    MaterialFlowOrderUpdateLoggingInfo logging_info;
    logging_info.task = task;
    logging_info.order_index = i;
    logging_info.order_state = order_state;

    logger_->logMaterialFlowOrderUpdate(logging_info);
  }
}

bool IteratedAuctionDispositionInitiator::process(const BidSubmission &bid_submission) {
  auction_initiator_state_->addBidSubmission(bid_submission);
  return true;
}

bool IteratedAuctionDispositionInitiator::process(const WinnerResponse &winner_response) {
  if (winner_response.doesAccept()) {
    auto task = layered_precedence_graph_->getTask(winner_response.getTaskUuid());
    logMaterialFlowOrderStatesOfTask(task, OrderStates::kQueued);
  }

  auction_initiator_state_->addWinnerResponse(winner_response);

  return true;
}

}  // namespace daisi::cpps::logical

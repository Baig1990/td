//
// Copyright Aliaksei Levin (levlam@telegram.org), Arseny Smirnov (arseny30@gmail.com) 2014-2023
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "td/telegram/MinChannel.hpp"
#include "td/telegram/PollManager.h"
#include "td/telegram/UserId.h"
#include "td/telegram/Version.h"

#include "td/utils/algorithm.h"
#include "td/utils/common.h"
#include "td/utils/tl_helpers.h"

namespace td {

template <class StorerT>
void PollManager::PollOption::store(StorerT &storer) const {
  using ::td::store;
  BEGIN_STORE_FLAGS();
  STORE_FLAG(is_chosen_);
  END_STORE_FLAGS();

  store(text_, storer);
  store(data_, storer);
  store(voter_count_, storer);
}

template <class ParserT>
void PollManager::PollOption::parse(ParserT &parser) {
  using ::td::parse;
  BEGIN_PARSE_FLAGS();
  PARSE_FLAG(is_chosen_);
  END_PARSE_FLAGS();

  parse(text_, parser);
  parse(data_, parser);
  parse(voter_count_, parser);
}

template <class StorerT>
void PollManager::Poll::store(StorerT &storer) const {
  using ::td::store;
  bool is_public = !is_anonymous_;
  bool has_open_period = open_period_ != 0;
  bool has_close_date = close_date_ != 0;
  bool has_explanation = !explanation_.text.empty();
  bool has_recent_voter_dialog_ids = !recent_voter_dialog_ids_.empty();
  bool has_recent_voter_min_channels = !recent_voter_min_channels_.empty();
  BEGIN_STORE_FLAGS();
  STORE_FLAG(is_closed_);
  STORE_FLAG(is_public);
  STORE_FLAG(allow_multiple_answers_);
  STORE_FLAG(is_quiz_);
  STORE_FLAG(false);
  STORE_FLAG(has_open_period);
  STORE_FLAG(has_close_date);
  STORE_FLAG(has_explanation);
  STORE_FLAG(is_updated_after_close_);
  STORE_FLAG(has_recent_voter_dialog_ids);
  STORE_FLAG(has_recent_voter_min_channels);
  END_STORE_FLAGS();

  store(question_, storer);
  store(options_, storer);
  store(total_voter_count_, storer);
  if (is_quiz_) {
    store(correct_option_id_, storer);
  }
  if (has_open_period) {
    store(open_period_, storer);
  }
  if (has_close_date) {
    store(close_date_, storer);
  }
  if (has_explanation) {
    store(explanation_, storer);
  }
  if (has_recent_voter_dialog_ids) {
    store(recent_voter_dialog_ids_, storer);
  }
  if (has_recent_voter_min_channels) {
    store(recent_voter_min_channels_, storer);
  }
}

template <class ParserT>
void PollManager::Poll::parse(ParserT &parser) {
  using ::td::parse;
  bool is_public;
  bool has_recent_voter_user_ids;
  bool has_open_period;
  bool has_close_date;
  bool has_explanation;
  bool has_recent_voter_dialog_ids;
  bool has_recent_voter_min_channels;
  BEGIN_PARSE_FLAGS();
  PARSE_FLAG(is_closed_);
  PARSE_FLAG(is_public);
  PARSE_FLAG(allow_multiple_answers_);
  PARSE_FLAG(is_quiz_);
  PARSE_FLAG(has_recent_voter_user_ids);
  PARSE_FLAG(has_open_period);
  PARSE_FLAG(has_close_date);
  PARSE_FLAG(has_explanation);
  PARSE_FLAG(is_updated_after_close_);
  PARSE_FLAG(has_recent_voter_dialog_ids);
  PARSE_FLAG(has_recent_voter_min_channels);
  END_PARSE_FLAGS();
  is_anonymous_ = !is_public;

  parse(question_, parser);
  parse(options_, parser);
  parse(total_voter_count_, parser);
  if (is_quiz_) {
    parse(correct_option_id_, parser);
    if (correct_option_id_ < -1 || correct_option_id_ >= static_cast<int32>(options_.size())) {
      parser.set_error("Wrong correct_option_id");
    }
  }
  if (has_recent_voter_user_ids) {
    vector<UserId> recent_voter_user_ids;
    parse(recent_voter_user_ids, parser);
    recent_voter_dialog_ids_ = transform(recent_voter_user_ids, [](UserId user_id) { return DialogId(user_id); });
  }
  if (has_open_period) {
    parse(open_period_, parser);
  }
  if (has_close_date) {
    parse(close_date_, parser);
  }
  if (has_explanation) {
    parse(explanation_, parser);
  }
  if (has_recent_voter_dialog_ids) {
    parse(recent_voter_dialog_ids_, parser);
  }
  if (has_recent_voter_min_channels) {
    parse(recent_voter_min_channels_, parser);
  }
}

template <class StorerT>
void PollManager::store_poll(PollId poll_id, StorerT &storer) const {
  td::store(poll_id.get(), storer);
  if (is_local_poll_id(poll_id)) {
    auto poll = get_poll(poll_id);
    CHECK(poll != nullptr);
    bool has_open_period = poll->open_period_ != 0;
    bool has_close_date = poll->close_date_ != 0;
    bool has_explanation = !poll->explanation_.text.empty();
    BEGIN_STORE_FLAGS();
    STORE_FLAG(poll->is_closed_);
    STORE_FLAG(poll->is_anonymous_);
    STORE_FLAG(poll->allow_multiple_answers_);
    STORE_FLAG(poll->is_quiz_);
    STORE_FLAG(has_open_period);
    STORE_FLAG(has_close_date);
    STORE_FLAG(has_explanation);
    END_STORE_FLAGS();
    store(poll->question_, storer);
    vector<string> options = transform(poll->options_, [](const PollOption &option) { return option.text_; });
    store(options, storer);
    if (poll->is_quiz_) {
      store(poll->correct_option_id_, storer);
    }
    if (has_open_period) {
      store(poll->open_period_, storer);
    }
    if (has_close_date) {
      store(poll->close_date_, storer);
    }
    if (has_explanation) {
      store(poll->explanation_, storer);
    }
  }
}

template <class ParserT>
PollId PollManager::parse_poll(ParserT &parser) {
  int64 poll_id_int;
  td::parse(poll_id_int, parser);
  PollId poll_id(poll_id_int);
  if (is_local_poll_id(poll_id)) {
    string question;
    vector<string> options;
    FormattedText explanation;
    int32 open_period = 0;
    int32 close_date = 0;
    bool is_closed = false;
    bool is_anonymous = true;
    bool allow_multiple_answers = false;
    bool is_quiz = false;
    bool has_open_period = false;
    bool has_close_date = false;
    bool has_explanation = false;
    int32 correct_option_id = -1;

    if (parser.version() >= static_cast<int32>(Version::SupportPolls2_0)) {
      BEGIN_PARSE_FLAGS();
      PARSE_FLAG(is_closed);
      PARSE_FLAG(is_anonymous);
      PARSE_FLAG(allow_multiple_answers);
      PARSE_FLAG(is_quiz);
      PARSE_FLAG(has_open_period);
      PARSE_FLAG(has_close_date);
      PARSE_FLAG(has_explanation);
      END_PARSE_FLAGS();
    }
    parse(question, parser);
    parse(options, parser);
    if (is_quiz) {
      parse(correct_option_id, parser);
      if (correct_option_id < -1 || correct_option_id >= static_cast<int32>(options.size())) {
        parser.set_error("Wrong correct_option_id");
      }
    }
    if (has_open_period) {
      parse(open_period, parser);
    }
    if (has_close_date) {
      parse(close_date, parser);
    }
    if (has_explanation) {
      parse(explanation, parser);
    }
    if (parser.get_error() != nullptr) {
      return PollId();
    }
    return create_poll(std::move(question), std::move(options), is_anonymous, allow_multiple_answers, is_quiz,
                       correct_option_id, std::move(explanation), open_period, close_date, is_closed);
  }

  auto poll = get_poll_force(poll_id);
  if (poll == nullptr) {
    return PollId();
  }
  return poll_id;
}

}  // namespace td

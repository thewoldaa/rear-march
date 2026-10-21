// SPDX-License-Identifier: GPL-3.0-or-later
#include "test_common.h"

#include "core/action_mailbox.h"
#include "core/runtime_config.h"

using namespace rm;

TEST_CASE("the runtime store starts with a usable snapshot") {
    RuntimeStore store;
    const RuntimeConfig* config = store.current();
    REQUIRE(config != nullptr);
    CHECK(config->enabled);
    CHECK(config->turn.angle_deg == 180.0);
}

TEST_CASE("publishing replaces the live snapshot") {
    RuntimeStore store;
    RuntimeConfig first;
    first.turn.dpi = 8000;
    store.publish(first);

    CHECK(store.current()->turn.dpi == 8000);

    RuntimeConfig second;
    second.turn.dpi = 4000;
    store.publish(second);

    CHECK(store.current()->turn.dpi == 4000);
}

TEST_CASE("earlier snapshots stay valid after a publish") {
    // The hook thread may still hold a pointer to the previous snapshot when
    // the UI publishes a new one; retaining every snapshot is what makes that
    // safe without a reference count in the hot path.
    RuntimeStore store;
    RuntimeConfig first;
    first.turn.dpi = 12000;
    store.publish(first);
    const RuntimeConfig* held = store.current();

    for (int i = 0; i < 10; ++i) {
        RuntimeConfig next;
        next.turn.dpi = 1000 + i;
        store.publish(next);
    }

    CHECK(held->turn.dpi == 12000);
}

TEST_CASE("the mailbox counts every post and drains to zero") {
    ActionMailbox mailbox;
    mailbox.post(AppAction::Toggle);
    mailbox.post(AppAction::Toggle);
    mailbox.post(AppAction::CalibrateUp);

    CHECK(mailbox.take(AppAction::Toggle) == 2);
    CHECK(mailbox.take(AppAction::Toggle) == 0);
    CHECK(mailbox.take(AppAction::CalibrateUp) == 1);
    CHECK(mailbox.take(AppAction::CalibrateDown) == 0);
}

TEST_CASE("the mailbox clears every slot") {
    ActionMailbox mailbox;
    mailbox.post(AppAction::Toggle);
    mailbox.post(AppAction::CalibrateSave);
    mailbox.clear();

    CHECK(mailbox.take(AppAction::Toggle) == 0);
    CHECK(mailbox.take(AppAction::CalibrateSave) == 0);
}

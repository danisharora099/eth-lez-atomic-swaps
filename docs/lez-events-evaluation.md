# LP-0012 Structured Events — Atomic Swaps Evaluation

Evaluation of the LP-0012 structured event system
([lambda-prize PR #14](https://github.com/logos-co/lambda-prize/pull/14),
implementation in `bristinWild/logos-execution-zone`) against this atomic
swaps PoC, on the `test/lez-events` branch.

## Setup

The LP-0012 submission lives on a fork of `logos-execution-zone` whose `main`
is several hundred commits ahead of the LEZ revision this repo pins
(`9fa541f`), and it mixes in LP-0013 (token authority) work. To evaluate the
event system in isolation, the 11 LP-0012 commits were backported onto
`9fa541f`:

- Branch: [`danisharora099/logos-execution-zone#lp0012-events-9fa541f`](https://github.com/danisharora099/logos-execution-zone/tree/lp0012-events-9fa541f)
- Squashed backport of `bristinWild:backup/lp0012-before-rebase` (11 commits,
  base `085ca69e`) onto `9fa541f`, resolving conflicts with upstream PR #404
  (block/timestamp validity-window split). All program artifacts rebuilt
  against the new `ProgramOutput` journal layout; LEZ test suites pass
  (`nssa` 176, `common` 53, `sequencer_core`, `lez-events`).

This repo then:

- pins the LEZ Cargo deps and `scaffold.toml` `[repos.lez]` to that branch,
- emits `HtlcLocked` / `HtlcClaimed` / `HtlcRefunded` events from the HTLC
  guest program (`programs/lez-htlc/methods/guest/`),
- adds `LezClient::wait_for_receipt` / `decode_htlc_events` /
  `confirm_htlc_tx` on the host, polling the new `getTransactionReceipt` RPC,
- confirms lock/claim/refund transactions via receipts instead of (for lock)
  re-polling account state,
- adds receipt/event integration tests in `tests/lez_integration.rs`
  (`test_receipt_events_for_lock_and_claim`, `test_receipt_events_for_refund`,
  `test_rejected_claim_surfaces_receipt_error`).

## What worked

- **Guest-side API is ergonomic.** `lez_events::emit_event(discriminant,
  &BorshStruct)` plus a flush on output write was a ~30 line change to the
  HTLC program. Defining event types next to the instruction types means the
  host can reuse the same crate for decoding.
- **Receipts close a real gap.** Previously `claim()`/`refund()` were
  fire-and-forget and `lock()` had to re-poll account state to know the tx
  landed. `getTransactionReceipt` gives a definitive Included/Rejected status
  plus the emitted events in one call, including the revealed preimage in
  `HtlcClaimed` — exactly what a swap counterparty needs.
- **Rejection feedback.** A wrong-preimage claim now surfaces as a rejected
  receipt with the execution error string, instead of silently never changing
  state.
- **Program attribution.** Events come back tagged with the emitting
  `program_id`, so multi-program transactions (e.g. HTLC + transfer) can be
  filtered cleanly.

## Gaps found

1. **Receipts are per-tx-hash only — no subscription or filtering.** The
   maker discovers the taker's claim (the preimage reveal) without knowing
   the claim tx hash. `getTransactionReceipt` cannot help there, so the
   account-state polling watcher (`src/lez/watcher.rs`) had to stay. An
   indexer-style API (events by program id / account / block range) or a
   subscription would let events fully replace state polling. This is the
   single biggest limitation for the atomic-swap use case.
2. **Failure-path events don't work under `risc0_dev_mode`.** The
   `FAILURE_SENTINEL` journal trick requires real proving; the dev-mode
   executor discards the journal on guest panic (documented in the
   submission). Localnet runs dev mode, so rejected receipts carry an error
   string but no events. Fine for the PoC, untested in anger.
3. **Event flush is tied to `write_nssa_outputs`, but upstream moved to a
   builder API.** At `9fa541f` programs use `ProgramOutput::new(..).write()`;
   the LP-0012 free functions no longer exist upstream. The backport patches
   `write()` to drain the buffer, but the upstream PR will need to decide
   where flushing officially lives.
4. **Global sequence counter is process-wide.** `lez-events` tests need
   `--test-threads=1`; sequence numbers are only meaningful per-transaction,
   so a per-execution reset (or host-assigned ordering) would be cleaner.
5. **Fork drift.** The submission predates ~400 commits of upstream movement
   (validity windows, error variants, block timestamps). The backport
   conflicts were modest but real; an upstream PR should be cut against
   current HEAD soon, before drift grows.

## Recommendation

The event system does what LP-0012 promised and measurably improved this
PoC's LEZ client. For it to be the "events in LEZ" answer:

- upstream `lez-events` + sequencer receipt support into
  `logos-blockchain/logos-execution-zone` (or `lez-programs`) ahead of the
  LP-0013 token PR, which builds on it;
- add an indexer/subscription query (events by program/account) so consumers
  that don't know tx hashes — like a swap counterparty — can use events;
- decide the canonical flush point in the current builder-style
  `ProgramOutput` API.

## Reproducing

```bash
git checkout test/lez-events
make setup          # fetches the events-enabled LEZ pin
make localnet-start
NSSA_WALLET_HOME_DIR=.scaffold/wallet cargo test --test lez_integration -- \
    --ignored --test-threads=1 test_receipt test_rejected_claim
make localnet-stop
make demo-makefile  # full headless swap on the events-enabled stack
```

Notes:

- Run the localnet integration tests with `--test-threads=1`; concurrent
  `setup()` calls race on wallet topups against a single localnet.
- `make demo` / `make test` (the `lgs run` profiles) failed here with an
  unrelated scaffold-version mismatch ("`build idl` is only supported for
  `lez-framework` projects"); the `make demo-makefile` fallback works.

## Verification record (2026-06-11, this machine)

- LEZ branch: `nssa` 176, `common` 53, `sequencer_core` 4, `lez-events` 7
  tests pass; artifacts rebuilt via `cargo risczero build`.
- This repo: `cargo clippy --all-targets --all-features -- -D warnings`
  clean; 19 `lez_htlc_program` unit tests pass.
- Localnet: `test_receipt_events_for_lock_and_claim`,
  `test_receipt_events_for_refund`, `test_rejected_claim_surfaces_receipt_error`
  pass, plus pre-existing `test_lock_then_claim`,
  `test_watcher_detects_lock_and_claim`, `test_claim_wrong_preimage_fails`
  (no regressions).
- `make demo-makefile`: full ETH↔LEZ swap completed, maker and taker both
  finished with the preimage revealed and claimed on both chains.

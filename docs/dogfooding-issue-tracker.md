# Delivery dogfooding — issue tracker

Index of the upstream issues filed off the back of the
`logos-delivery-module` v0.1.1 integration into this repo. See
[`dogfooding-issue-triage.md`](./dogfooding-issue-triage.md) for the
per-finding rationale and the original narrative log in
[`../delivery-dogfooding.md`](../delivery-dogfooding.md).

All issues filed 2026-05-19; drafts have been deleted now that the
upstream issue is the source of truth. Reproducible evidence is kept
under [`dogfooding-evidence/2026-05-19/`](./dogfooding-evidence/2026-05-19/).

## Filed

| ID  | Repo                              | Filed URL                                                          | Evidence | Notes |
|-----|-----------------------------------|--------------------------------------------------------------------|----------|-------|
| A4  | `logos-co/logos-delivery-module`  | https://github.com/logos-co/logos-delivery-module/issues/36        | —        | Peer-count / structured `getStatus()` API request |
| A6  | `logos-co/logos-delivery-module`  | https://github.com/logos-co/logos-delivery-module/issues/34        | —        | `getSubscriptions()` / `subscribeIfNot`. Cross-linked to A7 (#35) |
| A7  | `logos-co/logos-delivery-module`  | https://github.com/logos-co/logos-delivery-module/issues/35        | —        | Document idempotency contract for `subscribe`/`unsubscribe`. Filed separately from A6 (#34); doc-side coverage in DOC (#270) |
| B1  | `logos-co/logos-basecamp`         | https://github.com/logos-co/logos-basecamp/issues/190              | [`dogfooding-evidence/2026-05-19/B1/`](./dogfooding-evidence/2026-05-19/B1/) — real capture + synthetic `fetchBalances` example | Filed with synthetic example + note that real `fetchBalances(configJson)` capture is available on request |
| B2a | `logos-co/logos-package-manager`  | https://github.com/logos-co/logos-package-manager/issues/12        | [`dogfooding-evidence/2026-05-19/B2/`](./dogfooding-evidence/2026-05-19/B2/) — `lgpm install` rejection + matching `#lgx` counterexample | Variant mismatch — `lgpm` half. Cross-linked to B2b (logos-basecamp#191). **Filed without `from eco dev` label** — lack permission to create labels on this repo |
| B2b | `logos-co/logos-basecamp`         | https://github.com/logos-co/logos-basecamp/issues/191              | [`dogfooding-evidence/2026-05-19/B2/`](./dogfooding-evidence/2026-05-19/B2/) — `bin-macos-app` boot log with wrong-variant install showing silent drop | Variant mismatch — PackageManagerLib silent-drop half. Cross-linked to B2a (logos-package-manager#12) |
| B5  | `logos-co/logos-delivery-module`  | https://github.com/logos-co/logos-delivery-module/issues/37        | [`dogfooding-evidence/2026-05-19/B5/`](./dogfooding-evidence/2026-05-19/B5/) — per-derivation `narinfo` HEAD probes for aarch64-darwin, x86_64-linux, x86_64-darwin (cross systems via `nix derivation show --system`, pure eval). Heavy tier 404 on Logos Cachix everywhere; x86_64-darwin widens to all 12 probed paths 404 | Cold-cache coverage. Optional fresh cold-build wall-clock still TBD. Targets logos-co/logos-delivery-module (packaging/CI) |
| DOC | `logos-co/logos-docs`             | https://github.com/logos-co/logos-docs/issues/270                  | —        | Consolidated journey-doc issue covering A5, A7, B3, B4, C1, C2, C3. Filed as an issue (not a PR) because we don't have the doc patch written; a PR can follow once one of us writes it. Cross-links A7 (delivery#35), A6 (delivery#34), A4 (delivery#36), B1 (basecamp#190), B2a (package-manager#12), B2b (basecamp#191) |

Every filed issue carries the `from eco dev` label except B2a — see
its row for the reason.

## Watched / not filing

| ID | Source | Disposition |
|----|--------|-------------|
| A1 | `logos-delivery-module#18` | Already filed upstream; doc-packet links it. Optional +1 comment |
| A2 | `logos-delivery-module#26` | Already filed upstream; doc-packet links it |
| A3 | `logos-delivery-module#27` | Not reproduced from this repo |
| D1 | this repo | Waku removal — guard against regression |
| D2 | this repo | Maker backoff — guard against regression |
| D3 | this repo | SwapTheme — guard against regression |
| D4 | this repo | `swap-module/flake.lock` tracked — guard against regression |
| D5 | this repo | Two-Basecamp launcher — guard against regression |
| D6 | this repo | Hashlock-vs-topic check — guard against regression |

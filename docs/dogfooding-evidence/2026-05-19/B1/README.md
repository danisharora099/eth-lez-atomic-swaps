# B1 evidence — capture method

The capture in [`proxy-log-excerpt.txt`](./proxy-log-excerpt.txt) was taken
on 2026-05-19 from a single boot of the `bin-macos-app` LogosBasecamp
configured by [`scripts/basecamp-instance.sh`](//scripts/basecamp-instance.sh),
with the launcher's hardcoded
`QT_LOGGING_RULES="*.debug=false;qt.qpa.*=false"` (line 289) **bypassed**.
The launcher's filter happens to suppress the `qt.remoteobjects: Send`
trace, but the `[info] ... ModuleProxy: callRemoteMethod ... args: ...`
lines are spdlog INFO output and are not gated by `QT_LOGGING_RULES` —
they appear in basecamp.log under the launcher's default settings too.
The bypass was only needed because the launcher uses `env -i`, which
prevented overriding `QT_LOGGING_RULES` from a wrapper environment
without editing the script.

## Exact commands

```bash
# 1. Build current LGX-portable artefacts and install them into the
#    isolated maker user-dir. Same as `make basecamp-init-maker`.
make swap-lgx-build
scripts/basecamp-instance.sh init maker

# 2. Resolve the bundled Basecamp app store path.
nix build --no-link --print-out-paths \
  github:logos-co/logos-basecamp#bin-macos-app
# -> /nix/store/g3i25...-LogosBasecamp-macos-app-0.0.0-dev

# 3. Boot the maker instance directly (skipping the launcher's env -i so
#    QT_LOGGING_RULES can be set explicitly without editing the script).
#    Same XDG_RUNTIME_DIR / TMPDIR shortening rule (macOS sun_path budget)
#    as basecamp-instance.sh. Same --user-dir, HOME, XDG_* paths.
REPO=/Users/danisharora099/Developer/status/eth-lez-atomic-swaps
BIN=/nix/store/g3i25.../LogosBasecamp.app/Contents/MacOS/LogosBasecamp
rm -rf /tmp/lbc-maker && mkdir -p /tmp/lbc-maker && chmod 700 /tmp/lbc-maker
env -i \
  PATH="/usr/bin:/bin:/usr/sbin:/sbin:$HOME/.nix-profile/bin" \
  HOME="$REPO/.basecamp/maker/home" \
  XDG_CONFIG_HOME="$REPO/.basecamp/maker/xdg/config" \
  XDG_CACHE_HOME="$REPO/.basecamp/maker/xdg/cache" \
  XDG_DATA_HOME="$REPO/.basecamp/maker/xdg/data" \
  XDG_RUNTIME_DIR=/tmp/lbc-maker TMPDIR=/tmp/lbc-maker \
  NSSA_WALLET_HOME_DIR="$REPO/.basecamp/maker/wallet" \
  SWAP_UI_AUTO_ENV_FILE="$REPO/.env" SWAP_UI_AUTO_ROLE=maker \
  QT_LOGGING_RULES="qt.qpa.*=false" \
  "$BIN" --user-dir "$REPO/.basecamp/maker/data" \
  > /tmp/b1-proxy-on.log 2>&1 &

# 4. Let auto-init run for ~30s (loadEnv, messagingInit, fetchBalancesFromEnv,
#    requestModule traffic across the swap/swap_ui/delivery_module/capability
#    boundaries), then stop.
sleep 30 && pkill LogosBasecamp

# 5. Inspect.
grep -n 'ModuleProxy: callRemoteMethod' /tmp/b1-proxy-on.log | head
#    61 ModuleProxy lines were produced during one ~30s boot, including
#    `messagingInit`, `loadEnv`, `fetchBalancesFromEnv`, `messagingStatus`,
#    `requestModule`, `subscribe`, `resolveFlatDependencies`,
#    `setUserModulesDirectory`, ...
```

## What is and isn't in this evidence

- **Section 1 (real)** — three swap-module lines (`messagingInit`,
  `loadEnv`, `fetchBalancesFromEnv`) plus three capability-module lines,
  copied verbatim from `/tmp/b1-proxy-on.log` and then redacted. They
  prove the leak mechanism: any string-typed `Q_INVOKABLE` arg is
  written to the spdlog INFO stream verbatim, with no per-method or
  per-key filtering. The same shape applies to every cross-module call
  on every module — `delivery_module.subscribe(topic)`,
  `capability_module.requestModule(consumer, target)`, etc.
- **Section 2 (synthetic)** — a single hand-constructed `fetchBalances`
  line built from the live `.env` schema. The values are placeholder
  patterns (`<REDACTED_*>`), not taken from any real config. This
  demonstrates what the leak looks like for the pre-mitigation
  `fetchBalances(configJson)` entry point that the dogfooding finding
  originally observed. A fresh real capture of this exact line would
  require either reverting this repo's local mitigation
  (`fetchBalancesFromEnv` → `fetchBalances(configJson)` in
  [`swap-ui/src/swap_ui_plugin.cpp`](//swap-ui/src/swap_ui_plugin.cpp#L716-L770))
  or driving the legacy "Fetch Balances" GUI button (still wired at
  [`swap-ui/src/swap_ui_plugin.cpp#L799-L818`](//swap-ui/src/swap_ui_plugin.cpp#L799-L818))
  with config that contains real secrets. Neither was performed here.

## Redaction discipline

Every captured line was sanitised with stable, structurally faithful
placeholders so the proxy log shape stays inspectable:

| Placeholder                          | Replaces                                          |
|--------------------------------------|---------------------------------------------------|
| `<REDACTED_REPO_ROOT>`               | absolute path of this repo on the captured host    |
| `<REDACTED_CAPABILITY_TOKEN_*>`      | per-module UUID tokens minted by capability_module |
| `<REDACTED_ETH_PRIVATE_KEY>`         | 64-hex-char ETH signing key                        |
| `<REDACTED_LEZ_SIGNING_KEY>`         | LEZ signing key                                    |
| `<REDACTED_RPC_URL>`                 | ETH / LEZ JSON-RPC URL                             |
| `<REDACTED_ADDRESS>`                 | ETH / LEZ account identifier                       |

`/Users/danisharora099/...` and capability-token UUIDs that appear in
the raw `/tmp/b1-proxy-on.log` are personally identifying or
session-sensitive even though the dev `.env` happens to use anvil's
public test key (`ac0974be...80`), and were redacted accordingly.

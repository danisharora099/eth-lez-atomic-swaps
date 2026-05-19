# B2 evidence — capture method

Two transcripts in this directory:

- [`transcript-b2a-lgpm.txt`](./transcript-b2a-lgpm.txt) — host `lgpm install`
  rejecting a `#lgx-portable` archive, plus the matching counterexample showing
  `lgpm install` succeeds on the default `#lgx` (`-dev`-suffixed) archive from
  the *same* upstream module. Demonstrates that `lgpm` 1.0.0 from
  `logos-co/logos-package-manager#cli` (built without `LGPM_PORTABLE_BUILD`)
  refuses any LGX whose `main` keys do not contain `<host>-dev`.
- [`transcript-b2b-basecamp.txt`](./transcript-b2b-basecamp.txt) — boot log
  excerpt from `bin-macos-app` LogosBasecamp pointed at a throwaway user-dir
  with the wrong-variant (`darwin-arm64-dev`) install of the *same* upstream
  module. Demonstrates that PackageManagerLib enumerates the manifest, finds
  no variant it can resolve (because it is built with `LGPM_PORTABLE_BUILD`
  and only tries the bare host string `darwin-arm64`), and silently drops the
  module from the load-eligible set without logging a single
  `[info]`/`[warn]`/`[error]` line that mentions the variant mismatch.

Both reproductions are non-destructive — they run entirely against
`/tmp/b2-*` paths and do not touch this repo's `.basecamp/maker` /
`.basecamp/taker` user-dirs, `scripts/basecamp-instance.sh`, or `Makefile`.
The local mitigation shipped in this repo (`#lgx-portable` + manual
`extract_lgx_variant` helper, see
[`scripts/basecamp-instance.sh#L113-L178`](//scripts/basecamp-instance.sh#L113-L178))
is unchanged.

## Exact commands

```bash
# 1. Build both LGX flavours of the upstream module so the same artefact
#    feeds both transcripts.
DEV_LGX=$(nix build --no-link --print-out-paths \
  'github:logos-co/logos-delivery-module/v0.1.1#lgx')/logos-delivery_module-module-lib.lgx
PORTABLE_LGX=$(nix build --no-link --print-out-paths \
  'github:logos-co/logos-delivery-module/v0.1.1#lgx-portable')/logos-delivery_module-module-lib.lgx

# 2. Build the host lgpm (non-portable, as a downstream consumer would have it).
LGPM=$(nix build --no-link --print-out-paths \
  'github:logos-co/logos-package-manager#cli')/bin/lgpm

# 3. B2a: prove `lgpm install` rejects the portable LGX.
T1=$(mktemp -d -t b2-portable.XXXXXX)
"$LGPM" --modules-dir "$T1/modules" --ui-plugins-dir "$T1/plugins" \
  install --file "$PORTABLE_LGX" --allow-unsigned
# -> FAILED, "Package does not contain variant for platform: darwin-arm64-dev"

# 3a. Counterexample: same lgpm, default `#lgx` archive (variant = -dev).
T2=$(mktemp -d -t b2-dev.XXXXXX)
"$LGPM" --modules-dir "$T2/modules" --ui-plugins-dir "$T2/plugins" \
  install --file "$DEV_LGX" --allow-unsigned
# -> done; creates $T2/modules/delivery_module/ with variant = darwin-arm64-dev

# 4. B2b: install the WRONG (#lgx, -dev) variant by hand into a throwaway
#    user-dir, then boot bin-macos-app pointed at it.
rm -rf /tmp/b2-bad-userdir /tmp/b2-bad-home /tmp/b2-bad-xdg /tmp/lbc-b2bad
mkdir -p /tmp/b2-bad-userdir/modules/delivery_module \
         /tmp/b2-bad-userdir/plugins \
         /tmp/b2-bad-home /tmp/b2-bad-xdg/{config,cache,data} \
         /tmp/lbc-b2bad
chmod 700 /tmp/lbc-b2bad
TMP=$(mktemp -d) && tar -xzf "$DEV_LGX" -C "$TMP"
cp "$TMP/manifest.json" /tmp/b2-bad-userdir/modules/delivery_module/manifest.json
cp -R "$TMP/variants/darwin-arm64-dev/." /tmp/b2-bad-userdir/modules/delivery_module/
printf '%s' 'darwin-arm64-dev' > /tmp/b2-bad-userdir/modules/delivery_module/variant

BIN=$(nix build --no-link --print-out-paths \
  'github:logos-co/logos-basecamp#bin-macos-app')/LogosBasecamp.app/Contents/MacOS/LogosBasecamp

env -i \
  PATH="/usr/bin:/bin:/usr/sbin:/sbin:$HOME/.nix-profile/bin" \
  HOME=/tmp/b2-bad-home \
  XDG_CONFIG_HOME=/tmp/b2-bad-xdg/config \
  XDG_CACHE_HOME=/tmp/b2-bad-xdg/cache \
  XDG_DATA_HOME=/tmp/b2-bad-xdg/data \
  XDG_RUNTIME_DIR=/tmp/lbc-b2bad TMPDIR=/tmp/lbc-b2bad \
  QT_LOGGING_RULES="qt.qpa.*=false" \
  "$BIN" --user-dir /tmp/b2-bad-userdir \
  > /tmp/b2-bad.log 2>&1 &
sleep 18 && pkill LogosBasecamp

# 5. Inspect.
grep -nE 'Module loaded:|Currently loaded modules:|resolveFlatDependencies' /tmp/b2-bad.log
grep -niE 'variant|platform|main file|mainFilePath|known modules|unsupported|skipping|incompatible|not loadable|error|warn' /tmp/b2-bad.log | grep -i delivery
#  -> no output for the second grep — the silent-drop signal.
```

The same `env -i` direct-launch trick is documented in
[`docs/dogfooding-evidence/2026-05-19/B1/README.md`](/docs/dogfooding-evidence/2026-05-19/B1/README.md);
it is used here for the same reason (the launcher's hardcoded
`QT_LOGGING_RULES="*.debug=false;qt.qpa.*=false"` at
[`scripts/basecamp-instance.sh#L289`](//scripts/basecamp-instance.sh#L289)
cannot be overridden through wrapper environment because the launcher uses
`env -i`).

## What is and is not in this evidence

- **B2a transcript (real)** — every line is from a live `lgpm` invocation in
  this session. The "Package does not contain variant for platform:
  darwin-arm64-dev" string is verbatim. Both runs used the same upstream LGX
  archives so the only difference under test is the `main.<variant>` key /
  `variants/<variant>/` directory inside the manifest.
- **B2b transcript (real)** — boot banner and the `Module loaded:` /
  `Currently loaded modules:` / `resolveFlatDependencies` lines are verbatim
  from `/tmp/b2-bad.log`. The "what is *not* in the log" §4 was verified by
  running the exact `grep` shown in the transcript over all 350 captured
  lines: zero hits.
- **B2b corroboration that is NOT directly captured here** — the downstream
  `[warning] Module not found in known modules: <name>` /
  `Failed to load core dependency "<name>" for "<consumer>"` line. That
  symptom requires a UI plugin to actually request the dropped module via
  `package_manager.loadModule`, which only happens when a user opens the
  relevant tab in the Basecamp GUI. It was captured during this repo's M2
  cross-node proof against `bin-macos-app` before the local mitigation, and
  is cited verbatim in
  [`scripts/basecamp-instance.sh#L215-L218`](//scripts/basecamp-instance.sh#L215-L218)
  and [`delivery-dogfooding.md#L460-L464`](//delivery-dogfooding.md#L460-L464).
  The transcript points the reader at those citations rather than re-capturing
  a GUI-driven run.

## Redaction discipline

| Placeholder              | Replaces                                                 |
|--------------------------|----------------------------------------------------------|
| `<HASH>`                 | leading 32-char Nix store hash (kept the suffix readable) |
| `<INSTANCE_SUFFIX>`      | the per-process suffix on Qt RemoteObjects registry URLs (`local:logos_<svc>_<suffix>`) |
| `<REDACTED_CAPABILITY_TOKEN>` | per-module UUID tokens minted by `capability_module`     |
| `<REDACTED_REPO_ROOT>`   | absolute path of this repo (would only appear if the user-dir were inside the repo; this throwaway run uses `/tmp/b2-bad-userdir` so the placeholder does not appear in the transcript) |

The throwaway user-dir layout (`/tmp/b2-*`) and `/nix/store/...` paths are not
personally identifying, but Nix store hashes change between builds and would
mislead a maintainer trying to reproduce, so they are anonymised. The
upstream commit SHAs in the boot banner (logos-basecamp `57d65eb1`,
logos-package-manager `9101875b`, etc.) are kept verbatim — they identify
the build under test and are public.

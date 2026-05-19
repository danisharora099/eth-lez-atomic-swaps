# B5 evidence — capture method

Transcripts in this directory:

- [`nix-config.txt`](./nix-config.txt) — host Nix version, system, and
  the `substituters` / `trusted-public-keys` actually configured at the
  time of capture. Establishes that `https://logos-co.cachix.org` is
  configured and trusted, so any 404 reported below is a genuine cache
  miss and not a configuration gap.
- [`cachix-probe.txt`](./cachix-probe.txt) — per-derivation HTTP HEAD
  probe of `.narinfo` against both substituters
  (`logos-co.cachix.org` and `cache.nixos.org`) for the heavy
  derivations the original dogfooding finding called out, plus the
  `logos-cpp-sdk-*` siblings for contrast, on **aarch64-darwin**
  (host system; everything below was evaluated from this host). This
  is the primary evidence. Reports 200 (substituter has it; fresh
  consumer would fetch) vs 404 (substituter does not have it; fresh
  consumer would build locally).
- [`cachix-probe-x86_64-linux.txt`](./cachix-probe-x86_64-linux.txt) —
  same probe shape against the **x86_64-linux** evaluation of the
  same locked flake revision (`nix derivation show ... --system
  x86_64-linux` from the aarch64-darwin host; pure evaluation, no
  build).
- [`cachix-probe-x86_64-darwin.txt`](./cachix-probe-x86_64-darwin.txt)
  — same probe shape against the **x86_64-darwin** evaluation
  (`--system x86_64-darwin` from the aarch64-darwin host; pure
  evaluation, no build).
- [`nix-build-dryrun.txt`](./nix-build-dryrun.txt) — `nix build
  --dry-run` against both `#lgx-portable` and `#lgx` outputs. Empty
  output, with the reason: this host already built these flake outputs
  during prior B2 evidence sessions, so `--dry-run` reports nothing to
  do. Included to document that `--dry-run` is not a substitute for
  the narinfo probe when the local store is already warm.

The capture is fully read-only: no `nix build` (without
`--dry-run`) was issued, no module was installed into the repo's
`.basecamp/maker` / `.basecamp/taker` user-dirs, no production file
(`scripts/basecamp-instance.sh`, `Makefile`, `flake.nix`, …) was
modified.

## Exact commands

```bash
# 1. Confirm Nix can see the Logos Cachix substituter.
nix --version
uname -sm
nix show-config | grep -E '^(substituters|trusted-public-keys|trusted-substituters|extra-substituters)'

# 2. Pin the upstream flake under test and resolve the LGX .drv outputs.
#    For the host system (aarch64-darwin) `--system` can be omitted.
#    For the cross-platform probes (x86_64-linux, x86_64-darwin) pass
#    `--system <system>` to force pure evaluation for that system. No
#    build is triggered for any system — only .drv resolution.
nix flake metadata 'github:logos-co/logos-delivery-module/v0.1.1'
for sys in aarch64-darwin x86_64-linux x86_64-darwin; do
  nix path-info --derivation 'github:logos-co/logos-delivery-module/v0.1.1#lgx-portable' --system "$sys"
  nix path-info --derivation 'github:logos-co/logos-delivery-module/v0.1.1#lgx'          --system "$sys"
done

# 3. Enumerate the closure once the .drvs are in the local store and
#    filter to the dogfooding-relevant names (zerokit*, liblogosdelivery*,
#    logos-delivery_module-module-lib*, plus logos-cpp-sdk-* / logos-module
#    as 200/404 contrast).
nix-store --query --requisites \
  /nix/store/<HASH>-logos-delivery_module-module-lib-lgx-1.1.0.drv \
  | grep -iE 'zerokit|liblogosdelivery|logos-delivery_module|logos-cpp|logos-module|module-lib'

# 4. For each .drv pulled by step 3, resolve its `out` store path.
#    Non-FOD case (most paths):
nix derivation show /nix/store/<HASH>-<name>.drv \
  | jq -r '.derivations | to_entries[0].value.outputs.out.path'
# FOD case (`zerokit-0.9.0-vendor-staging` — `outputHashMode = "recursive"`):
# `nix derivation show` reports only the output hash, not the path.
# Resolve the path with nix-store --query --outputs instead:
nix-store --query --outputs /nix/store/<HASH>-zerokit-0.9.0-vendor-staging.drv

# 5. Probe each output's narinfo against both substituters with a HEAD.
#    HTTP 200 = substituter serves it; HTTP 404 = fresh consumer builds locally.
for hash in <HASHES_FROM_STEP_4>; do
  for sub in logos-co.cachix.org cache.nixos.org; do
    code=$(curl -sI -o /dev/null -w '%{http_code}' "https://${sub}/${hash}.narinfo")
    printf '%s %-50s %s\n' "$code" "<name-for-$hash>" "$sub"
  done
done

# 6. Sanity cross-check with `nix path-info --store` against the same
#    substituter — Nix itself returns the same answer the HEAD probe does.
nix path-info --store https://logos-co.cachix.org \
  /nix/store/<HASH>-logos-delivery_module-module-lib-lgx-1.1.0

# 7. Document the dry-run that *does not* surface the gap on this host
#    (local store already warm; included so the reader knows why).
nix build --no-link 'github:logos-co/logos-delivery-module/v0.1.1#lgx-portable' --dry-run
nix build --no-link 'github:logos-co/logos-delivery-module/v0.1.1#lgx'          --dry-run
```

## What is and isn't in this evidence

- **`cachix-probe.txt` (real)** — every status code is from a live HTTP
  HEAD against the named substituter. The unredacted store-path
  hashes (so the upstream maintainer can rerun the same probe
  verbatim) are:

  | Hash                                | Name                                         |
  |-------------------------------------|----------------------------------------------|
  | `lv5fw3xbiay58dasr6lcmhyxpmrbvi6n`  | `zerokit-0.9.0-vendor-staging`               |
  | `5065vqgba9m7sbrcsn58s956q47sszcq`  | `zerokit-0.9.0-vendor`                       |
  | `7gd40kd270zrcgawj1dvinygavcbgi33`  | `zerokit-0.9.0`                              |
  | `masfdwjrqndax1lk2rznxzdjz5s4rckb`  | `liblogosdelivery-dev`                       |
  | `fchkhx30rpwnc1jmdql7mpr6f9b8ybaz`  | `logos-module-0.1.0`                         |
  | `anas8svynmkc8qg8bnqi3nz4c2yx2srk`  | `logos-delivery_module-module-lib-1.1.0`     |
  | `9883902cbd7ryxx6zpmsxs9vf6dwbb1l`  | `logos-delivery_module-module-lib-bundle-1.1.0` |
  | `cnd54gravalvijjrrplr3j6c1q1235vi`  | `logos-delivery_module-module-lib-lgx-1.1.0` (`#lgx-portable`) |
  | `q5g7d1m54bqs3835r4fha4cvbqncpawq`  | `logos-delivery_module-module-lib-lgx-1.1.0` (`#lgx`, `-dev`) |
  | `h4p5dj3ndim6sjscpggxkn6ya1wmjnrg`  | `logos-cpp-sdk-lib-0.1.0`                    |
  | `ymfvx1znkqaxlxihxzz409mqlvwnjx6r`  | `logos-cpp-sdk-generator-0.1.0`              |
  | `1syckjfgx3qrjd7winaz7hwpd1q04aa6`  | `logos-cpp-sdk-headers-0.1.0`                |

  Hashes are deterministic from the flake's locked inputs at
  `0c346c0c` on `aarch64-darwin`; the upstream maintainer reproducing
  on the same flake revision and system should compute the exact same
  set.
- **`nix-config.txt` (real)** — verbatim `nix show-config` lines from
  the captured host. Substituter URLs and trusted-public-keys are
  public; kept verbatim.
- **`nix-build-dryrun.txt` (real)** — both invocations produced no
  output. Captured to make it explicit that `--dry-run` *cannot* be
  used as the headline probe on this host, because the local store is
  already warm from B2 work.
- **`cachix-probe-x86_64-linux.txt` (real)** — same probe shape as
  the aarch64-darwin file, evaluated for `--system x86_64-linux` from
  the aarch64-darwin host (pure evaluation, no build). The
  unredacted x86_64-linux hashes are:

  | Hash                                | Name                                         |
  |-------------------------------------|----------------------------------------------|
  | `lv5fw3xbiay58dasr6lcmhyxpmrbvi6n`  | `zerokit-0.9.0-vendor-staging` (FOD; same hash across systems) |
  | `hr3z7536qxypidc49jrxlpvcbxv6ysll`  | `zerokit-0.9.0-vendor`                       |
  | `d190iwlz9amcq875d2lhqxzi2dkyvq33`  | `zerokit-0.9.0`                              |
  | `zcyg9hb30gpqpvmiijbpyp4wih1skza0`  | `liblogosdelivery-dev`                       |
  | `yhkkq2cg7za2xhyl3rb58fc9lj824pwj`  | `logos-module-0.1.0`                         |
  | `qncb3abs2g0vqs9ram8q97jyzlcrdl3g`  | `logos-delivery_module-module-lib-1.1.0`     |
  | `rg4sqq4fzp6mlcmggp60x6fwz63njkdd`  | `logos-delivery_module-module-lib-bundle-1.1.0` |
  | `lnmn944jfcg5jiy6lqyrry4db8knnhqb`  | `logos-delivery_module-module-lib-lgx-1.1.0` (`#lgx-portable`) |
  | `jm44z54w6mx234p5468b8j3q9qlyaihr`  | `logos-delivery_module-module-lib-lgx-1.1.0` (`#lgx`, `-dev`) |
  | `ic7dkz1y5bzp6ysp4fmzqqy578282dg8`  | `logos-cpp-sdk-lib-0.1.0`                    |
  | `6sx0c8lccj5bax7b3ak4nzfhd6cswvsr`  | `logos-cpp-sdk-generator-0.1.0`              |
  | `4lb3ydrwck5kbfsdiydpcc7nznsr7iby`  | `logos-cpp-sdk-headers-0.1.0`                |

- **`cachix-probe-x86_64-darwin.txt` (real)** — same shape, evaluated
  for `--system x86_64-darwin`. The unredacted x86_64-darwin hashes are:

  | Hash                                | Name                                         |
  |-------------------------------------|----------------------------------------------|
  | `lv5fw3xbiay58dasr6lcmhyxpmrbvi6n`  | `zerokit-0.9.0-vendor-staging` (FOD; same hash across systems) |
  | `a7bx3cnknrqxqcdarqly6pzvhv50kikp`  | `zerokit-0.9.0-vendor`                       |
  | `7klmx3xlnq1jl8k3rypj2kplj7salvbx`  | `zerokit-0.9.0`                              |
  | `jj656n5b94y17x9yf660r5kzbzldb8n6`  | `liblogosdelivery-dev`                       |
  | `vp45ikdbzxf2yzyn7r4hkr8vqmfa7syr`  | `logos-module-0.1.0`                         |
  | `46s14wcbrhvmmsd5rsagh23jvfpb4iaq`  | `logos-delivery_module-module-lib-1.1.0`     |
  | `1jk76pw0shjs2ay1nrr9fw7yhkjjzk0m`  | `logos-delivery_module-module-lib-bundle-1.1.0` |
  | `ghdcmsq68d65wm88ccf8hmj0bzkmcvb3`  | `logos-delivery_module-module-lib-lgx-1.1.0` (`#lgx-portable`) |
  | `zvr2w5rpw3aal0iwq1k1cimhknrxviz7`  | `logos-delivery_module-module-lib-lgx-1.1.0` (`#lgx`, `-dev`) |
  | `fmvxsryi4kq15gm2qs6w12h4mxqfcd8g`  | `logos-cpp-sdk-lib-0.1.0`                    |
  | `b4wb6iy68h1sv8i4qdg66qbv0cihgfpn`  | `logos-cpp-sdk-generator-0.1.0`              |
  | `d0s8rpxsbzn4k6srnjp43858rmiq5l4s`  | `logos-cpp-sdk-headers-0.1.0`                |

  Cross-system note: `zerokit-0.9.0-vendor-staging` resolves to the
  same FOD output hash on all three systems because it is
  `outputHashMode = "recursive"` with a declared `outputHash`. Every
  other path is system-specific. The cross-platform probes were
  evaluated from the aarch64-darwin host — `nix derivation show
  --system <other>` is pure evaluation and succeeds without any real
  build for the cross system.
- **Wall-clock for a real cold build (NOT recaptured here)** — the
  ~1h cold-build observation that originally triggered the finding is
  cited from `delivery-dogfooding.md` lines ~407-417 / 435-445. A
  fresh end-to-end re-time would require a clean Nix store; per the
  carve-out in the handoff, no production-affecting builds were
  issued. The narinfo probe is the structural evidence; the wall
  clock is the impact estimate.

## Redaction discipline

| Placeholder            | Replaces                                                         |
|------------------------|------------------------------------------------------------------|
| `<HASH>` in commands   | the leading 32-char Nix store hash for the path being discussed   |
| `<HASHES_FROM_STEP_4>` | the deterministic list of output hashes from `nix derivation show` |
| `<name-for-$hash>`     | human-friendly derivation name (preserved verbatim in the table)   |

- Substituter URLs (`https://cache.nixos.org`,
  `https://logos-co.cachix.org`) are public; verbatim.
- `trusted-public-keys` values are public; verbatim.
- Upstream commit `0c346c0c2ab2404c11a62cd6c385e806e8465434` identifies
  the build under test; verbatim.
- No `.env`, capability-token, repo-path, or wallet-key data is touched
  by any command above. Nothing personally identifying was captured.

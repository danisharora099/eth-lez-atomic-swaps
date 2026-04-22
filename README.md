# Atomic Swaps PoC

Cross-chain atomic swap between LEZ and Ethereum using hash time-locked contracts (HTLCs).

This repo includes:

- a headless local demo
- a CLI for maker, taker, status, and refund flows
- an optional `logos-app` plugin UI

## How the swap works

```text
Taker                                          Maker
  1. Generate preimage + hashlock
  2. Lock ETH with a longer timelock
  --------------------------------------------> sees ETH lock
                                     3. Verify ETH lock, then lock LEZ
                                        with a shorter timelock
  4. Claim LEZ, which reveals the preimage
                                     5. Read the preimage and claim ETH
```

If one side stops responding, the timelocks allow refunds.

## First-time quickstart

If you are new to this repo, follow this exact order:

1. Clone the repo with submodules.
2. Install the required toolchains.
3. Run `make setup`.
4. Run `make demo` for the fastest successful end-to-end swap.
5. Run `make infra` if you want to drive maker and taker yourself.

`make setup` must finish successfully before `make infra` or most other flows.

### Supported platforms

- Apple Silicon macOS (`arm64`)
- Linux `x86_64`
- Linux `aarch64`

Intel macOS is not supported because upstream does not publish a `logos-blockchain-circuits` bundle for `macos-x86_64`.

### Required for the CLI and local demo

- Rust 1.85+ via [rustup](https://rustup.rs/)
- [Foundry](https://book.getfoundry.sh/getting-started/installation) (`forge`, `anvil`)
- GNU `make`
- a C/C++ toolchain
- [`logos-scaffold`](https://github.com/logos-co/logos-scaffold) on your `PATH`
- the RISC Zero toolchain installed with `rzup install rust`

Notes:

- The first full build can take 5-10 minutes because it compiles `libwaku` and the LEZ guest artifacts.
- Docker or Podman is not required for the normal local flow.

### Optional for the `logos-app` UI

- CMake 3.21+
- Qt 6.5+
- [Nix](https://nixos.org/) for building `logos-app`

### 1. Clone

```bash
git clone --recurse-submodules https://github.com/logos-co/eth-lez-atomic-swaps.git
cd eth-lez-atomic-swaps
```

If you already cloned without submodules:

```bash
git submodule update --init --recursive
```

### 2. Install prerequisites

<details><summary><b>macOS (Apple Silicon)</b></summary>

```bash
xcode-select --install
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
curl -L https://foundry.paradigm.xyz | bash && foundryup
curl -L https://risczero.com/install | bash
rzup install rust
```

Install `logos-scaffold` from a local clone:

```bash
git clone https://github.com/logos-co/logos-scaffold.git
cd logos-scaffold
cargo install --path .
```

If you want the optional UI as well:

```bash
brew install cmake qt@6
```

If CMake cannot find Qt6:

```bash
export CMAKE_PREFIX_PATH="$(brew --prefix qt@6)"
```

The workspace [`.cargo/config.toml`](.cargo/config.toml) already contains the macOS `aarch64` linker flags needed by `libwaku`.

</details>

<details><summary><b>Linux</b></summary>

For Ubuntu or Debian:

```bash
sudo apt install build-essential make
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
curl -L https://foundry.paradigm.xyz | bash && foundryup
curl -L https://risczero.com/install | bash
rzup install rust
```

For Fedora:

```bash
sudo dnf install gcc gcc-c++ make
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
curl -L https://foundry.paradigm.xyz | bash && foundryup
curl -L https://risczero.com/install | bash
rzup install rust
```

Install `logos-scaffold` from a local clone:

```bash
git clone https://github.com/logos-co/logos-scaffold.git
cd logos-scaffold
cargo install --path .
```

If you want the optional UI as well:

```bash
# Ubuntu / Debian
sudo apt install cmake qt6-base-dev qt6-declarative-dev

# Fedora
sudo dnf install cmake qt6-qtbase-devel qt6-qtdeclarative-devel
```

Qt 6.5+ is required for the UI. Older distros may need [aqtinstall](https://github.com/miurahr/aqtinstall) or the [Qt online installer](https://www.qt.io/download-qt-installer).

</details>

### 3. Run one-time setup

From the repo root:

```bash
make setup
```

What this does:

- downloads `logos-blockchain-circuits` into `.scaffold/circuits`
- runs `logos-scaffold setup`
- creates the local LEZ checkout and wallet under `.scaffold/`

You do not need `logos-scaffold init`. This repo already ships a checked-in [`scaffold.toml`](scaffold.toml) with the expected relative paths.

If you want to inspect the generated LEZ wallet accounts:

```bash
logos-scaffold wallet list --long
```

### 4. Fastest smoke test: run the built-in demo

```bash
make demo
```

This is the quickest way to verify your machine is set up correctly. It:

- starts the local LEZ network if needed
- deploys the Ethereum HTLC to Anvil
- runs both maker and taker
- completes a full swap without the UI

### 5. Interactive local stack

If you want to run each side yourself, start the infrastructure first and leave it running:

```bash
make infra
```

`make infra` starts:

- Anvil
- the LEZ localnet
- an embedded Waku rendezvous node
- contract deployment
- `.env` and `.env.taker` generation

Use `Ctrl-C` in that terminal to stop everything cleanly.

Then open two more terminals in the repo root.

Maker:

```bash
cargo run --bin swap-cli -- --env-file .env maker
```

Taker:

```bash
cargo run --bin swap-cli -- --env-file .env.taker taker
```

After `cargo build --release`, you can replace `cargo run --bin swap-cli --` with `./target/release/swap-cli`.

## CLI reference

Common commands:

```bash
cargo run --bin swap-cli -- --env-file .env maker
cargo run --bin swap-cli -- --env-file .env.taker taker
cargo run --bin swap-cli -- --env-file .env status --swap-id <hex>
cargo run --bin swap-cli -- --env-file .env status --hashlock <hex>
cargo run --bin swap-cli -- --env-file .env refund eth --swap-id <hex>
cargo run --bin swap-cli -- --env-file .env refund lez --hashlock <hex>
```

If you are not using the local stack from `make infra`, start from [`.env.example`](.env.example) and provide your own RPC endpoints, keys, contract address, and LEZ account details.

## `logos-app` plugin (optional UI)

The UI runs inside [logos-app](https://github.com/logos-co/logos-app) as an IComponent plugin.

### First-time `logos-app` setup

```bash
git clone https://github.com/logos-co/logos-app.git
cd logos-app
nix build
```

That produces `result/bin/logos-app`.

The [`Makefile`](Makefile) assumes these defaults:

- `LOGOS_APP_INTERFACES=$(HOME)/Developer/status/logos-app/app/interfaces`
- `LOGOS_APP_BIN=$(HOME)/Developer/status/logos-app/result/bin/logos-app`

Override them if your checkout lives elsewhere:

```bash
make plugin-build LOGOS_APP_INTERFACES=<path-to-logos-app>/app/interfaces
make plugin-run-maker LOGOS_APP_BIN=<path-to-logos-app>/result/bin/logos-app
```

Plugin CMake uses Nix Qt paths hardcoded in the [`Makefile`](Makefile). If your Nix store hashes differ, rebuild `logos-app` and refresh those variables.

### Run the UI

After `make infra` is running:

```bash
make plugin-run-maker
make plugin-run-taker
```

This launches two `logos-app` instances, one as maker and one as taker.

On macOS, the plugin is installed under:

```text
~/Library/Application Support/Logos/LogosAppNix/plugins/lez_atomic_swap/
```

`make plugin-install` copies `lez_atomic_swap.dylib` and `libswap_ffi.dylib` into that plugin directory. The UI loads `.env` for maker and `.env.taker` for taker.

## Screenshots

| Config | Maker | Taker | Refund |
|--------|-------|-------|--------|
| ![Config](docs/config.png) | ![Maker](docs/maker.png) | ![Taker](docs/taker.png) | ![Refund](docs/refund.png) |

![logos-app plugin](docs/logos-app-plugin.gif)

## Project layout

| Path | Purpose |
|---|---|
| [`scaffold.toml`](scaffold.toml) | Local LEZ checkout, wallet, and localnet configuration |
| `contracts/` | Solidity HTLC contract built with Foundry |
| `programs/lez-htlc/` | LEZ HTLC program built with RISC Zero |
| `src/` | Orchestration, clients, messaging, maker/taker/refund CLI flows |
| `swap-ffi/` | C FFI bridge for the Qt UI |
| `logos-module/` | `logos-app` plugin |
| `tests/` | Integration tests |

## Common make targets

| Command | What it does |
|---|---|
| `make setup` | Download circuits and run `logos-scaffold setup` |
| `make infra` | Start local infra, deploy contracts, and write `.env` files |
| `make demo` | Run a full headless swap |
| `make test` | Build contracts, start localnet, run `cargo test`, stop localnet |
| `make contracts` | Run `forge build` inside `contracts/` |
| `make localnet-start` | Start the LEZ localnet |
| `make localnet-stop` | Stop the LEZ localnet |
| `make plugin-build` | Build the optional Qt plugin |
| `make plugin-run-maker` | Launch `logos-app` as maker |
| `make plugin-run-taker` | Launch `logos-app` as taker |

## Architecture

```text
+-----------------------------------+
| logos-app plugin (logos-module/)  |
+-----------------------------------+
| swap-ffi (C bridge / cdylib)      |
+-----------------------------------+
| Swap orchestration library        |
+----------------+------------------+
| alloy (ETH)    | nssa_core (LEZ)  |
+----------------+------------------+
```

## Design notes

- SHA-256 is used for the hashlock so both chains share the same primitive.
- The taker locks first, so the ETH timelock is longer and the LEZ timelock is shorter.
- LEZ timelocks are enforced on-chain; local wall-clock checks are just for UX.
- Waku messaging runs in-process through `libwaku`; there is no separate Docker service.

For more detail on the messaging side, see [delivery-dogfooding.md](delivery-dogfooding.md).

## Troubleshooting

- `logos-scaffold: command not found`
  Ensure `logos-scaffold` is installed and that `~/.cargo/bin` is on your `PATH`.
- `missing lez at .scaffold/lez-cache/repos/lez/...`
  `make setup` did not finish successfully. Install `logos-scaffold` if needed, then rerun `make setup`.
- `Risc Zero Rust toolchain not found. Try running rzup install rust`
  Install RISC Zero and run `rzup install rust`, then rerun the command that failed.
- Git pull blocked by untracked `scaffold.toml`
  Older clones sometimes had that file gitignored. Move it aside, pull again, then compare your old copy with the checked-in [`scaffold.toml`](scaffold.toml).

## Maintainer notes

- Bump `CIRCUITS_VERSION` in the [`Makefile`](Makefile) when the `lssa` revision in [`Cargo.toml`](Cargo.toml) needs a newer published `logos-blockchain-circuits` release.
- Bump `[repos.lez].pin` in [`scaffold.toml`](scaffold.toml) when intentionally moving to a different LEZ revision.

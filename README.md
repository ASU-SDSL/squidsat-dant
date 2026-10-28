# DANT firmware tests

The firmware runs hardware tests directly, without an application state machine.
The radio branch adds SX1268 breakout and DANT REV1 RFM98PW tests. The CAN branch
adds standalone raw CAN and ISO-TP payload tests. Each test has its own build
instructions; Nucleo images must not be used on the DANT PCB.

Use an existing Zephyr workspace with the STM32 SDK and Python dependencies.
Initialize the dependencies with `git submodule update --init --recursive`.
Run `west build` from the workspace root. For a Git worktree, run west from the
Zephyr workspace and pass the application's absolute path.

- Radio sources: `antenna_app`; breakout instructions: `antenna_app/RADIO_BREAKOUT.md`.
- DANT serial-only radio check: `dant_radio_test/README.md` (transmission disabled).
- CAN instructions: `CAN_TESTS.md`; raw test: `can_smoke_test`;
  ISO-TP test: `sdsl-can/sdsl-can-lib/tests/payload_size_test`.

The corresponding feature branch supplies each test and its documentation.

## Prerequisites

Install the standard Zephyr dependencies for your OS:

https://docs.zephyrproject.org/latest/develop/getting_started/index.html

You will also need:

- Python 3
- `west`
- CMake and Ninja
- A Zephyr-supported ARM toolchain
- ST-LINK tools if you want to use `flash.sh`

## First-Time Setup

Create and activate a Python virtual environment from the repository root.

macOS/Linux:

```sh
python3 -m venv .venv
source .venv/bin/activate
python -m pip install --upgrade pip west
```

Windows PowerShell:

```powershell
py -3 -m venv .venv
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
.\.venv\Scripts\Activate.ps1
python -m pip install --upgrade pip west
```

Initialize the Zephyr workspace and install Python requirements:

```sh
west init -l antenna_app
west update
west zephyr-export
python -m pip install -r zephyr/scripts/requirements.txt
```

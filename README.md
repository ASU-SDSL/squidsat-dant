# SquidSat DANT

Firmware workspace for the SquidSat deployable antenna / radio practice project. The active Zephyr application lives in `antenna_app` and currently includes:

- SX1268/RFM98 radio support through RadioLib
- CAN bus setup and message helpers
- Nanopb/protobuf support for `dant_link.proto`
- Nucleo-F103RB board configuration and overlay

## Repository Layout

```text
antenna_app/                 Zephyr application
antenna_app/src/             Application source files
antenna_app/src/can_handler.* CAN helper code
antenna_app/src/radio.*      Radio task and queue code
antenna_app/boards/          Board overlays
antenna_app/west.yml         Zephyr west manifest
flash.sh                     Helper script for flashing with st-flash
```

`antenna_app/lib/RadioLib` is a Git submodule. If you did not clone with `--recurse-submodules`, initialize it before building:

```sh
git submodule update --init --recursive
```

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

## Build

Build the application for the Nucleo-F103RB:

```sh
west build -p always -b nucleo_f103rb antenna_app -d build
```

The firmware binary is generated at:

```text
build/zephyr/zephyr.bin
```

## Flash

If `st-flash` is installed and available in your `PATH`, use the helper script:

```sh
./flash.sh
```

To flash a different build directory:

```sh
./flash.sh path/to/build
```

## Useful Files

- `antenna_app/prj.conf`: Zephyr Kconfig options for GPIO, SPI, CAN, logging, C++, and Nanopb.
- `antenna_app/boards/nucleo_f103rb.overlay`: CAN bus, LED, and button device tree overlay.
- `antenna_app/src/main.cpp`: Radio initialization and test entry point.
- `antenna_app/src/can_handler.cpp`: CAN initialization, receive filter, and send helper.

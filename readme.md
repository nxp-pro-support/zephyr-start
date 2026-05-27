# Background

This repository contains an example Zephyr application laid out as a west
manifest workspace.

This approach lets new developers spin up an application with minimal effort.
The manifest file (`west.yml`) pulls in a pinned Zephyr tree plus the other
modules/libraries the project depends on, so everyone in the project ends up
with the same build environment.

The manifest also references a sample module/library repository as an example
of how to structure shared libraries that are consumed by multiple
applications.

Some background on Zephyr workspaces and west manifest files:

- https://docs.zephyrproject.org/latest/develop/west/basics.html
- https://docs.zephyrproject.org/latest/develop/application/index.html
- https://blog.golioth.io/improving-zephyr-project-structure-with-manifest-files/

# Required Toolchain

> **You must have Zephyr `v4.4.0` and Zephyr SDK `1.0.1` installed before
> this workspace will build.** Older Zephyr and SDK combinations (for
> example Zephyr 4.3 with SDK 0.17, which earlier MCUXpresso installer
> images shipped) are not compatible with this manifest as-is.

This workspace is tracked against:

| Component | Required Version |
| --- | --- |
| Zephyr | `v4.4.0` (pinned in `west.yml` — `west update` will fetch this) |
| Zephyr SDK | `1.0.1` |
| MCUXpresso Installer | `v26.03` or newer |
| Python | `3.12.x` (installer venv) or `3.13.x` (system) |
| west | `>= 1.4.0` |

# Setup

This assumes you have either installed all of the necessary tooling via the
NXP MCUXpresso installer:

https://mcuxpresso.nxp.com/mcux-vscode/latest/html/MCUXpresso-Installer.html

or followed the official Zephyr Getting Started documentation:

https://docs.zephyrproject.org/latest/develop/getting_started/index.html

> **When running the MCUXpresso installer, you must select both the
> "Zephyr Developer Tools v4.4" component and the "Zephyr SDK 1.0.1"
> component.** If you already ran the installer with older selections, run
> it again and add the v4.4 / 1.0.1 components — the installer can install
> multiple SDK / Zephyr versions side by side.

## 0. Activate the Python virtual environment

If you installed Zephyr via the MCUXpresso installer (`v26.03` or newer), the
installer creates a dedicated Python virtual environment that must be
activated before running `west`, `cmake`, or any other tooling from the
terminal.

The current installer places the venv at:

Windows (cmd):

```
%userprofile%\.mcuxpressotools\.mcux-venv-3.12\Scripts\activate.bat
```

Windows (PowerShell):

```powershell
& "$env:USERPROFILE\.mcuxpressotools\.mcux-venv-3.12\Scripts\Activate.ps1"
```

Linux / macOS:

```
source ~/.mcuxpressotools/.mcux-venv-3.12/bin/activate
```

If the PowerShell `Activate.ps1` script is refused with
`running scripts is disabled on this system`, relax the per-user execution
policy once:

```powershell
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
```

Then re-run the activate command. Corporate group policy may override this; if
so, use the `.bat` form in `cmd.exe` instead, or get IT to relax the policy.

Older MCUXpresso installer images used the path
`%userprofile%\.mcuxpressotools\.venv\Scripts\activate.bat`. If you upgraded
from an older installer and still have that directory, the new installer
leaves the old one in place but does not use it.

If you followed the official Zephyr install instead, activate whichever
virtual environment you created for that workflow.

## 1. Create a working folder for the west workspace

```
mkdir zephyr-start-workspace
cd zephyr-start-workspace
```

## 2. Initialize the workspace from this manifest

```
west init -m https://github.com/nxp-pro-support/zephyr-start --mr main
```

This registers the workspace against this repository as the manifest. The
manifest repo itself is checked out at `zephyr-start/` in the workspace root.

## 3. Run `west update`

```
west update
```

This may take several minutes the first time. It pulls down the pinned Zephyr
tree plus all of the manifest-listed dependencies. Future `west update` calls
are much quicker because they only pull deltas.

![zephyr_start](https://github.com/nxp-pro-support/zephyr-start/assets/152433281/8df1b0aa-721d-4895-a4ae-12a2d6c6ff4d)

## 4. Build the hello world

Open the workspace folder in VS Code (or whichever editor you prefer). The
`zephyr-start` folder at the workspace root contains an `app/` folder with
`hello_world` and `demos/`.

Build the hello world for the NXP RT1050-EVK from the workspace root:

```
cd zephyr-start-workspace
west build -p always -b mimxrt1050_evk/mimxrt1052/qspi zephyr-start/app/hello_world
```

> **Board name format note (Zephyr 4.x hwmv2):** Several NXP EVK boards now
> require a flash-variant suffix in the board name. For example,
> `mimxrt1050_evk` alone is rejected and must be `mimxrt1050_evk/mimxrt1052/qspi`
> or `mimxrt1050_evk/mimxrt1052/hyperflash`. Multi-core SoCs use a similar
> `/soc/cpu` suffix, e.g. `frdm_mcxn947/mcxn947/cpu0`. Boards with multiple
> hardware revisions use the `@` syntax, e.g.
> `mimxrt1170_evk@A/mimxrt1176/cm7`.

This repository also includes a local out-of-tree custom board example at
`boards/nxp/rt1050_custom`. From the workspace root:

```
west build -p always -b rt1050_custom zephyr-start/app/hello_world
```

![build_hello_world](https://github.com/nxp-pro-support/zephyr-start/assets/152433281/00d49408-9aa8-410d-a03b-b8b575e245ca)

# Verified build matrix (Zephyr v4.4.0, SDK 1.0.x)

The following app / board combinations have been built clean in this
workspace as of this README revision. Use them as a quick sanity check after
you set up the toolchain.

| App | Board | Notes |
| --- | --- | --- |
| `app/hello_world` | `mimxrt1050_evk/mimxrt1052/qspi` | |
| `app/hello_world` | `rt1050_custom` | local custom board in this repo |
| `app/demos/bare_metal_direct` | `frdm_mcxn947/mcxn947/cpu0` | |
| `app/demos/direct_irq` | `frdm_mcxn947/mcxn947/cpu0` | |
| `app/demos/display` | `frdm_mcxn947/mcxn947/cpu0` | |
| `app/demos/display` | `mimxrt1170_evk@A/mimxrt1176/cm7` | |
| `app/demos/frdm_can_shell_monitor_cdc` | `frdm_mcxa156` | see app README |
| `app/demos/frdm_can_traffic_generator` | `frdm_mcxw71` | see app README |
| `app/demos/frdm_shell_bench_tool_cdc` | `frdm_mcxa156` | |
| `app/demos/frdm_shell_bench_tool__i2c_gpio` | `frdm_mcxa156` | |
| `app/demos/hello_rtt` | `mimxrt1170_evk/mimxrt1176/cm7` | SEGGER RTT console |
| `app/demos/hello_zephyr` | `frdm_mcxn947/mcxn947/cpu0` | |
| `app/demos/i2c_shell` | `frdm_mcxn947/mcxn947/cpu0` | |
| `app/demos/lan8651_t1s_demo` | `frdm_mcxa156` | see app README |
| `app/demos/lan8651_t1s_demo` | `frdm_mcxa266` | see app README |
| `app/demos/littlefs` | `mimxrt1060_evk/mimxrt1062/qspi` | |
| `app/demos/lvgl` | `mimxrt1170_evk@A/mimxrt1176/cm7 --shield rk055hdmipi4ma0` | display via shield |
| `app/demos/synchronization` | `frdm_mcxn947/mcxn947/cpu0` | optional Segger SystemView, see app README |
| `app/demos/z_for_each` | `frdm_mcxn947/mcxn947/cpu0` | |

Apps under `app/demos/` that are unmodified upstream Zephyr samples
(`can_counter`, `dhcpv4_client`, `rw612-ble-shell`) are kept here as reference
and are not part of this build matrix. Build them per the upstream Zephyr
sample documentation if needed.

# 5. Debug with Segger Ozone

In this workflow Segger Ozone is used as the debugger after a build:

https://www.segger.com/products/development-tools/ozone-j-link-debugger/

Ozone is a standalone debugger that works with a J-Link (or an EVK with the
on-board debug probe re-flashed as a J-Link). It is a popular option in the
Zephyr community and has features that other debuggers do not, such as live
variable viewing and real-time plotting.

Inside the `hello_world` application is a file `RT1050.jdebug` which is
preconfigured to point at the output of the hello world build:

`zephyr-start/app/hello_world/build/zephyr/zephyr.elf`

![debug_w_ozone](https://github.com/nxp-pro-support/zephyr-start/assets/152433281/a989f9bd-2523-4e7b-8b55-adedeb7094d7)

### Important note for i.MX RT parts

The i.MX RT series is flashless with a ROM bootloader, so creating an Ozone
project takes a couple of extra steps.

You can use the existing `RT1050.jdebug` as a template by copying it into
another project's folder. It will pick up `build/zephyr/zephyr.elf` from
whichever project it sits in.

**Example:** copy `RT1050.jdebug` into `samples/drivers/uart/echo_bot` and
then build / run:

![debug_uart_driver_sample](https://github.com/nxp-pro-support/zephyr-start/assets/152433281/0e4389b4-df8a-4205-a49e-6a86e8a922b4)

Segger note about Ozone debug with a ROM bootloader:

https://wiki.segger.com/Debug_on_a_Target_with_Bootloader

> If your MCU has a ROM bootloader (or a custom bootloader) that needs to be
> executed first, simply leave the functions `AfterTargetDownload()` and
> `AfterTargetReset()` empty (but not commented out!). This will override
> Ozone's default and nothing will be executed so the ROM bootloader can run
> without interference and jump to the application space where per default
> Ozone will then stop at `main`.

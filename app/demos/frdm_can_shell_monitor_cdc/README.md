# FRDM CAN Shell Monitor CDC

This demo turns the FRDM-MCXA156 into a USB CDC connected CAN shell monitor.
It is intentionally close to a zero-application-code tool: the firmware hosts
Zephyr's shell over USB CDC ACM and enables `CONFIG_CAN_SHELL`.

## Zephyr Version Note

This workspace is Zephyr `v4.4.0`. The CAN shell in 4.4 includes the
`can dump <device>` command, which is a simpler way to stream raw frames than
the filter/parse approach used here. This demo intentionally keeps the
catch-all `can filter add` + frame-line parsing flow because the Web Serial UI
in `scripts/` consumes that printed format. If you only need a console dump,
`can dump can0` is available.

## Build And Flash

Run from this folder:

```powershell
cd C:\zephyr-start-workspace\zephyr-start\app\demos\frdm_can_shell_monitor_cdc
west build -p always -b frdm_mcxa156 .
west flash -r jlink
```

After flashing, use the target USB CDC ACM port. On this machine the Zephyr CDC
port usually enumerates as VID/PID `2FE3:0004`.

## Manual Shell Flow

Open the CDC shell and run:

```text
device list
can show can0
can stop can0
can bitrate can0 500000
can mode can0 normal
can filter add can0 000 000
can filter add can0 -e 00000000 00000000
can start can0
```

The first filter is catch-all for standard 11-bit CAN IDs. The second filter is
catch-all for extended 29-bit CAN IDs. Matching frames are printed by the CAN
shell. Use `listen-only` instead of `normal` for passive sniffing when another
active node is already present to ACK frames.

The project keeps the application code minimal. The robustness changes are still
configuration-only: the serial shell TX/RX buffers and CAN shell RX queue are
larger than the defaults so Web Serial has more headroom while frames stream.

To transmit a frame:

```text
can mode can0 normal
can start can0
can send can0 123 11 22 33 44
```

## Web Serial UI

Serve the `scripts` folder from localhost:

```powershell
cd C:\zephyr-start-workspace\zephyr-start\app\demos\frdm_can_shell_monitor_cdc\scripts
python -m http.server 8766
```

Open:

```text
http://localhost:8766/webserial_can_monitor.html
```

Chrome or Edge is required for Web Serial.

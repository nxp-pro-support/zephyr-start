# FRDM CAN Traffic Generator

Small Zephyr app for an FRDM-MCXW71 that generates CAN traffic for the
`frdm_can_shell_monitor_cdc` demo.

## Defaults

- Board: `frdm_mcxw71`
- CAN device: chosen `zephyr,canbus`, aliased as `sys-can`
- Bitrate: `500000`
- Sample point: `875`
- Console/shell: default J-Link UART, typically `COM7` on this bench

The app sends three frames per cycle:

- Standard ID `0x156`
- Standard ID `0x321`
- Extended ID `0x18ff1560`

## Build and Flash

Run commands from this folder:

```powershell
cd C:\zephyr-start-workspace\zephyr-start\app\demos\frdm_can_traffic_generator
west build -p always -b frdm_mcxw71 .
west flash -r jlink
```

If more than one J-Link board is attached, pass the J-Link serial number to the
runner:

```powershell
west flash -r jlink --dev-id <jlink-serial>
```

## Runtime Shell

Open the J-Link UART at `115200 8N1`.

```text
can_gen status
can_gen stop
can_gen start
can_gen start 100
can_gen rate 500
can_gen burst 20
can_gen restart
```

For a two-node bench setup, run the MCXA156 monitor in normal CAN mode so it can
ACK frames from the generator. Listen-only mode is useful for passive sniffing,
but a lone transmitter will see ACK errors if every other node is listen-only.
After connecting an ACKing node, `can_gen restart` is the fastest way to clear a
stalled transmit mailbox.

The generator also auto-recovers from the no-ACK case. If its transmit mailboxes
fill because no other active CAN node is acknowledging frames, it prints a short
message, restarts the CAN controller, waits briefly, and retries.

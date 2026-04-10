# LAN8651 10BASE-T1S demo

This demo is a small Zephyr application for experimenting with the Microchip
LAN8651 on `frdm_mcxa156` and `frdm_mcxa266`.

What it does:

- brings up a static IPv4 configuration
- logs Ethernet carrier and IPv4 address events
- runs a UDP echo server on port `4242`
- optionally runs a minimal HTTP status server on port `8080`

## Measured footprint

Built in this workspace with Zephyr `4.4.0-rc2`:

| Build | Flash | RAM |
| --- | ---: | ---: |
| `frdm_mcxa156` UDP only | 76,692 B | 31,760 B |
| `frdm_mcxa156` UDP + HTTP | 90,320 B | 46,344 B |
| `frdm_mcxa266` UDP only | 81,736 B | 31,760 B |
| `frdm_mcxa266` UDP + HTTP | 95,364 B | 46,336 B |

That leaves substantial headroom on both boards, including `frdm_mcxa156`
with 1 MiB flash and 128 KiB RAM.

## Wiring

The overlay defaults assume one LAN8651 node on a PLCA network of 8 nodes.
Update the MAC address and `plca-node-id` if you bring up multiple boards.

### `frdm_mcxa266`

Use `LPSPI1` on the Arduino header:

- `D13` -> LAN8651 `SCK`
- `D12` -> LAN8651 `MISO` / `SDO`
- `D11` -> LAN8651 `MOSI` / `SDI`
- `D10` -> LAN8651 `CS`
- `D8` -> LAN8651 `RST`
- `D7` -> LAN8651 `INT`
- `3V3` -> LAN8651 `3V3`
- `GND` -> LAN8651 `GND`

### `frdm_mcxa156`

`frdm_mcxa156` does not route the needed SPI pins to the Arduino SPI header the
same way. The overlay uses `LPSPI1` on header `J2` for the bus and Arduino GPIOs
for control:

- `J2 pin 12` -> LAN8651 `SCK`
- `J2 pin 10` -> LAN8651 `MISO` / `SDO`
- `J2 pin 8` -> LAN8651 `MOSI` / `SDI`
- `D4` -> LAN8651 `CS`
- `D8` -> LAN8651 `RST`
- `D7` -> LAN8651 `INT`
- `3V3` -> LAN8651 `3V3`
- `GND` -> LAN8651 `GND`

## Build

From the workspace root:

```powershell
west build -p always -b frdm_mcxa156 zephyr-start\app\demos\lan8651_t1s_demo -d build\lan8651_mcxa156
west build -p always -b frdm_mcxa266 zephyr-start\app\demos\lan8651_t1s_demo -d build\lan8651_mcxa266
```

Optional HTTP footprint variant:

```powershell
west build -p always -b frdm_mcxa156 zephyr-start\app\demos\lan8651_t1s_demo -d build\lan8651_mcxa156_http -- -DEXTRA_CONF_FILE=web.conf
west build -p always -b frdm_mcxa266 zephyr-start\app\demos\lan8651_t1s_demo -d build\lan8651_mcxa266_http -- -DEXTRA_CONF_FILE=web.conf
```

## Test

After boot, the board logs the configured IPv4 address and starts:

- UDP echo on `4242`
- HTTP status on `8080` when built with `web.conf`

Examples:

```powershell
# UDP echo
$udp = New-Object System.Net.Sockets.UdpClient
$udp.Connect("192.0.2.156", 4242)
$payload = [System.Text.Encoding]::ASCII.GetBytes("hello")
[void]$udp.Send($payload, $payload.Length)

# HTTP status
curl http://192.0.2.156:8080/
```

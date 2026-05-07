# Web Serial UI

The browser UI talks to the Zephyr shell over Web Serial. It builds commands
such as `i2c scan sys_i2c` and `gpio set sys_io LED_RED 1`; the board firmware
does not need a custom host protocol.

Demo flow:

```text
1. Connect to the debugger UART or USB CDC shell port.
2. Press Discover to parse device list, I2C devices, GPIO devices, and GPIO lines.
3. Press Scan I2C to run i2c scan sys_i2c and populate the address grid.
4. Press Read Temp to read P3T1755 register 0x00 at I2C address 0x48.
5. Press Start 1s Temp Poll to plot temperature once per second.
6. Use Query GPIO to populate named line controls, then Configure, Set, Get, or Toggle.
```

Run from this directory:

```powershell
python -m http.server 8765
```

Then open:

```text
http://localhost:8765/webserial_bench_tool.html
```

Use Chrome or Edge. Web Serial requires a user click to grant port access, so
press `Connect` in the page and choose the J-Link CDC UART device.

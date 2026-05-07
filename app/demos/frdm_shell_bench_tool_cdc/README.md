# FRDM Shell Bench Tool CDC

This variant starts from Zephyr's USB CDC ACM pattern and routes the Zephyr
shell to the FRDM-MCXA156 target USB port. The demo point is that the board can
become a USB-to-I2C/GPIO bench tool mostly through Kconfig and devicetree.

## What Changed From The USB CDC ACM Sample

Zephyr's `samples/subsys/usb/cdc_acm` creates a CDC ACM UART and runs an echo
application on top of it. For the bench-tool demo, the shell owns that UART
instead:

```text
CONFIG_USB_DEVICE_STACK_NEXT=y
CONFIG_CDC_ACM_SERIAL_INITIALIZE_AT_BOOT=y
CONFIG_SHELL=y
CONFIG_I2C_SHELL=y
CONFIG_GPIO_SHELL=y
```

The FRDM overlay creates `cdc_acm_uart0`, selects it as `zephyr,shell-uart`, and
adds friendlier shell labels:

```text
sys_i2c -> lpi2c0
sys_io  -> gpio3
```

## Build And Flash

Run these commands from this folder:

```powershell
cd C:\zephyr-start-workspace\zephyr-start\app\demos\frdm_shell_bench_tool_cdc
west build -p always -b frdm_mcxa156 .
west flash
```

If the board is using J-Link firmware or an external J-Link probe:

```powershell
west flash -r jlink
```

After flashing, use the target USB connector, not the MCU-Link debugger UART.
Open the new CDC ACM COM port at any baud rate; CDC ignores the baud setting.

## Demo Commands

```text
help
device list
device list i2c

i2c scan sys_i2c
i2c read sys_i2c 48 00 2

gpio devices
gpio info sys_io
gpio conf sys_io LED_RED ol0
gpio set sys_io LED_RED 1
gpio set sys_io LED_RED 0
gpio toggle sys_io LED_BLUE
```

For GPIO configuration, `o` means output, `l` means active low, and the final
`0` initializes logical off. The FRDM LEDs are active low, so `gpio set ... 1`
turns the selected LED on after that configuration.

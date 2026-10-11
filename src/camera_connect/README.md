# M5Stack CoreS3 GC0308 Camera Connection Test (Zephyr RTOS)

This project demonstrates the implementation and verification of the onboard **GalaxyCore GC0308** camera sensor on the **M5Stack CoreS3** (ESP32-S3) running **Zephyr RTOS**.

It includes an out-of-tree Zephyr video driver for GC0308, custom DeviceTree overlays, and a test application that verifies hardware reset sequencing, I2C probing, chip identification (PID: `0x9b`), and Zephyr Video Subsystem API integration.

---

## Hardware Architecture & Pinout

The M5Stack CoreS3 integrates a GalaxyCore GC0308 (0.3 MegaPixel, VGA 640x480) CMOS image sensor connected via an 8-bit DVP parallel interface and an I2C (SCCB) bus.

### 1. Pin Mapping

| Function | Signal | Connection | Notes |
| :--- | :--- | :--- | :--- |
| **SCCB Clock** | SIOC / SCL | **GPIO 11** | Shared internal I2C bus (`i2c0`) |
| **SCCB Data** | SIOD / SDA | **GPIO 12** | Shared internal I2C bus (`i2c0`) |
| **I2C Address** | - | `0x21` (7-bit) | Standard GC0308 SCCB address |
| **Reset** | `CAM_RST` | **AW9523B P1_0** (Pin 8) | Controlled via I2C I/O expander (Active-Low) |
| **Power Down** | `CAM_PWDN` | **GND** | Pulled down with 10kΩ resistor (Always Active) |
| **System Clock** | `MCLK` / `XCLK`| **20MHz Oscillator** | Onboard oscillator `X2` (No ESP32-S3 GPIO needed) |
| **Vertical Sync** | `VSYNC` | **GPIO 46** | DVP VSYNC signal |
| **Horizontal Ref**| `HREF` | **GPIO 38** | DVP HREF / Data enable signal |
| **Pixel Clock** | `PCLK` | **GPIO 45** | DVP Pixel clock |
| **Data Bits (8-bit)**| D0 | **GPIO 39** | Mapped to DVP `DATA_IN0` (Schematic: CAM_D2) |
| | D1 | **GPIO 40** | Mapped to DVP `DATA_IN1` (Schematic: CAM_D3) |
| | D2 | **GPIO 41** | Mapped to DVP `DATA_IN2` (Schematic: CAM_D4) |
| | D3 | **GPIO 42** | Mapped to DVP `DATA_IN3` (Schematic: CAM_D5) |
| | D4 | **GPIO 15** | Mapped to DVP `DATA_IN4` (Schematic: CAM_D6) |
| | D5 | **GPIO 16** | Mapped to DVP `DATA_IN5` (Schematic: CAM_D7) |
| | D6 | **GPIO 48** | Mapped to DVP `DATA_IN6` (Schematic: CAM_D8) |
| | D7 | **GPIO 47** | Mapped to DVP `DATA_IN7` (Schematic: CAM_D9) |

### 2. Power Management (AXP2101 PMIC)
The sensor relies on three power rails controlled by the onboard AXP2101 PMIC:
* `VDDCAM_3V3`: **ALDO3** (3.3V) - Digital I/O supply
* `AVDD`: **BLDO1** (2.8V) - Sensor analog supply
* `DVDD`: **BLDO2** (3.3V) - Sensor core digital supply

All three rails are configured with `regulator-boot-on` and `regulator-always-on` in the DeviceTree overlay.

---

## Software Implementation

* **DeviceTree Overlay** (`boards/m5stack_cores3_esp32s3_procpu.overlay`):
  - Configures the ESP32-S3 pinctrl group `lcd_cam_default` with the DVP pinmux assignments.
  - Adds the `gc0308` node to `&i2c0` with `reset-gpios = <&aw9523b_gpio 8 GPIO_ACTIVE_LOW>`.
  - Enables `&lcd_cam` and `&lcd_cam_dvp` controllers with `cam-clk = <20000000>` and links endpoints.
* **GC0308 Sensor Driver** (`drivers/video/gc0308.c`):
  - Implements Zephyr Video Driver API (`video_driver_api`).
  - Performs active-low hardware reset via the AW9523B GPIO expander.
  - Probes register `0x00` and validates PID `0x9b`.
  - Applies default initialization register table.
  - Configured with initialization priority `90` to safely run after the AW9523B I2C expander is ready.
* **Application** (`src/main.c`):
  - Performs direct I2C read of register `0x00` to confirm PID `0x9b`.
  - Confirms Zephyr video sensor device readiness (`device_is_ready`).
  - Queries negotiated video format via `video_get_format()`.

---

## Build & Flash Instructions

### Prerequisites
* Zephyr development environment initialized with West.
* Zephyr SDK installed.
* Python virtual environment activated (`.venv`).

### 1. Build
```powershell
# In PowerShell:
& "$env:USERPROFILE\zephyrproject\.venv\Scripts\Activate.ps1"
$env:ZEPHYR_BASE = "$env:USERPROFILE\zephyrproject\zephyr"

# Build for M5Stack CoreS3 PROCPU:
west -z $env:ZEPHYR_BASE build -p auto -b m5stack_cores3/esp32s3/procpu .
```

Or execute the provided build script:
```powershell
powershell -ExecutionPolicy Bypass -File .\build_app.ps1
```

### 2. Flash to CoreS3
Connect the M5Stack CoreS3 via USB-C and run:
```powershell
# Flash via West:
west -z $env:ZEPHYR_BASE flash -d build --esp-device COM9
```

Or execute the provided flash script:
```powershell
powershell -ExecutionPolicy Bypass -File .\flash_app.ps1
```

---

## Real Hardware Verification Log

The following serial output was captured live from the physical M5Stack CoreS3 hardware over COM9 at 115200 bps:

```text
ESP-ROM:esp32s3-20210327
Build:Mar 27 2021
rst:0x15 (USB_UART_CHIP_RESET),boot:0x28 (SPI_FAST_FLASH_BOOT)
Saved PC:0x40377082
SPIWP:0xee
mode:DIO, clock div:1
load:0x3fc90c50,len:0x2d28
load:0x40374000,len:0xcc40
load:0x50000000,len:0x24
SHA-256 comparison failed:
Calculated: ff7f7de272f59f903b0bbb7bf9fc977187d1f25f1c858c43572492056b5c90b9
Expected: 0000000030060000000000000000000000000000000000000000000000000000
Attempting to boot anyway...
entry 0x4037a89c
I (soc_init): ESP Simple boot
I (soc_init): compile time Oct  3 2026 17:58:49
W (soc_init): Unicore bootloader
I (soc_init): chip revision: v0.2
I (flash_init): Boot SPI Speed : 80MHz
I (flash_init): SPI Mode       : DIO
I (flash_init): SPI Flash Size : 16MB
I (boot): DRAM	: lma=00000020h vma=3fc90c50h size=02d28h ( 11560)
I (boot): IRAM	: lma=00002d50h vma=40374000h size=0cc40h ( 52288)
I (boot): RTC_DATA	: lma=0000f998h vma=50000000h size=00024h (    36)
I (boot): IROM	: lma=00010000h vma=42000000h size=085c0h ( 34240)
I (boot): DROM	: lma=00020000h vma=3c010000h size=031b4h ( 12724)
I (boot): libc heap size 325 kB.
I (cache): Instruction cache: size 16KB, 8Ways, cache line size 32Byte
I (spi_flash): detected chip: generic
I (spi_flash): flash io: dio
[00:00:00.025,000] <dbg> video_gc0308.gc0308_hw_reset: Asserting HW reset via GPIO
[00:00:00.078,000] <inf> video_gc0308: GC0308 camera sensor successfully detected! Chip ID: 0x9b
[00:00:00.203,000] <inf> video_gc0308: GC0308 initialized successfully (Default: 320x240 RGB565)
*** Booting Zephyr OS build v4.4.0-12700-g8dafb9a897da ***
[00:00:00.203,000] <inf> main: ==============================================
[00:00:00.203,000] <inf> main:  M5Stack CoreS3 Camera Connection Test App   
[00:00:00.203,000] <inf> main: ==============================================
[00:00:00.203,000] <inf> main: I2C0 controller is ready.
[00:00:00.203,000] <inf> main: Probing GC0308 on I2C address 0x21...
[00:00:00.203,000] <inf> main: >>> Direct I2C Read Success: Reg[0x00] = 0x9b
[00:00:00.203,000] <inf> main: >>> MATCH! GC0308 Image Sensor confirmed (PID: 0x9b)!
[00:00:00.203,000] <inf> main: GC0308 Video Device is ready: gc0308@21
[00:00:00.203,000] <inf> main: Current Camera Format: 320x240 (FourCC: RGBP)
[00:00:00.203,000] <inf> main: ESP32-S3 LCD_CAM DVP Interface is ready: lcd_cam_dvp
[00:00:00.203,000] <inf> main: Camera probe and verification sequence completed.
```

---

## Next Steps
1. **DMA Video Frame Capture**:
   - Enqueue video buffers (`video_enqueue`) into the ESP32-S3 DVP driver and capture raw image frames.
2. **Built-in LCD Display Preview**:
   - Render the captured 320x240 RGB565 frames onto the CoreS3 ILI9342C display.

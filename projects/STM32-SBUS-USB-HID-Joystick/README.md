# STM32 SBUS to USB HID Joystick — UAV Pilot Simulator Training

**Personal engineering project by Mustafa Hassan Mohamed Hassan**  
**Project application:** ASTRON TEAM — pilot training and RC simulator interface  
**Field:** Embedded C · STM32 · SBUS · DMA · USB HID · System Integration

## Why I developed this project

I developed this interface to enable the **ASTRON TEAM pilot to practise flying in a simulator using the same Radiolink AT9S Pro radio transmitter used to control our team's UAV**. This allows familiar control-stick movements and switch operation to be practised on a computer rather than learning on an unrelated game controller.

The device bridges the radio receiver's **SBUS output** to a **native USB HID joystick** recognised by Windows. The project was tested with **PicaSim**; it is a pilot-training interface, not an onboard aircraft flight-control system.

## System architecture

```text
Radiolink AT9S Pro transmitter
             │ 2.4 GHz radio control link
             ▼
Radiolink R9DS v2.1 receiver
             │ SBUS, 100 kbaud, inverted, even parity, 2 stop bits
             ▼
NUCLEO-F303RE / STM32F303RE
   USART1 RX (PA10) + circular DMA
   SBUS 25-byte frame parsing and channel decoding
   Channel mapping, scaling and dead-zone
   Custom USB HID joystick report (15 bytes)
             │ native USB: PA11 (D−), PA12 (D+)
             ▼
Windows — "STM32 SBUS Joystick"
             │
             ▼
PicaSim — Roll / Pitch / Throttle / Yaw
```

## My engineering work

- Developed and integrated the **STM32F303RE embedded interface** between Radiolink R9DS SBUS data and USB HID joystick reports.
- Configured **USART1 reception and circular DMA** for continuous input and decoded SBUS channel data.
- Mapped transmitter sticks, switches and rotary controls into joystick axes and buttons, including dead-zone treatment for centred channels.
- Implemented a **15-byte USB HID input report** containing seven 16-bit axes and eight button bits.
- Integrated the receiver, NUCLEO development board and native USB data connection.
- Verified device recognition and controls in Windows and configured primary flight controls in PicaSim.
- Recorded wiring, troubleshooting, signal-loss behaviour and test observations in the project documentation.

## Hardware and tools

| Component / Tool | Role |
|---|---|
| Radiolink AT9S Pro | Actual UAV radio transmitter used for simulator practice |
| Radiolink R9DS v2.1 | SBUS receiver output |
| NUCLEO-F303RE (STM32F303RE) | SBUS decoding and USB HID generation |
| STM32 HAL / STM32CubeIDE | Embedded firmware and build tools |
| USART1 + DMA | Non-blocking SBUS reception |
| Native USB FS | Joystick device interface |
| Windows `joy.cpl` | Joystick recognition and axis/button testing |
| PicaSim | Flight simulator used for validation |

## Confirmed results from the project records

| Verification | Recorded observation |
|---|---|
| Firmware build | 0 errors / 0 warnings |
| SBUS receive rate | Approximately 74–75 frames/second |
| Parser status under normal reception | `err=0`, `flags=0x00` |
| Windows device recognition | `STM32 SBUS Joystick — Status: OK` |
| HID controls | Axes and buttons checked in Windows Game Controllers |
| Simulator input | Roll, Pitch, Throttle and Yaw mapped in PicaSim |
| Receiver signal-loss check | SBUS failsafe flags observed; receiver output altered throttle channel |

These results are drawn from the original project testing documentation. They were **not retested while publishing this page**. Receiver failsafe output should not be treated as a separately engineered, safety-rated MCU failsafe.

## Implementation details

- **SBUS framing:** 25-byte frames, 16 channels with 11-bit values; first eight channels used in the joystick mapping.
- **Nominal channel scaling:** measured operating range approximately 200–1800 mapped to HID 0–32767.
- **Main controls:** CH1 Roll → X; CH2 Pitch → Y; CH3 Throttle → Z; CH4 Yaw → Rz.
- **Auxiliary controls:** CH5 three-position switch mapped to buttons; CH6, CH7 and CH8 mapped to additional axes.
- **USB HID:** 7 × 16-bit axes (14 bytes) plus 8 button bits (1 byte).
- **USB development setup:** two distinct connections — ST-LINK for programming/debugging and native STM32 USB for HID.

## ASTRON TEAM relationship

The project is **my own engineering implementation**, developed for practical use by **ASTRON TEAM** in pilot training. It is also documented in the team's avionics integration repository.

- [ASTRON TEAM organisation](https://github.com/ASTRON-TEAM)
- [My GitHub profile](https://github.com/MustafaHassanMH7)

## Source availability

This page publishes the purpose, architecture and documented test results. **The original STM32CubeIDE firmware project, executable image, photos and full test logs are not included in this public page yet.** Their absence does not imply that the source files were uploaded or independently rebuilt.

*Project documentation based on the recorded NUCLEO-F303RE / Radiolink AT9S Pro + R9DS implementation and PicaSim verification.*

## Original project package / Kaynak dosyalarÄ±

- [Original STM32CubeIDE firmware, wiring, tests and complete Turkish instructions](implementation/README.md)


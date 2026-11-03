# Teknik Detaylar — Firmware Mimarisi

Bu dosya projeyi **kullanmak için gerekli değildir**. Kaynak kodu anlamak veya projeyi sıfırdan yeniden oluşturmak isteyenler içindir.

## Donanım ve clock

- MCU: STM32F303RE
- Board: NUCLEO-F303RE
- HSE: 8 MHz bypass
- PLL: ×6
- SYSCLK/HCLK: 48 MHz
- PCLK1: 24 MHz
- PCLK2: 48 MHz
- USB: 48 MHz

## USART1 / SBUS

```text
Baud: 100000
Word Length: 9 bits
Parity: Even
Stop: 2
Mode: RX only
RX inversion: Enabled
```

HAL yapılandırmasının etkin sonucu SBUS için 8 data + even parity + 2 stop bitiyle uyumludur.

## DMA

```text
USART1_RX
DMA1 Channel 5
Peripheral → Memory
Circular
High priority
Memory increment enabled
Byte width
Buffer: 64 bytes
```

Ana parser DMA interrupt callback'iyle frame beklemek yerine `__HAL_DMA_GET_COUNTER()` üzerinden güncel yazma konumunu okuyup yeni byte'ları işler.

## SBUS frame

Firmware 25-byte frame kullanır:

```text
byte 0     : 0x0F start
byte 1..22 : 16 kanal × 11 bit = 176 bit
byte 23    : flags
byte 24    : 0x00 end (bu doğrulanan sistemde)
```

Her kanal 11 bit'tir. Kanal çözme mantığı:

```c
bit_index = channel * 11;
byte_index = 1 + (bit_index / 8);
bit_offset = bit_index % 8;
packed_data = frame[byte_index]
            | (frame[byte_index + 1] << 8)
            | (frame[byte_index + 2] << 16);
channel_value = (packed_data >> bit_offset) & 0x07FF;
```

## DMA circular okuma

```text
current_pos = 64 - DMA_remaining_counter
```

- `current_pos > old_pos`: old_pos → current_pos arası işlenir.
- `current_pos < old_pos`: buffer sona sarılmıştır; önce old_pos→63, sonra 0→current_pos işlenir.

## Kanal → HID dönüşümü

Ölçülen çalışma aralığı yaklaşık `200…1800` olduğundan değer önce clamp edilir, sonra:

```text
HID = (SBUS - 200) × 32767 / 1600
```

CH1/CH2/CH4 için merkez yaklaşık 1000 ve dead-zone ±25'tir.

CH5 switch:

```text
CH5 < 600          → Button 1
600 ≤ CH5 < 1400   → Button 2
CH5 ≥ 1400         → Button 3
```

## HID report

7 adet 16-bit eksen = 14 byte, 8 adet button biti = 1 byte. Toplam:

```text
15 bytes
```

Byte layout:

```text
0..1   X       CH1 Roll
2..3   Y       CH2 Pitch
4..5   Z       CH3 Throttle
6..7   Rx      CH6 knob
8..9   Ry      CH7 lever
10..11 Rz      CH4 Yaw
12..13 Slider  CH8 knob
14     Buttons CH5
```

16-bit değerler little-endian gönderilir.

Kritik sabitler:

```c
#define HID_JOYSTICK_REPORT_SIZE 15U
#define USBD_CUSTOM_HID_REPORT_DESC_SIZE 50U
#define CUSTOM_HID_EPIN_SIZE 0x10U
```

`usbd_customhid.h` içindeki varsayılan/fallback descriptor boyutu değil, proje yapılandırmasındaki `USB_DEVICE/Target/usbd_conf.h` içindeki 50U değeri aktiftir.

## Ana loop

Yaklaşık mantık:

```text
SBUS_ProcessDmaRx()
↓
10 ms geçti ve son SBUS frame ≤100 ms eskiyse HID report gönder
↓
500 ms'de LD2 toggle
↓
1000 ms'de USART2 debug satırı yaz ve fps sayaçlarını sıfırla
```

## Failsafe

Doğrulanmış alıcı davranışı:

- Normal `flags=0x00`
- Verici kapatılınca önce `0x04`, sonra `0x0C`
- CH3 yaklaşık 200
- Verici açılınca `0x00`

Firmware flags'i raporlar; bağımsız safety-critical failsafe politikası STM32 tarafında ayrıca uygulanmış değildir.

## CubeMX doğrulanmış ayarları

- PA10 = USART1_RX
- PA11 = USB_DM
- PA12 = USB_DP
- USB Device FS = Custom HID
- Product string = `STM32 SBUS Joystick`
- USART2 = ST-LINK VCP debug

Hazır projeyi kullanmak için bu ayarları yeniden oluşturmanız gerekmez.

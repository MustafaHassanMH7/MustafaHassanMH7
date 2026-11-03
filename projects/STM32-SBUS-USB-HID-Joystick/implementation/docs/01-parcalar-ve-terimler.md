# Parçalar ve Temel Terimler

## Parça listesi

| Parça | Görevi |
|---|---|
| Radiolink AT9S Pro | Kullanıcının hareketlerini radyo ile gönderir |
| Radiolink R9DS v2.1 | Radyo sinyalini alır ve SBUS üretir |
| NUCLEO-F303RE | SBUS'u USB joystick'e çevirir |
| ST-LINK USB kablosu | Firmware yükleme, güç ve debug |
| USB data kablosu | STM32 native USB D+/D-/GND |
| Jumper kablolar | Bağlantılar |
| ~1.5 kΩ direnç | PA12/D+ pull-up |
| Windows PC | Programlama, test ve simülatör |

## Terimler

**Pin:** Kart üzerindeki elektrik bağlantı noktası. Örn. `PA10`, `PA11`, `PA12`.

**GND (Ground):** Devrenin ortak referans noktası. R9DS ve NUCLEO'nun GND'si ortak olmalıdır.

**5V / 3V3:** Besleme voltajları. Birbirleriyle karıştırılmamalıdır.

**SBUS:** RC alıcının birçok kanalı tek dijital hatta taşıdığı seri protokol.

**USART/UART:** STM32'nin seri veri alıp gönderen donanım birimi. Bu projede SBUS `USART1_RX` üzerinden gelir.

**DMA:** Gelen seri veriyi işlemci her byte için uğraşmadan RAM'e taşıyan donanım.

**USB HID:** Klavye, mouse, joystick gibi standart USB giriş aygıtları sınıfı.

**D+ / D-:** USB veri hatları. Bu projede `D+=PA12`, `D-=PA11`.

**Pull-up:** Bir sinyal hattını direnç üzerinden belirli voltaja çeken bağlantı. Bu prototipte `3V3 → ~1.5 kΩ → PA12/D+`.

**ST-LINK:** NUCLEO üzerindeki programlayıcı/debugger. Joystick USB değildir.

## Direnç alternatifi

```text
1 kΩ + 330 Ω + 220 Ω ≈ 1.55 kΩ
```

Bu üç direnç seri bağlanır.

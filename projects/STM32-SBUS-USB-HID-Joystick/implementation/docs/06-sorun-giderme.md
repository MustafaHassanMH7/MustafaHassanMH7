# Sorun Giderme

| Belirti | Muhtemel neden | Çözüm |
|---|---|---|
| Windows joystick'i görmüyor | D+/D-/pull-up/kablo | PA12=D+, PA11=D-, GND ortak, ~1.5k pull-up, data kablosu kontrolü |
| `NO SBUS frames` | SBUS gelmiyor | R9DS SBUS modu, PA10, 5V, GND, kumanda bağlantısını kontrol et |
| fps 74–75 ama joystick yok | Native USB tarafı | PA11/PA12 ve 48 MHz USB firmware'ini kontrol et |
| Sadece COM port var | USB-TTL kullanılmış | USB-TTL kaldır; PA11/PA12 native USB kullan |
| ST-LINK takılı ama joystick yok | Normal davranış | Ayrı native USB D+/D-/GND bağlantısı gerekir |
| Windows çalışıyor, PicaSim çalışmıyor | PicaSim mapping yok | Enable joystick + Joystick 0/1/2/3 mapping |
| Eksen ters | Yön | PicaSim `Invert` |
| `48 warnings / excess elements...` | Descriptor size yanlış | `USBD_CUSTOM_HID_REPORT_DESC_SIZE=50U` |
| USB arada kopuyor | Uzun/dağınık kablo | D+/D- kablolarını kısa ve yakın tut, ortak GND kontrol et |

## SBUS normal log

```text
fps=74–75
err=0
flags=0x00
end=0x00
```

## Failsafe doğrulaması

```text
Transmitter ON:  flags=0x00
OFF sonrası:     flags=0x04 → 0x0C
CH3:             yaklaşık 200
Tekrar ON:       flags=0x00
```

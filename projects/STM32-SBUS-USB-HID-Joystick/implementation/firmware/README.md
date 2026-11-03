# Firmware

Bu klasör çalışan STM32 projesini içerir.

## Programlama bilmeyen kullanıcı

En kolay yol `release/STM32_SBUS_USB_HID_Joystick.elf` dosyasını STM32CubeProgrammer ile karta yüklemektir. Ana [README](../README.md) bölüm 5'i izleyin.

## Geliştirici

STM32CubeIDE projesi:

```text
firmware/STM32_Project/F303RE_Blink_Test_v2
```

Klasör adı tarihsel olarak Blink testinden kalmıştır. İçerik final SBUS USB HID Joystick firmware'idir.

Doğrulanmış build:

```text
0 errors, 0 warnings
```

Hazır çalışan projeyi yalnızca kullanıyorsanız `.ioc → Generate Code` yapmanız gerekmez.

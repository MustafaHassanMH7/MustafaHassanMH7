# Firmware Yükleme

## Yöntem A — Programlama bilmeyen kullanıcı için önerilen yol

Hazır dosya:

```text
release/STM32_SBUS_USB_HID_Joystick.elf
```

1. STM32CubeProgrammer kurun: https://www.st.com/en/development-tools/stm32cubeprog.html
2. NUCLEO'yu ST-LINK USB ile bağlayın.
3. STM32CubeProgrammer'da `ST-LINK` seçip `Connect` basın.
4. `.elf` dosyasını açın.
5. `Download` ile flash'a yazın.
6. Kartı resetleyin.

Bu yöntem kaynak kodu derlemeyi gerektirmez.

## Yöntem B — Kaynak koddan derlemek

STM32CubeIDE: https://www.st.com/en/development-tools/stm32cubeide.html

Import:

```text
File → Import → Existing Projects into Workspace
```

Klasör:

```text
firmware/STM32_Project/F303RE_Blink_Test_v2
```

Build sonucu:

```text
Build Finished. 0 errors, 0 warnings.
```

Sonra `Run` veya `Debug` ile ST-LINK üzerinden karta yükleyin.

## Önemli uyarı

Hazır çalışan projeyi yalnızca kullanıyorsanız CubeMX'te yeniden `Generate Code` yapmayın. Custom HID içinde elle değiştirilmiş descriptor/endpoint ayarlarının üzerine yazılabilir.

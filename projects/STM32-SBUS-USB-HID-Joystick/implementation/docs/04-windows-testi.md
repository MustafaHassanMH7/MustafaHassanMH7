# Windows Testi

## Cihazı açın

```text
Win + R → joy.cpl
```

Beklenen:

```text
STM32 SBUS Joystick
Status: OK
```

![Windows joystick algılama](../images/06-windows-joycpl-detected.png)

## Eksenleri test edin

`Properties → Test` açın ve kumandayı hareket ettirin.

![Eksen testi](../images/07-windows-joycpl-test-axes.png)

CH5 C switch üç konumda Button 1/2/3 üretir.

![Buton testi](../images/08-windows-joycpl-test-buttons.png)

Windows burada doğru çalışıyorsa donanım ve firmware zinciri büyük ölçüde doğrulanmıştır.

# Sık Sorulan Sorular

## ST-LINK USB tek başına neden yetmiyor?

Çünkü ST-LINK ayrı bir programlayıcı/debugger bölümüdür. STM32F303RE'nin HID verisi PA11/PA12 native USB hatlarından çıkar.

## USB-TTL kullanabilir miyim?

Hayır. USB-TTL size seri COM port verir. Bu proje USB HID joystick ister.

## USB kablosunun kırmızı telini neden bağlamıyoruz?

İlk geliştirme testinde NUCLEO zaten ST-LINK USB'den beslenir. İkinci USB kablosunun 5V hattını bağlamak iki güç kaynağını birleştirebilir.

## Neden direnç var?

Çalışan prototipte PA12/D+ hattına 3V3 üzerinden yaklaşık 1.5 kΩ pull-up kullanılmıştır.

## 1.5 kΩ yoksa ne kullanabilirim?

Bu projede doğrulanmış alternatif:

```text
1 kΩ + 330 Ω + 220 Ω seri = 1.55 kΩ
```

## joy.cpl çalışıyor ama PicaSim çalışmıyor. Firmware bozuk mu?

Genellikle hayır. Proje sırasında aynı durum yaşandı; PicaSim'de `Enable joystick` kapalı ve `Map to=None` idi. Mapping yapıldıktan sonra sorun çözüldü.

## `.ioc` dosyasını açıp Generate Code yapmalı mıyım?

Projeyi kullanmak için hayır. Hazır firmware çalışıyor. Generate Code elle düzenlenmiş USB HID dosyalarını etkileyebilir.

## Başka SBUS alıcı kullanabilir miyim?

Muhtemelen mümkün olabilir ancak bu paket yalnızca Radiolink AT9S Pro + R9DS v2.1 ile doğrulandı. Elektrik seviyesi, inversion ve timing farklı olabilir.

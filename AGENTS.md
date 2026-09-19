# module_rgb — RGB-лента/матрица WS2812

> **Опциональный модуль.** Подключается через `src_filter` + `build_flags`.
> **Работоспособен только в составе сборки, содержащей ядро** (см. `../../TRS.md` §2.1).

Управление лентой/матрицей WS2812 (NeoPixelBus): режимы solid/rainbow/gradient/individual/
equalizer, яркость, анимационный таймер, отложенное применение и сохранение конфига.

- **Репозиторий:** https://github.com/Arcanum753/module_rgb
- **Папка:** `src/module_rgb/`
- **Флаг активации:** `-D MODULE_RGB`
- **Registry:** `object=module_rgb`, `define=MODULE_RGB`, `web=1`, `loop=0` (без `namespace`/`res`/`prio`)
- **Только для платформы:** обе (ESP8266/ESP32)
- **Зависит от модулей:** —
- **Зависит от ядра:** `core_web`, `core_sys`, `core_json`

## Назначение

Управление лентой/матрицей WS2812 (NeoPixelBus): режимы solid/rainbow/gradient/individual/
equalizer, яркость, анимационный таймер, отложенное применение/сохранение конфига.

## Функциональные требования

- FR-RGB-1: Конфиг `/config_rgb.json`: `dataPin`, `numLeds`, `brightness`, `mode` (0..4), `effectSpeed`, `solidColor`, `gradStartColor`, `gradEndColor`, `individualColors[]`, `eqBands`, `eqLedsPerBand`.
- FR-RGB-2: Лимиты `RGB_MAX_LEDS=100`, `RGB_DEFAULT_LEDS=10`.
- FR-RGB-3: Режимы: сплошной цвет, радуга, статичный градиент, индивидуальные цвета, эквалайзер.
- FR-RGB-4: Установка пикселя (`/rgb/setPixel`) и сохранение конфига (`/rgb/save`); чтение `/rgb/info`, `/rgb/ver`.
- FR-RGB-5: Анимация — таймером EERTOS; применение конфига — отложенной задачей.

## Аппаратные интерфейсы

| Интерфейс | Выводы по умолчанию | Примечание |
|-----------|---------------------|------------|
| WS2812 (лента) | DATA из `config_rgb.json` (`dataPin`) | NeoPixelBus: ESP32 RMT, ESP8266 UART1 |

## Веб-интерфейс

Страница `rgb.html` (пункт меню — `web/_menu.html`); маршруты `/rgb/setPixel`,
`/rgb/save`, `/rgb/info`, `/rgb/ver`.

## Конфигурация

`/config_rgb.json` — поля: см. FR-RGB-1. Лимит количества светодиодов — FR-RGB-2.

## Слоистая структура

Из `../../LAYERS.md`: `module_rgb` — WS2812 (NeoPixelBus): эффекты, анимация; сейчас
`strRgbConfig`, `NeoPixelBusType`; выделить `_types.h`, `_engine.cpp`. Локальный
stateless-хелпер `hexStringToUint32` — в `common_module.*`, namespace `ns_module_rgb`.

## Ограничения и известные проблемы

- OPEN-7: `module_rgb`/`module_gpio`/`module_editor` не имеют resource-bus и недоступны из макросов. **Каноническое описание:** масштабировать правило apply/save на компоненты без resource-bus — отдельная задача; см. также `../module_gpio/AGENTS.md`, `../module_editor/AGENTS.md`.

## Ссылки

- Ядро и конвенции: `../../TRS.md`
- Слоистая структура: `../../LAYERS.md`
- Общие утилиты: `../../TRS.md` §3.1.12 (`common/`)
- Сборка: `../../BUILD.md`
- Реестр компонентов: `../../INVENTORY.md`

> Если модуль читается вне дерева ядра (standalone), корневые документы доступны в
> репозитории ядра avr-fota.

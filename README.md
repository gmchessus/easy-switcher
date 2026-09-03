# easy-switcher — объединённый мануал (README + SKILL)

> Сконсолидировано из `README.md` и `SKILL.md` (единый источник правды).

sudo apt install cmake libevdev-dev  
git clone https://github.com/freemind001/preview.git  
cd preview  
mkdir build  
cd build  
cmake ..  
make  
sudo make install  
sudo easy-switcher --configure  
sudo systemctl enable easy-switcher  
sudo systemctl start easy-switcher  

---

# (содержимое бывшего SKILL.md)

---
name: easy-switcher
description: Демон переключения раскладок клавиатуры для Linux (Wayland/X11). C++/cmake/libevdev. Конвертирует текст из неверной раскладки через виртуальную клавиатуру (uinput). Горячая клавиша Pause (слово) / Shift+Pause (фраза). Локальный патч fast-path emission для ускорения конвертации.
---

# Easy Switcher — демон переключения раскладок клавиатуры

Демон для Linux, переключающий раскладку набранного текста через виртуальную клавиатуру (uinput/libevdev). Работает на Wayland (KDE Plasma/KWin) и X11.

## Как работает

1. Пользователь набирает текст в неверной раскладке (например, «ghbdtn» вместо «привет»).
2. Нажимает **Pause** (конвертация слова) или **Shift+Pause** (конвертация фразы).
3. Демон генерирует последовательность событий: смена раскладки → backspace каждого символа → повторный набор в правильной раскладке.

## Сборка и установка

```bash
sudo apt install cmake libevdev-dev
git clone https://github.com/freemind001/preview.git
cd preview
mkdir build && cd build
cmake ..
make
sudo make install
sudo easy-switcher --configure
sudo systemctl enable easy-switcher
sudo systemctl start easy-switcher
```

## Локальные изменения (патч fast-path emission)

### Проблема

В оригинальном коде `VirtualKeyboard::emit_key` спит `delay` после **каждого** события. При конвертации слова из N букв генерируется `4 + 4N` событий (4 смена раскладки + 2N backspace + 2N набор). Итоговая пауза: `(4 + 4N) * delay`.

При `delay=50` и слове из 5 букв: `24 * 50 = 1200 мс`.

### Решение

Разделение эмиссии на два пути:
- **Смена раскладки** (первые 3-4 события) — через `emit_key` с настраиваемым `delay` (критично для KWin).
- **Backspace и повторный набор** — через `emit_key_fast` с фиксированной паузой 2 мс.

Результат при `delay=50`, слово из 5 букв:
- до: `24 * 50 = 1200 мс`
- после: `4 * 50 + 20 * 2 = 240 мс`

### Применение патча

```bash
cd <upstream repo>
git apply 0001-fast-path-emission.patch
```

Патч находится в `easy-switcher/0001-fast-path-emission.patch`.

## Структура исходников

| Файл | Назначение |
|------|------------|
| `src/main.cpp` | Точка входа, event loop, input handler |
| `src/VirtualKeyboard.cpp/h` | Эмуляция клавиатуры через uinput |
| `src/Converter.cpp/h` | Конвертация текста между раскладками |
| `src/Config.cpp/h` | Чтение конфигурации |
| `src/DeviceManager.cpp/h` | Управление input-устройствами |
| `src/Event.cpp/h` | Структуры событий |
| `src/EventLoop.cpp/h` | Главный цикл (epoll/select) |
| `src/InputReader.cpp/h` | Чтение нажатий клавиш |

## Конфигурация

Файл: `/etc/easy-switcher/config.conf` (пример в `resources/default.conf.example`).

Ключевые параметры:
- `delay` — задержка (мс) после смены раскладки (для KWin/Wayland)
- `hotkey` — горячая клавиша конвертации слова
- `hotkey_phrase` — горячая клавиша конвертации фразы
- `layout1`, `layout2` — раскладки для конвертации (по умолчанию en/ru)

## Конвертации раскладки

По умолчанию: английская ↔ русская. Поддерживаются любые пары раскладок, определённые в `Converter`.

Порядок событий конвертации:
1. Эмуляция комбинации смены раскладки (Alt+Shift или одна клавиша)
2. Backspace каждого символа с конца к началу
3. Повторный набор символов в новой раскладке

## Диагностика

```bash
# Статус демона
systemctl status easy-switcher

# Логи
journalctl -u easy-switcher -f

# Проверка uinput
ls -la /dev/uinput
```

## Известные ограничения

- На Wayland (KDE) `delay` должен быть достаточным для KWin (обычно 30-50 мс).
- На X11 можно использовать минимальный `delay`.
- Для работы требуется доступ к `/dev/uinput` (обычно root или группа `input`).

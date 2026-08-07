# Easy Switcher — задержка после смены раскладки

Документация локальных изменений (черновик для upstream-PR).

## Проблема

Демон easy-switcher переключает раскладку и «переписывает» текст через
виртуальную клавиатуру (uinput). В исходном коде `VirtualKeyboard::emit_key`
спит `delay` **после каждого** отправленного события:

```cpp
void VirtualKeyboard::emit_key(int code, int value) {
    libevdev_uinput_write_event(uidev_, EV_KEY, code, value);
    libevdev_uinput_write_event(uidev_, EV_SYN, SYN_REPORT, 0);
    usleep(delay * 1000);   // после КАЖДОГО события
}
```

При конвертации слова из N букв генерируется `4 + 4N` событий
(4 — смена раскладки, 2N — backspace, 2N — повторный набор).
Итоговая пауза = `(4 + 4N) * delay`.

Например при `delay=50` и слове из 5 букв: `24 * 50 = 1200 мс` (~1.2 c).
На Wayland/KDE (KWin обрабатывает синтетический ввод с задержкой) большой
`delay` нужен именно **для момента переключения раскладки**, но платить им
за каждую букву backspace/повторного набора бессмысленно — конвертация
ощущается как «тормоза».

## Решение

Разнести две задачи по задержке:

- события **смены раскладки** отправляются как раньше, через `emit_key`
  (с настраиваемым `delay`) — это критичный момент, чтобы KWin успел
  переключить раскладку до начала повторного набора;
- **backspace и повторный набор** идут через новый `emit_key_fast`
  с фиксированной малой паузой (2 мс) — достаточно, чтобы uinput не
  «сливал» события в кучу.

## Изменённые файлы

### `src/VirtualKeyboard.h`
Объявлен новый метод:

```cpp
void emit_key_fast(int code, int value);
```

### `src/VirtualKeyboard.cpp`
Добавлена реализация без `delay`:

```cpp
void VirtualKeyboard::emit_key_fast(int code, int value) {
    if (!uidev_) return;

    libevdev_uinput_write_event(uidev_, EV_KEY, code, value);
    libevdev_uinput_write_event(uidev_, EV_SYN, SYN_REPORT, 0);

    usleep(2 * 1000);
}
```

### `src/main.cpp` (`input_handler`)
Эмиссия разделена по этапам. Первые `switch_events` событий — смена
раскладки (с `delay`), остальные — быстрый путь:

```cpp
const auto events = conv.convert(action_needed);
const int switch_events = (conv.ls_keys[1] != 0) ? 4 : 3;
for (size_t i = 0; i < events.size(); ++i) {
    const auto &ev = events[i];
    if (i < static_cast<size_t>(switch_events)) {
        vk.emit_key(ev.code, ev.value);
    } else {
        vk.emit_key_fast(ev.code, ev.value);
    }
    if (debug_mode) { /* ... */ }
}
```

`switch_events` = 4, если комбинация смены раскладки из двух клавиш
(по умолчанию `56+42` = Alt+Shift: down/down/up/up), иначе 3 (одна клавиша).
Это повторяет логику порядка событий из `Converter::convert`.

## Результат

При `delay=50`, слово из 5 букв:
- до: `24 * 50 = 1200 мс`;
- после: `4 * 50 + 20 * 2 = 240 мс`.

Семантика настройки `delay` не меняется — она по-прежнему отвечает за
паузу после смены раскладки, но больше не тормозит каждый символ.

## Совместимость / обратная совместимость

- Поведение при `emit_key` не тронуто.
- Новый метод добавлен рядом, существующие вызовы не изменены.
- Фиксированные 2 мс в `emit_key_fast` — разумный компромисс; при желании
  можно вынести в конфиг (`fast-delay`).

## Файлы в этом каталоге

- `0001-fast-path-emission.patch` — готовый патч, применяется к апстриму
  (`git apply`) командами:
  ```
  cd <upstream repo>
  git apply 0001-fast-path-emission.patch
  ```
- `src/` — исходники с применённым патчем.

## Тестирование

- Сборка: `cmake -B build && cmake --build build` (g++/cmake/libevdev).
- Проверка: слово в неверной раскладке → Pause (слово) / Shift+Pause
  (фраза). Конвертация выполняется быстро и с корректной сменой раскладки
  на Wayland (KDE Plasma).

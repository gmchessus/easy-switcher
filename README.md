# easy-switcher

Fork of [freemind001/preview](https://github.com/freemind001/preview) — keyboard layout switcher daemon for Linux (Wayland/X11) with a fast-path emission patch for faster conversion.

## What it does

Converts text typed in the wrong keyboard layout (e.g. `ghbdtn` → `привет`) by emulating backspace and re-typing via uinput. Press **Pause** to convert a word, **Shift+Pause** for a phrase.

## Fast-path patch

The original code sleeps after every key event. This fork splits emission into two paths:

- Layout switch events — original `emit_key` with configurable `delay`
- Backspace/retype events — new `emit_key_fast` with fixed 2ms pause

Result for a 5-letter word with `delay=50`: **1200ms → 240ms**.

## Build & install

```bash
sudo apt install cmake libevdev-dev
git clone https://github.com/gmchessus/easy-switcher.git
cd easy-switcher
mkdir build && cd build
cmake ..
make
sudo make install
sudo easy-switcher --configure
sudo systemctl enable --now easy-switcher
```

## Configuration

File: `/etc/easy-switcher/config.conf` (example in `resources/default.conf.example`).

| Parameter | Description |
|-----------|-------------|
| `delay` | ms after layout switch (30-50 for KWin/Wayland) |
| `hotkey` | key to convert a word |
| `hotkey_phrase` | key to convert a phrase |
| `layout1`, `layout2` | layouts for conversion (default: en/ru) |

## Diagnostics

```bash
systemctl status easy-switcher
journalctl -u easy-switcher -f
ls -la /dev/uinput
```

## License

See upstream [freemind001/preview](https://github.com/freemind001/preview).

# Airton serial protocol — independent technical specification

**Version:** 1.3 — 30 October 2026

> **Disclaimer** — This document is independent and unofficial. It is not affiliated with Airton,
> Tuya, or their distributors. No warranty of compliance, completeness, or fitness for a particular
> purpose is provided. Use of this information and any associated code is at your own risk; verify
> voltage levels, isolation, and local regulations before connecting.

## 1. Overview

The bus is a clear-text serial link with two data wires and a common ground. The Wi-Fi module is
the master (`0x21`); the air-conditioner board (`0xD5`) and the display (`0xD1`) are slaves. The
module polls the board, forwards changes, and maintains a separate link with the display.

| Line | Direction | Frames |
|---|---|---|
| A | module `0x21` → board `0xD5` / display `0xD1` | `A1`, `A2`, `AB`, `A5` |
| B | board `0xD5` / display `0xD1` → module `0x21` | `A3`, `A6`, `AC`, `A4` |

| Physical layer | Value |
|---|---|
| Bitrate | 9600 baud, 8N1 |
| Levels | TTL 5 V |
| Wires | TX, RX, common GND |
| Master | `0x21` — module |

| Address | Role |
|---|---|
| `0x21` | Wi-Fi module (master) |
| `0xD5` | air-conditioner board |
| `0xD1` | display / thermostat |

## 2. Frame format

```text
7A 7A | SRC | DST | LEN | 00 00 | TYPE | PAYLOAD | CRC16
```

- `SRC` / `DST`: source and destination addresses.
- `LEN`: total length including CRC (12–34).
- `TYPE`: message type.
- `CRC16`: CRC-16/MODBUS, high byte first, computed over `0 .. LEN-3`.

Frames are split by `LEN`: find `7A 7A`, read the length, wait for the complete frame, verify the
CRC, then consume.

### CRC-16/MODBUS

| Parameter | Value |
|---|---|
| Polynomial | `0x8005` reflected → `0xA001` |
| Init | `0xFFFF` |
| RefIn / RefOut | yes / yes |
| Final XOR | `0x0000` |
| Coverage | `0 .. LEN-3` |
| Byte order | high byte first |

Test vector:

```text
7A 7A 21 D5 0C 00 00 AB 0A 0A  →  FC F9
```

## 3. Messages

| Type | Size | Direction | Cadence | Role |
|---|---:|---|---|---|
| `AB` | 12 | `21 → D5` | 60 s | module poll to the board |
| `A2` | 12 | `21 → D5` | at boot | startup poll |
| `AC` | 18 | `D5 → 21` | ~80 ms | poll response: total usage time and revision |
| `A1` | 24 | `21 → D5` | on change | module command to the board |
| `A3` | 34 | `D5 → 21` | 60 s + immediate | full board state |
| `A6` | 28 | `D5 → 21` | with `A3` | complementary state, all zero |
| `A5` | 27 | `21 → D1` | 10 s | module announcement to the display |
| `A4` | 13 | `D1 → 21` | ~0.1 s | display acknowledgement |

## 4. `A1` command — module → board

| Offset | Field | Value |
|---|---|---|
| `8–11` | reserved | `00 00 00 00` |
| `12` | state byte | bit 7 = super, bits 4–6 = fan, bit 3 = power, bits 0–2 = mode |
| `13` | setpoint | bits 0–4 = `setpoint − 16`; bit 5 = Fahrenheit; bit 6 = quiet |
| `14` | swing | bit 0 = vertical, bit 4 = horizontal; other bits preserved |
| `15` | flags | see the flags table |
| `16–21` | reserved | `00` |
| `22–23` | CRC16 | high byte first |

Example — heating, 24 °C, auto, vane off:

```text
7A 7A 21 D5 18 00 00 A1 00 00 00 00 0C 08 00 80 00 00 00 00 00 00 8A 98
```

## 5. `A3` state — board → module

| Offset | Field | Value |
|---|---|---|
| `8–9` | prefix | `0A 0A` |
| `10` | room temperature | integer °C |
| `11` | room temperature decimal | tenths of °C; current = `A3[10]` + `A3[11]`/10 |
| `13` | state byte | same encoding as `A1[12]` |
| `14` | setpoint | same encoding as `A1[13]` |
| `15` | swing | bit 0 = vertical, bit 4 = horizontal |
| `16` | flags | see the flags table |
| `21` | category | `01` cool/heat_cool, `02` dry, `03` fan_only, `04` heat |
| `23–24` | revision | counter, e.g. `BC 58` |
| `25–31` | identifier | `DB11370` (ASCII) |
| `32–33` | CRC16 | high byte first |

## 6. Encodings

State byte, low 3 bits (with power bit `0x08` set):

| Mode bits | Mode |
|---|---|
| `0` | heat_cool |
| `1` | cool |
| `2` | dry |
| `3` | fan_only |
| `4` | heat |

Off clears the power bit; the mode bits are kept.

State byte, bits 4–6 (fan):

| Fan bits | Fan speed |
|---|---|
| `0` | auto (or quiet via setpoint bit `0x40`) |
| `1` | low |
| `2` | medlow |
| `3` | medium |
| `4` | medhigh |
| `5` | high |
| `6` | no fan mode; preserved |
| `8` | super bit (combined with a speed) |
| `D` | strong / TURBO (`8` + `5`) |

**Swing:** bit 0 = vertical, bit 4 = horizontal: `00` off, `01` vertical, `10` horizontal,
`11` both; other bits preserved. Positions 1–5 are not exposed.

### Flags (`A1[15]` and `A3[16]`)

| Bit | Meaning |
|---|---|
| `0x80` | display on; clear for display off |
| `0x40` | ionizer |
| `0x10` | health; the board shares this bit between anti-mould and auxiliary heat |
| `0x08` | unknown/status (observed, preserved) |
| `0x04` | unknown/status (observed, preserved) |
| `0x02` | sleep |
| `0x01` | eco |

The flags are preserved in `A1` commands. The `0x10` bit is not a cool-mode flag: cooling uses
`0x80`, like the other modes.

## 7. `AC` poll response

| Offset | Field |
|---|---|
| `8–9` | prefix `0A 0A` |
| `10–11` | total usage time, hours (big-endian, DP 102) |
| `12` | unknown (`00` observed) |
| `13–14` | revision (matches `A3[23:24]`) |
| `15` | unknown (`01` observed) |
| `16–17` | CRC16 |

Example:

```text
7A 7A D5 21 12 00 00 AC 0A 0A 5A D1 00 BC 75 01 78 27
```

`0x5AD1` = 23249 hours; `BC75` is the revision.

## 8. Timing and sequences

| Event | Frames | Delay |
|---|---|---|
| display heartbeat | `A5` then `A4` | 10 s |
| board poll | `AB` then `AC` | 60 s |
| periodic state | `A3` + `A6` | 60 s |
| command | `A1` then `A3` + `A6` | ~170 ms |
| remote key press | `A3` + `A6` | immediate, no `A1` |

At power-up the module sends an `A5` variant with byte 11 `04` and bytes 16–18 `01 01 01`, then an
`A2` startup poll, then the normal `A5`. After the power cycle byte `A5[16]` becomes `01`; its
meaning is not confirmed.

## 9. Reference frames

```text
AB  7A 7A 21 D5 0C 00 00 AB 0A 0A FC F9
A2  7A 7A 21 D5 0C 00 00 A2 0A 0A FE 29
AC  7A 7A D5 21 12 00 00 AC 0A 0A 5A D1 00 BC 75 01 78 27
A1  7A 7A 21 D5 18 00 00 A1 00 00 00 00 0C 08 00 80 00 00 00 00 00 00 8A 98
A3  7A 7A D5 21 22 00 00 A3 0A 0A 19 00 00 0C 08 01 98 00 00 00 00 04 64 BC 58 44 42 31 31 33 37 30 76 9E
```

## 10. Implementation notes

- Verify the voltage (TTL 5 V) and the common ground before connecting.
- Power the ESP from the module connector's +12 V through a 5 V regulator (a 12 V → 5 V buck module or a bare 7805); never connect +12 V directly to the ESP.
- Never transmit on the bus while the original module is still connected.
- Preserve unknown fields and verify the CRC of every frame.
- Wait for the first `A3` before sending `A1`; before that, the unit state is unknown.
- Reject frames shorter than 10 bytes and release a partial frame after 50 ms without new bytes.
- Test in listen-only mode first, then transmit, with a rollback path.

## 11. Limitations and liability

Vane positions 1–5, some display/status bits and fields not listed here remain undocumented.
Preserve unknown values.

Document provided “as is”, without express or implied warranty. The author disclaims any liability
for material, software, or personal damage, data loss, or non-compliance with regulations resulting
from the use of this information. Trademarks belong to their respective owners. This document is
neither official nor approved by Airton or Tuya.

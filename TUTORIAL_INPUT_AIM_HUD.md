# Crown's Bane — Туторіал: Інпут v2, Камера, Прицілювання, HUD, День/Ніч

> **Передумови:** попередні туторіали пройдені, `BP_PlayerShip` є в рівні.
> **Час:** ~30 хв.

---

## Що змінилось у цьому оновленні (v2)

| # | Що | Файли |
|---|---|---|
| 1 | **Канонічний overlay IMC** — кодом створюється валідний IMC завжди | `ShipPawn.cpp` |
| 2 | **Камера повертається мишкою** (IA_Look + раw polling) | `ShipPawn.h/.cpp` |
| 3 | **Raw polling завжди запущений + дебаунс** | `ShipPawn.cpp` |
| 4 | **Виправлена формула день/ніч** (cos, а не лінійна) | `DayNightSystem.cpp` |
| 5 | **«Моон»-освітлення вночі** | `DayNightSystem.cpp` |
| 6 | **FlushPressedKeys + ConsumeCaptureMouseDown** | `CrownsBanePlayerController.cpp` |
| 7 | **Прицілювання AC4-стиль** (залишилось з v1) | `CannonComponent.h/.cpp` |
| 8 | **HUD polish: crosshair, годинник, плаваючі цифри** (з v1) | `CrownsBaneHUD.cpp` |

---

## ЧАСТИНА 1 — Виправлення інпуту v2 (остаточне-остаточне)

### Проблема з v1

У v1 я додав raw polling, але гейтив його `if (!bEnhancedInputReady)`. Виявилось:
якщо користувач має **свій** IMC зі зламаними мапінгами (наприклад, `A` без
Negate-модифікатора), Enhanced Input «успішно» зареєстрований (`bEnhancedInputReady = true`),
але IMC робить неправильне — raw polling не спрацьовує, і гра поводиться дивно.

Плюс: ніде не було прив'язки для повороту камери мишкою.

### Чотири рівні захисту

```
Рівень 1: Канонічний overlay IMC (завжди коректний, створюється в коді)
Рівень 2: Enhanced Input з усіма IA (W/S/A+Negate/D, LMB/Space fire, Mouse2D look)
Рівень 3: Raw polling в Tick() — завжди працює, гейтиться через ConsumeActionCooldown
Рівень 4: FlushPressedKeys + ConsumeCaptureMouseDown в OnPossess (фокус viewport)
```

### Рівень 1 — Канонічний Overlay IMC (нове)

Замість реєстрації `ShipMappingContext` з BP (який може бути зламаний), `AddInputMappingContext()` тепер створює **новий IMC програмно** кожного разу:

```cpp
UInputMappingContext* Overlay = NewObject<UInputMappingContext>(...);
Overlay->MapKey(IA_IncreaseSail, EKeys::W);
Overlay->MapKey(IA_DecreaseSail, EKeys::S);
Overlay->MapKey(IA_Turn, EKeys::D);
FEnhancedActionKeyMapping& MA = Overlay->MapKey(IA_Turn, EKeys::A);
MA.Modifiers.Add(NewObject<UInputModifierNegate>(Overlay)); // ← ключове!
Overlay->MapKey(IA_Turn, EKeys::Right);
FEnhancedActionKeyMapping& ML = Overlay->MapKey(IA_Turn, EKeys::Left);
ML.Modifiers.Add(NewObject<UInputModifierNegate>(Overlay));
Overlay->MapKey(IA_FireLeft, EKeys::Q);
Overlay->MapKey(IA_FireRight, EKeys::E);
Overlay->MapKey(IA_Fire, EKeys::LeftMouseButton);
Overlay->MapKey(IA_Fire, EKeys::SpaceBar);
Overlay->MapKey(IA_Look, EKeys::Mouse2D);
Subsystem->AddMappingContext(Overlay, 0);
```

Це **завжди** створює валідний IMC. Твій IMC_Ship з BP гарантовано не реєструється (щоб уникнути конфлікту axis-сум).

### Рівень 2 — Enhanced Input
IA_Look прив'язаний до `ETriggerEvent::Triggered` з `Axis2D` (Mouse2D дає (dX, dY)).

### Рівень 3 — Raw polling (завжди активний)
Тепер запускається **кожен кадр**, з `ConsumeActionCooldown(...)` який:
- повертає `true` якщо action не спрацьовував у останні 0.15-0.25 сек
- повертає `false` якщо вже спрацьовував → другий сурс проігнорується

І Enhanced Input, і raw polling проходять через той самий debounce table:

```cpp
bool AShipPawn::ConsumeActionCooldown(FName ActionTag, float CooldownSec)
{
    const float Now = GetWorld()->GetTimeSeconds();
    if (const float* Last = ActionFireTimes.Find(ActionTag))
    {
        if (Now - *Last < CooldownSec) return false;
    }
    ActionFireTimes.Add(ActionTag, Now);
    return true;
}
```

Для mouse delta (look) — raw polling активний тільки якщо EI не встановив `bEnhancedInputReady`.

### Рівень 4 — PlayerController focus

```cpp
FInputModeGameOnly Mode;
Mode.SetConsumeCaptureMouseDown(true);
SetInputMode(Mode);
FlushPressedKeys();
```

`FlushPressedKeys()` чистить застарілі натискання, які редактор не встиг відправити.

### Камера — нові біндинги

| Клавіша | Дія |
|---|---|
| **Мишка (рух X)** | Обертання камери по yaw (відносно корабля) |
| **Мишка (рух Y)** | Нахил камери по pitch (обмежено: -70° ↔ +10°) |
| **LookYawSensitivity** | Налаштування в BP_PlayerShip |
| **LookPitchSensitivity** | Налаштування в BP_PlayerShip |

SpringArm має `bInheritYaw = true`, тому `LookYawOffset` додається до yaw корабля. Pitch встановлюється напряму (base `-25°` + look delta).

### Рівень 1 — Enhanced Input
Без змін відносно попереднього туторіалу. Якщо `EnhancedInputComponent` кастується успішно → прапорець `bEnhancedInputReady = true` і прив'язки живуть через стандартний механізм.

### Рівень 2 — Raw Polling (новинка)

Якщо EnhancedInput не спрацював (`bEnhancedInputReady == false`), `ShipPawn::Tick` щокадрово опитує стан клавіш через:

```cpp
APlayerController* PC = Cast<APlayerController>(GetController());
PC->IsInputKeyDown(EKeys::W);   // W / ↑ → IncreaseSail
PC->IsInputKeyDown(EKeys::S);   // S / ↓ → DecreaseSail
PC->IsInputKeyDown(EKeys::A/D); // A/D / ←/→ → TurnAxis
PC->IsInputKeyDown(EKeys::Q);   // Q → Port fire
PC->IsInputKeyDown(EKeys::E);   // E → Starboard fire
PC->IsInputKeyDown(EKeys::LeftMouseButton / SpaceBar); // Camera-aim fire
```

Edge-trigger трекери (`bRawPrevW` тощо) гарантують що `IncreaseSail` викликається **один раз** при натисканні, а не кожен кадр.

### Рівень 3 — OnPossess focus lock

`CrownsBanePlayerController::OnPossess` тепер відразу після посесії відправляє:

```cpp
bShowMouseCursor = false;
SetInputMode(FInputModeGameOnly());
```

Це виправляє найбільш поширену причину "гра не реагує" — курсор PIE-режиму захоплював події мишки.

### Діагностика на екрані

На екрані показується рядок (жовтий):
```
[Ship] Sail=Full Speed=1234 Turn=1.00 EI=OK LastInput=EnhancedInput Ctrl=PlayerController_0
```
- `EI=OK` → Enhanced Input активний
- `EI=FALLBACK` → використовується raw polling
- `LastInput=RawPoll` → підтвердження що raw polling спрацював

---

## ЧАСТИНА 2 — Прицілювання AC4-стиль

### Як це виглядає

Коли камера дивиться вбік (DotProduct > 0.35):
- **Жовті лінії** — траєкторії кожного ядра (балістична крива)
- **Червоне кільце** — зона падіння на поверхні води
- **Червона точка** — точний центр падіння

Якщо гармати ще перезаряджаються → лінії сірі.

### Фізика розрахунку

```
Кожен крок (0.1 сек):
  Vel.Z -= 980 * GravityScale * dt
  Pos   += Vel * dt
  ЗУПИНИТИСЬ якщо Pos.Z <= SeaLevelZ (0 за замовчуванням)
```

40 кроків × 0.1 сек = 4 сек максимального польоту ядра.

### Налаштування в HUD Details

| Властивість | Дефолт | Опис |
|---|---|---|
| `AimArcColorReady` | Жовтий | Колір траєкторій (готово до пострілу) |
| `AimArcColorReload` | Сірий | Колір під час перезарядки |
| `AimImpactRingColor` | Червоний | Колір кільця падіння |
| `AimImpactRingRadius` | 180 | Розмір кільця в world units |
| `SeaLevelZ` | 0 | Z-висота поверхні моря |

> **Порада:** якщо рівень океану інший (наприклад, -1000 у water plugin) — встанови `SeaLevelZ` відповідно в деталях HUD-актора.

---

## ЧАСТИНА 3 — День/Ніч (виправлена формула)

### Стара формула була інвертована

```cpp
// СТАРИЙ КОД (неправильно)
const float Angle = (TimeOfDay / 24.0f) * 360.0f - 90.0f;
```

При `TimeOfDay = 9` (ранок) → `Angle = 45°` → сонце pitch=45° → **світить вгору**.
При `TimeOfDay = 21` (вечір) → `Angle = 225°` (= -135°) → сонце _майже_ вниз.

Тому вдень було темно, а вночі світлішало.

### Нова формула (синусоїдальна)

```cpp
// НОВИЙ КОД (правильно)
const float Pitch = FMath::Cos(TimeOfDay / 24.0f * 2.0f * PI) * 90.0f;
```

| Час | Pitch | Висота сонця |
|---|---|---|
| 00:00 (північ) | +90° | Нижче горизонту |
| 06:00 | 0° | Схід сонця |
| 12:00 (полудень) | **-90°** | **Прямо над головою** |
| 18:00 | 0° | Захід сонця |
| 24:00 | +90° | Нижче горизонту |

### Нічне «місячне» освітлення

Коли сонце нижче горизонту, воно **нічого не освітлює** — сцена пітьма. Щоб ніч виглядала природньо і видима, коли `bIsNight == true` **ми перепризначаємо DirectionalLight** як імітацію місяця:

```cpp
if (bIsNight && Pitch > 0.0f)
{
    SunLight->SetActorRotation(FRotator(-55.0f, 40.0f, 0.0f)); // зверху
    Intensity = NightMoonIntensity; // 3.5
}
```

Тобто — той самий DirectionalLight, але з фіксованим кутом згори і блакитним кольором. Реалістичне місячне світло.

### Нові дефолти

| Параметр | Було | Стало | Чому |
|---|---|---|---|
| `TimeOfDay` | 9.0 | **12.0** | Починаємо в полудень — найбільше світла на старт |
| `NightMoonIntensity` | 2.2 | **3.5** | Більш видиме місячне світло |
| `NightSkyIntensity` | 0.55 | **1.2** | SkyLight вночі дає атмосферне заповнення |

### Цикл доби

```
12:00 → СТАРТ — яскравий день
17:00 → захід сонця (помаранчевий)
19:00 → ніч (сонце «замінюється» на місяць згори)
05:00 → світанок
12:00 → повний цикл (10 хвилин реального часу)
```

### Годинник на HUD

Показується над рядком Wanted (зверху по центру):
```
☀  09:15     (день)
☽  21:45     (ніч)
```

---

## ЧАСТИНА 4 — Плаваючі цифри урону

Коли гравець стріляє і ядро влучає:
- По **корпусу ворога** → **жовтий** текст `-25`
- По **воді** → **блакитний** текст (менш помітний)

Цифри піднімаються вгору та зникають протягом 1.4 секунди.

Реалізовано через `ACrownsBaneHUD::AddFloatingDamage(...)`, яка викликається з `ACannonball::OnHit` тільки для пострілів гравця (ворожі влучання не показуються, щоб не спамити).

---

## ЧАСТИНА 5 — Хрестик (прицільна сітка)

Простий 5-елементний хрестик у центрі екрану:

```
     |
  —  ·  —
     |
```

Дає орієнтир для камерного прицілювання. Колір і розмір регулюються в HUD:
- `CrosshairColor` — RGBA (дефолт: білий 45% прозорості)
- `CrosshairSize` — розмір у пікселях (дефолт: 14)

---

## Загальний чеклист

- [ ] Запустити PIE, перевірити рядок `[Ship]` на екрані
- [ ] Якщо `EI=FALLBACK` — натиснути W → побачити `LastInput=RawPoll`
- [ ] Натиснути A/D → корабль повертає
- [ ] Навести камеру вбік → з'являються жовті дуги прицілювання
- [ ] Вистрілити → побачити жовту цифру урону на ворогові
- [ ] Зачекати 5-10 хвилин → захід → ніч (але всe видно)
- [ ] Перевірити годинник у верхній частині екрану

---

## Відомі обмеження

- `SeaLevelZ = 0` — якщо твій рівень Океану на іншій висоті (Water Plugin може ставити `Z = -2000`), зміни цю властивість в деталях HUD.
- Прицільні дуги показуються навіть якщо боєприпасів немає — вони показують куди *полетіло б* ядро.
- Floating damage numbers прив'язані до `WorldLocation.Z += 140 * dt` в world-space, тому при швидкому русі можуть «спливати» збоку.

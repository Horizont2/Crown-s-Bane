# Crown's Bane — Туторіал: Шторми та полювання за скарбами

> **Передумови:** ти вже пройшов `SETUP_GUIDE.md` і корабель їздить та стріляє.
> **Час:** 25–40 хв (залежно від того, чи маєш готові Niagara/SFX асети).

Цей туторіал покроково показує, як підключити дві нові системи:

1. **Шторми** — періодична погода, що посилює вітер, додає дощ, блискавки та гримоти.
2. **Карти скарбів** — підбираєш свиток → на карті з’являється ціль → пливеш → відкриваєш скриню → получаєш золото/дерево/метал.

Код для обох систем уже є в репозиторії на гілці `claude/unreal-pirate-game-tvSGi`. Тобі залишається зробити Blueprint-обгортки та розставити асети.

---

## ЧАСТИНА 1 — Перекомпіляція C++ (2 хв)

1. Закрий Editor, якщо він відкритий.
2. Правою на `CrownsBane.uproject` → **Generate Visual Studio project files**.
3. Відкрий `.sln`, **Build → Build Solution**.
4. Відкрий Editor.

Після компіляції в редакторі з’являться нові C++ класи:
- `AStormSystem`
- `ATreasureQuestManager`
- `ATreasureChest`
- `ATreasureMapPickup`

---

## ЧАСТИНА 2 — Шторми (10 хв)

### 2.1. GameMode уже все спавнить

`ACrownsBaneGameMode::BeginPlay()` уже створює `AStormSystem` автоматично —
**нічого самому спавнити не треба**. Система починає у фазі `Clear`, потім
сама переходить у `BuildingUp → Storm → Dissipating → Clear` нескінченно.

### 2.2. Зроби Blueprint-обгортку (щоб призначити FX/звуки)

Content Browser → правою → **Blueprint Class** → зверху напиши `StormSystem`
→ обери `AStormSystem`. Назви `BP_StormSystem`.

Відкрий `BP_StormSystem` → **Class Defaults** (верхня панель) → знайди категорії
`Storm|FX` та `Storm|Audio`. Заповни:

| Поле              | Що ставити                                                      |
|-------------------|------------------------------------------------------------------|
| `Rain FX`         | Будь-який дощовий Niagara (напр. з StarterContent або MegaScans) |
| `Lightning Flash FX` | Niagara з яскравою спалахуючою емісією                        |
| `Rain Loop Sound` | Looping SoundCue дощу (або SoundWave з bLooping=true)            |
| `Thunder Sound`   | OneShot SoundCue грому                                           |

> Якщо немає свого контенту — постав `StarterContent` (Add Feature or Content Pack → Starter Content) і використай `Niagara/NS_Rain` + `SFX/Thunder` (якщо є). Пусті поля **не** валять гру — просто не буде ефекту.

### 2.3. Прив’яжи Blueprint до GameMode

Знайди свій BP_GameMode (якщо є) або `BP_CrownsBaneGameMode`. Відкрий
**Class Defaults** → в категорії **Game Systems** постав `Storm System Class`
= `BP_StormSystem`.

> Якщо в тебе ще немає BP_GameMode — створи: **Blueprint Class** → **All Classes** → `CrownsBaneGameMode`. У World Settings постав цей BP як GameMode Override.

### 2.4. (Опційно) Налаштуй тайминги

У `BP_StormSystem → Class Defaults → Storm|Timing` можна підкрутити:
- `Min/Max Clear Duration` — 90/180 сек (між штормами)
- `Min/Max Storm Duration` — 30/60 сек (сам шторм)
- `Buildup Duration`, `Dissipate Duration` — плавні переходи

Для швидкого тесту зменш `Min Clear Duration` до `5` і `Max` до `10` — шторм буде майже одразу.

### 2.5. Перевірка

Зіграй. У верхній частині HUD під зірками розшукуваності з’явиться смужка
шторму: `CLEAR SKIES → STORM INCOMING! → STORM - HANG ON! → STORM DISSIPATING`.
Коли шторм активний, вітер стає x2.5 сильнішим і змінює напрямок у 4 рази
швидше — маневрування реально ускладнюється.

**Debug:** на C++ рівні в логах побачиш `[StormSystem] Storm building up...` та
`[StormSystem] STORM!`.

---

## ЧАСТИНА 3 — Карти скарбів (15 хв)

### 3.1. Створи BP_TreasureChest

Content Browser → правою → **Blueprint Class** → `TreasureChest` → `ATreasureChest`.
Назви `BP_TreasureChest`. Відкрий:

1. **Viewport** → виділи `MeshComponent` → у Details постав Static Mesh — якийсь куб/скриня (з StarterContent підійде `SM_CornerFrame` як заглушка).
2. **Class Defaults** → категорія `Treasure|Reward`:
   - `Gold Reward` = 500
   - `Wood Reward` = 30
   - `Metal Reward` = 15
3. **Class Defaults** → категорія `Treasure|FX` (опційно):
   - `Beacon FX` — Niagara з жовтим/золотим стовпом світла (маяк, щоб видно здалеку)
   - `Open FX` — вибух золотих іскор при відкритті
   - `Open Sound` — переможний звук

### 3.2. Створи BP_TreasureMapPickup

`Blueprint Class` → `TreasureMapPickup` → `ATreasureMapPickup`. Назви
`BP_TreasureMapPickup`. Відкрий:

1. **Viewport** → `MeshComponent` → Static Mesh = плаский свиток/карта (або просто куб).
2. **Class Defaults** → `Treasure Map|FX` (опційно):
   - `Reveal FX` — Niagara з магічним спалахом жовтого/золотого
   - `Reveal Sound` — звук розгортання карти / магії

### 3.3. Створи BP_TreasureQuestManager

`Blueprint Class` → `TreasureQuestManager` → `ATreasureQuestManager`. Назви
`BP_TreasureQuestManager`. Відкрий **Class Defaults**:

- `Treasure Chest Class` = `BP_TreasureChest` ← **критично**, інакше скрині будуть порожніми
- `Min Quest Distance` = 6000 (тобто 60м)
- `Max Quest Distance` = 18000 (180м)
- `Sea Level Z` = 0 (або та висота, на якій у тебе площина океану)
- `Max Active Quests` = 3

### 3.4. Прив’яжи менеджер до GameMode

Відкрий `BP_CrownsBaneGameMode` → **Class Defaults** → категорія
**Game Systems** → `Treasure Quest Manager Class` = `BP_TreasureQuestManager`.

### 3.5. Розстав карти в рівні (або дропай з ворогів)

**Найпростіший варіант:** перетягни 3–5 штук `BP_TreasureMapPickup` з Content
Browser у рівень OceanMap, біля спавну гравця. Підніми Z так, щоб свитки
плавали над поверхнею води.

**Круткіший варіант (необов’язковий):** у `AEnemySpawner` або в `AEnemyShipBase::HandleDeath` додати шанс спавну `BP_TreasureMapPickup` — тоді вбив ворожий корабель, взяв карту, пішов шукати скарб. Це вже наступний крок полірування.

### 3.6. Перевірка

1. Зіграй.
2. Підпливи до свитка → чути звук `RevealSound`, на екрані повідомлення
   `New treasure map! Chest marked 120m away`.
3. У верхній частині HUD (над центром, нижче зірок) з’явиться **золотий компас зі стрілкою**, що вказує напрямок до скрині, і підпис `TREASURE 120 m`.
4. Пливи за стрілкою. На місці знайдеш скриню з променем маяка.
5. Перетни сферу скрині → звук/FX відкриття → `+500 Gold, +30 Wood, +15 Metal`
   додається до інвентарю.

---

## ЧАСТИНА 4 — HUD: що нового видно на екрані

Додалося без твоїх дій (HUD намалює автоматично, якщо відповідні системи
присутні):

| Елемент                        | Де                                 | Коли видно                |
|-------------------------------|------------------------------------|--------------------------|
| **Смужка шторму**             | Верх-центр, під зірками розшуку    | Коли `Intensity > 0.02`  |
| **Підпис фази шторму**        | Над смужкою шторму                 | Разом зі смужкою         |
| **Компас скарбу (золота стрілка)** | Верх-центр, ~18% висоти екрану | Коли є активний квест    |
| **Дистанція до скрині**       | Під стрілкою компаса               | Коли є активний квест    |

Стрілка компаса **обертається відносно носа корабля**: якщо скриня прямо по
курсу — стрілка вгору. Якщо позаду — вниз.

---

## ЧАСТИНА 5 — Розширення (ідеї)

Код уже викидає події, на які легко підписатися з Blueprint:

**Шторм:**
- `OnStormPhaseChanged(EStormPhase NewPhase)` — тригерити кастомні FX/сюжет
- `OnStormIntensityChanged(float NewIntensity)` — керувати пост-процесом, туманом

**Скарби:**
- `OnQuestIssued(FTreasureQuest NewQuest)` — UI-повідомлення "New map!", запис у журнал
- `OnQuestCompleted(FGuid QuestId)` — видавати досягнення, переходити до наступної сюжетної віхи

Приклад використання з Blueprint:
1. Відкрий `BP_CrownsBaneHUD` (або свій HUD) → у BeginPlay дістань `GetStormSystem` → `Bind Event to On Storm Phase Changed` → зроби Custom Event, що свічить пост-процесом.

---

## ЧАСТИНА 6 — Типові проблеми

| Симптом                                              | Причина                                              | Як виправити                                                 |
|------------------------------------------------------|------------------------------------------------------|--------------------------------------------------------------|
| Шторм настає, але дощу не чути / не видно            | Не призначені `RainFX`/`RainLoopSound` у `BP_StormSystem` | Додай асети в Class Defaults                                 |
| Стрілка компаса не з’являється після підбору карти    | Немає `BP_TreasureQuestManager` у World, або не виставлено `Treasure Quest Manager Class` у GameMode | Перевір поле в BP_GameMode Defaults                          |
| Скриня з’явилася, але коштує 0 ресурсів               | У `BP_TreasureQuestManager` не призначений `Treasure Chest Class` → спавниться C++ базовий `ATreasureChest` з дефолтними значеннями | Признач `BP_TreasureChest`                                   |
| Скриня спавниться під водою / у повітрі               | `Sea Level Z` у `BP_TreasureQuestManager` не збігається з висотою океану | Поміряй Z-координату площини океану та постав її в менеджер  |
| Шторм не починається                                  | `Min/Max Clear Duration` занадто великі               | Зменш до 5–10 сек для тесту                                  |
| Компас вказує не туди, куди треба                     | `SeaLevelZ` у менеджера занадто високий/низький → скриня далеко по Z | Підкажи менеджеру правильний Z або використовуй пласку карту |

---

## Готово!

Після цього туторіалу в тебе в грі:

- ✅ Циклічна погода з візуальними та геймплейними наслідками
- ✅ Посилення вітру під час шторму → реальні тактичні рішення (встигнути у порт чи прорватись через шторм)
- ✅ Система квестів на скарби з компасом
- ✅ HUD, що автоматично показує стан шторму і найближчий квест
- ✅ Дропалки, які можна розсунути по мапі або дати ворогам як loot

Наступні кроки, які просяться природно:
- Робити ворогів, які дропають `BP_TreasureMapPickup` з шансом
- Додати **мінімапу** зі стрілками
- Зробити **пост-процес для шторму** (темніше небо, насиченість ↓) через `OnStormIntensityChanged`
- Пов’язати **шторм із погіршенням видимості** (+ туман)

Приємного плавання, капітане.

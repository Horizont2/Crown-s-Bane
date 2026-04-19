# Crown's Bane — Покроковий гайд налаштування в UE5

> **Час на перший запуск:** ~1 година (раніше було 4-5, скоротили завдяки автоматизації).  
> **Потрібно:** Unreal Engine 5.3 через Epic Games Launcher.

Що **автоматично** тепер (нічого не треба клікати в Project Settings):
- Enhanced Input Component клас
- Game Instance = `CrownsBaneGameInstance`
- Global GameMode = `CrownsBaneGameMode`
- Плагіни Water, EnhancedInput, Niagara — вже в `.uproject`
- GameMode вже знає свої Pawn/Controller/HUD класи
- GameMode сам спавнить `WindSystem`, `WantedLevelManager`, `EnemySpawner`, `UpgradeManager`
- `ToggleUpgradeUI` вже на Tab/Escape через `DefaultInput.ini`

---

## ЧАСТИНА 1 — Створення проекту (15 хв)

### 1. Встанови UE 5.3
Epic Games Launcher → Unreal Engine → Library → встановити 5.3.

### 2. Клонуй репозиторій
```
git clone https://github.com/horizont2/crown-s-bane
cd crown-s-bane
git checkout claude/unreal-pirate-game-tvSGi
```

### 3. Генерація .sln та компіляція
1. Правою на `CrownsBane.uproject` → **Generate Visual Studio project files**
2. Відкрий `.sln` у Visual Studio 2022
3. **Build → Build Solution** (Ctrl+Shift+B) — ~5 хв першого разу
4. Двічі клікни `CrownsBane.uproject` — відкриється Editor

> Якщо редактор запитає "missing modules, rebuild?" → Yes.

---

## ЧАСТИНА 2 — Створення Input Actions (10 хв)

**Content Browser** → правою → **New Folder** → `Input`.

Створи 5 Input Actions (правою → **Input → Input Action**):

| Файл              | Value Type       |
|-------------------|------------------|
| `IA_IncreaseSail` | `Digital (bool)` |
| `IA_DecreaseSail` | `Digital (bool)` |
| `IA_Turn`         | `Axis1D (float)` |
| `IA_FireLeft`     | `Digital (bool)` |
| `IA_FireRight`    | `Digital (bool)` |

Створи Mapping Context (правою → **Input → Input Mapping Context**) → `IMC_Ship`.

Відкрий `IMC_Ship`, натисни **+** біля "Mappings", і для кожного action додай клавішу:

| Action            | Клавіша          | Modifiers                     |
|-------------------|------------------|-------------------------------|
| `IA_IncreaseSail` | `W`              | —                             |
| `IA_DecreaseSail` | `S`              | —                             |
| `IA_Turn`         | `D`              | —                             |
| `IA_Turn`         | `A`              | **Negate** (в Modifiers → +)  |
| `IA_FireLeft`     | Left Mouse Button| —                             |
| `IA_FireRight`    | Right Mouse Button| —                            |

Save All (Ctrl+Shift+S).

---

## ЧАСТИНА 3 — Blueprint корабля (15 хв)

### 1. Створи `BP_PlayerShip`

Content Browser → Blueprints папка → правою → **Blueprint Class** → шукай `ShipPawn` → `BP_PlayerShip`.

### 2. Налаштуй меш і Input

Відкрий `BP_PlayerShip`:

1. Ліва панель → клікни `ShipMesh`
2. Details → **Static Mesh** → для тесту: `Shape_Cube` (зі Starter Content)
3. **Scale** → `5, 2, 2` (щоб візуально виглядало як корабель)

4. Клікни сам `BP_PlayerShip` у Components панелі (верхній рядок)
5. Details → прокрути до секції **Input**:

| Поле                 | Значення          |
|----------------------|-------------------|
| Ship Mapping Context | `IMC_Ship`        |
| IA_IncreaseSail      | `IA_IncreaseSail` |
| IA_DecreaseSail      | `IA_DecreaseSail` |
| IA_Turn              | `IA_Turn`         |
| IA_FireLeft          | `IA_FireLeft`     |
| IA_FireRight         | `IA_FireRight`    |

6. **Compile** → **Save**

### 3. Налаштуй GameMode на використання цього BP

Content Browser → правою → **Blueprint Class** → `CrownsBaneGameMode` → `BP_GameMode`.

Відкрий `BP_GameMode`:
- **Default Pawn Class** → `BP_PlayerShip`
- Compile & Save.

**Edit → Project Settings → Maps & Modes → Default GameMode** → `BP_GameMode`.

---

## ЧАСТИНА 4 — Сцена та перший запуск (5 хв)

1. **File → New Level** → вибери **Basic** → збережи як `OpenSea` у `Content/Maps/`
2. На панелі зліва (Place Actors) → **Water** → перетягни **Water Body Ocean** на сцену
3. **Build → Build All Levels** (або зелена кнопка Build)
4. Перетягни `BP_PlayerShip` на сцену **над** водою (Z=200+)
5. Видали `Player Start` якщо він є, або постав його поруч з кораблем
6. Натисни **Play** → керуй W/S/A/D!

### Що має показуватись на екрані (debug):
- Зелений текст: `IMC registered: IMC_Ship`
- Жовтий текст: `[Ship] Sail=Stop  Speed=0  Turn=0.00  Controller=PlayerController_0`

Натисни W — Sail має змінитись на `Half` → `Full`. A/D — Turn має стати ±1.00.

**Якщо нічого не з'являється** — дивись секцію "Діагностика" нижче.

---

## ЧАСТИНА 5 — Гармати (Day 2 — 20 хв)

### 1. Сокети на меші корабля

1. Двічі клікни Static Mesh куба (або твоєї моделі корабля)
2. **Window → Socket Manager**
3. Створи 4 сокети:

| Назва сокета     | Позиція (приблизно) |
|------------------|---------------------|
| `CannonLeft_0`   | зліва, ближче до носа  |
| `CannonLeft_1`   | зліва, ближче до корми |
| `CannonRight_0`  | справа, ближче до носа |
| `CannonRight_1`  | справа, ближче до корми|

> Поверни сокети так щоб їхня X-вісь (червона стрілка) дивилась ПЕРПЕНДИКУЛЯРНО до бортa — це напрямок стрільби.

### 2. Blueprint ядра

Правою в Content → **Blueprint Class** → `Cannonball` → `BP_Cannonball`.

Відкрий, Details → `ProjectileMesh` → `Shape_Sphere`, Scale `0.2, 0.2, 0.2`.

### 3. Прив'яжи ядро до корабля

`BP_PlayerShip` → Details → `CannonComponent`:
- **Cannonball Class** → `BP_Cannonball`
- **Cannons Per Side** → `2`

Compile & Save. Тепер ЛКМ стріляє лівим бортом, ПКМ — правим.

---

## ЧАСТИНА 6 — Вороги (Day 3 — 30 хв)

### 1. Створи Blueprints ворогів

Content → правою → **Blueprint Class** → вибирай батьківський C++ клас:

| C++ клас        | BP              | Меш (тимчасово)             |
|-----------------|-----------------|------------------------------|
| `SloopShip`     | `BP_Sloop`      | куб Scale `4, 1.5, 1.5`     |
| `BrigShip`      | `BP_Brig`       | куб Scale `6, 2, 2`         |
| `GalleonShip`   | `BP_Galleon`    | куб Scale `9, 3, 3`         |

Для кожного:
- `CannonComponent` → Cannonball Class = `BP_Cannonball`
- Details → `ShipMesh` → встанови меш і додай ті ж самі сокети гармат

### 2. Navigation Mesh

1. **Place Actors → Volumes → Nav Mesh Bounds Volume** → перетягни на сцену
2. Розтягни так щоб покривав всю воду (Scale X/Y до 200-500)
3. Натисни **P** — зеленим підсвітиться навігаційна сітка
4. **Build → Build Navigation**

### 3. Прив'яжи вороги до EnemySpawner

GameMode вже спавнить `EnemySpawner` автоматично. Щоб він знав які кораблі спавнити, створи `BP_EnemySpawner`:

Content → **Blueprint Class** → `EnemySpawner` → `BP_EnemySpawner`. Відкрий, Details:
- **Sloop Class** → `BP_Sloop`
- **Brig Class** → `BP_Brig`
- **Galleon Class** → `BP_Galleon`

У `BP_GameMode` → Details → **Enemy Spawner Class** → `BP_EnemySpawner`.

---

## ЧАСТИНА 7 — Лут, Доки, UI (Day 5-6 — 40 хв)

### 1. Лут

- Blueprint: `LootPickup` → `BP_LootPickup` (меш = Shape_Cylinder, Scale 0.5)
- Blueprint: `LootSpawner` → `BP_LootSpawner`, Details → Loot Pickup Class = `BP_LootPickup`
- Перетягни `BP_LootSpawner` на сцену (1 штука)

### 2. Доки

- Blueprint: `DocksZone` → `BP_Docks`
- Перетягни `BP_Docks` на сцену (у бухту на березі)
- Details → Box Component → Box Extent = `1000, 1000, 500`

### 3. UMG Upgrade Widget

Content → правою → **User Interface → Widget Blueprint** → `WBP_UpgradeMenu`.

Відкрий. У Designer додай:
- 4 `Button` з текстом: "Корпус", "Вітрила", "Зброя", "Гармати"
- 3 `TextBlock` для: `Gold: {0}`, `Wood: {0}`, `Metal: {0}`

У Graph, для кожної кнопки On Clicked → **Get Player Controller → Cast to CrownsBanePlayerController → Buy Upgrade** (Category Byte: Hull=0, Sails=1, Weapons=2, CannonCount=3).

### 4. Показ віджета в доках

У `BP_Docks` → Event Graph → **Event On Player Enter Docks** → Create Widget (`WBP_UpgradeMenu`) → Add to Viewport.

---

## Діагностика (якщо щось не працює)

| Симптом                                 | Причина та рішення                                                                                  |
|-----------------------------------------|-----------------------------------------------------------------------------------------------------|
| На екрані червоний текст "ShipMappingContext NOT assigned" | Відкрий `BP_PlayerShip` → Details → Input → Ship Mapping Context = `IMC_Ship`               |
| На екрані: "EnhancedInput subsystem missing" | `Edit → Plugins → Enhanced Input` має бути ON (має бути вже увімкнено через `.uproject`)       |
| Controller=NONE в debug                  | `BP_PlayerShip` → Details → **Auto Possess Player** = Player 0 (має бути вже в C++ конструкторі)  |
| Sail змінюється але корабель не рухається | У Details **MaxSpeed** не 0 (має бути 1500). Перевір що **ShipMesh → Simulate Physics** = OFF     |
| Ядра не спавняться                       | CannonComponent → **Cannonball Class** = `BP_Cannonball`. Також перевір що є сокети на меші       |
| AI стоїть на місці                       | Build → Build Navigation. Перевір Nav Mesh Bounds Volume покриває воду                             |
| Апгрейди не працюють                     | GameMode спавнить UpgradeManager автоматично. Перевір Output Log — не має бути warnings           |
| Збереження не працює                     | `Config/DefaultEngine.ini` має `GameInstanceClass=/Script/CrownsBane.CrownsBaneGameInstance`       |

**Де дивитись логи:** `Window → Output Log` → фільтр "ShipPawn" — всі важливі події підписані `[ShipPawn]`.

---

## Безкоштовні ресурси

- **Fab.com** — "Pirate Ship" з фільтром "Free"
- **KayKit Ship Pack** на itch.io — низькополігональні кораблі
- **Quixel Megascans** — безкоштовно всередині UE5
- **Sketchfab** (фільтр CC0) — імпорт через FBX

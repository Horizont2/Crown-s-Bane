# Crown's Bane — Покроковий гайд налаштування в UE5

> **Час:** ~4-5 годин для першого запуску гри.  
> **Потрібно:** Unreal Engine 5.3 або новіший (безкоштовно через Epic Games Launcher).

---

## ДЕНЬ 1 — Створення проекту та рух корабля

### Крок 1: Встановлення UE5

1. Завантаж і встанови **Epic Games Launcher** з [unrealengine.com](https://www.unrealengine.com)
2. У Launcher → вкладка **Unreal Engine** → **Library** → кнопка **+** → встанови **UE 5.3**
3. Натисни **Launch** поруч з 5.3

### Крок 2: Створення проекту

1. У вікні вибору шаблону → **Games** → **Blank**
2. Тип проекту: **C++** (не Blueprint!)
3. Якість: **Scalable** (можна потім підвищити)
4. Raytracing: **вимкнено**
5. Назва проекту: **CrownsBane**
6. Шлях: вибери папку (НЕ ту саму що репозиторій)
7. Натисни **Create**
8. UE5 відкриє Visual Studio і скомпілює проект (~5 хвилин першого разу)

### Крок 3: Підключення нашого коду

1. Закрий Unreal Editor
2. Відкрий папку новоствореного проекту (наприклад `C:/Projects/CrownsBane/`)
3. Видали папку `Source/CrownsBane/` — вона порожня після шаблону
4. Скопіюй нашу папку `Source/CrownsBane/` з репозиторію в папку проекту
5. Скопіюй файл `CrownsBane.uproject` з репозиторію (замінить існуючий)
6. Клікни правою кнопкою на `CrownsBane.uproject` → **Generate Visual Studio project files**
7. Відкрий `.sln` файл у Visual Studio → **Build → Build Solution** (Ctrl+Shift+B)
8. Після успішної збірки — двічі клікни `CrownsBane.uproject`

### Крок 4: Налаштування Enhanced Input (обов'язково!)

1. **Edit → Project Settings → Input**
2. **Default Input Component Class** → вибери `EnhancedInputComponent`
3. **Default Player Input Class** → вибери `EnhancedPlayerInput`
4. Закрий Settings

### Крок 5: Створення Input Actions

У **Content Browser** (внизу) → правою кнопкою → **New Folder** → назви `Input`  
Зайди в папку `Input`.

Для кожного action: **правою кнопкою → Input → Input Action**

| Назва файлу     | Value Type | Призначення          |
|-----------------|------------|----------------------|
| `IA_IncreaseSail` | Digital (bool) | W — більше вітрила |
| `IA_DecreaseSail` | Digital (bool) | S — менше вітрила  |
| `IA_Turn`         | Axis1D (float) | A/D — поворот      |
| `IA_FireLeft`     | Digital (bool) | ЛКМ — лівий борт  |
| `IA_FireRight`    | Digital (bool) | ПКМ — правий борт |

### Крок 6: Створення Mapping Context

1. Правою кнопкою → **Input → Input Mapping Context** → назви `IMC_Ship`
2. Відкрий `IMC_Ship`
3. Натисни **+** для кожного action:

| Action           | Клавіша            | Modifiers           |
|------------------|--------------------|---------------------|
| `IA_IncreaseSail`| W                  | —                   |
| `IA_DecreaseSail`| S                  | —                   |
| `IA_Turn`        | D                  | —                   |
| `IA_Turn`        | A                  | Negate (інвертувати)|
| `IA_FireLeft`    | Left Mouse Button  | —                   |
| `IA_FireRight`   | Right Mouse Button | —                   |

4. Збережи (Ctrl+S)

### Крок 7: Підключення Ocean (Water Plugin)

1. **Edit → Plugins** → знайди **Water** → увімкни → Restart Editor
2. На панелі зліва (Place Actors) → **Water** → перетягни **Water Body Ocean** на сцену
3. Ocean налаштується автоматично — зелена кнопка **Build** якщо є помилки

### Крок 8: Створення Blueprint корабля гравця

1. Content Browser → Правою → **Blueprint Class** → шукай `ShipPawn` → вибери → `BP_PlayerShip`
2. Відкрий `BP_PlayerShip`
3. **Ліва панель** → клікни `ShipMesh (StaticMeshComponent)`
4. **Права панель → Details → Static Mesh** → вибери будь-який тимчасовий меш (наприклад `Shape_Cube`)
5. У **Details → Input** (прокрути вниз) → заповни:
   - `Ship Mapping Context` → `IMC_Ship`
   - `IA_IncreaseSail` → `IA_IncreaseSail`
   - `IA_DecreaseSail` → `IA_DecreaseSail`
   - `IA_Turn` → `IA_Turn`
   - `IA_FireLeft` → `IA_FireLeft`
   - `IA_FireRight` → `IA_FireRight`
6. **Compile** → **Save**

### Крок 9: Налаштування GameMode

1. Content Browser → Правою → **Blueprint Class** → `CrownsBaneGameMode` → `BP_GameMode`
2. Відкрий `BP_GameMode` → Details:
   - **Default Pawn Class** → `BP_PlayerShip`
   - **Player Controller Class** → `CrownsBanePlayerController`
   - **HUD Class** → `CrownsBaneHUD`
3. **Edit → Project Settings → Maps & Modes**:
   - **Default GameMode** → `BP_GameMode`
4. Compile & Save

### Крок 10: Налаштування GameInstance (для збереження)

1. **Edit → Project Settings → Maps & Modes**
2. **Game Instance Class** → вибери `CrownsBaneGameInstance`

### Крок 11: Розміщення на сцені та перший запуск

1. Перетягни `BP_PlayerShip` на сцену (над водою)
2. Додай **Player Start** (з Place Actors) — поруч з кораблем
3. Натисни **Play (зелена кнопка)** → W/S для вітрил, A/D для повороту!

---

## ДЕНЬ 2 — Гармати та стрільба

### Крок 12: Налаштування сокетів гармат на меші

Щоб гармати знали ЗВІДКИ стріляти, потрібні сокети на 3D-моделі:

1. Відкрий Static Mesh корабля (двічі клікни)
2. Вверху → **Window → Socket Manager**
3. Натисни **Create Socket** для кожної гармати:

```
CannonLeft_0   — лівий борт, перша гармата
CannonLeft_1   — лівий борт, друга гармата
CannonRight_0  — правий борт, перша гармата
CannonRight_1  — правий борт, друга гармата
```

4. Переміщуй сокети на потрібні позиції на моделі
5. Save

> **Порада:** Якщо тимчасово використовуєш куб — просто додай сокети по боках.

### Крок 13: Blueprint для Cannonball

1. Content → Правою → **Blueprint Class** → `Cannonball` → `BP_Cannonball`
2. Відкрий → клікни `ProjectileMesh` → Details:
   - Static Mesh → `Shape_Sphere` (тимчасово)
   - Scale → `0.2, 0.2, 0.2`
3. У `BP_PlayerShip` → Details → Cannon Component → **Cannonball Class** → `BP_Cannonball`
4. Compile & Save

---

## ДЕНЬ 3 — Вороги (AI)

### Крок 14: Створення ворожих Blueprints

Для кожного типу корабля:

1. Content → Правою → **Blueprint Class** → вибери відповідний C++ клас:
   - `SloopShip` → `BP_Sloop`
   - `BrigShip` → `BP_Brig`
   - `GalleonShip` → `BP_Galleon`

2. У кожному Blueprint → `ShipMesh`:
   - Встанови відповідний меш (або куб тимчасово)
   - Прикрути сокети гармат

3. Для кожного → `CannonComponent` → **Cannonball Class** → `BP_Cannonball`

### Крок 15: Navigation Mesh (потрібен для AI)

1. Вверху меню → **Build → Build Navigation** або натисни **P** щоб показати NavMesh
2. У **Place Actors** → **Volumes** → перетягни **Nav Mesh Bounds Volume** на сцену
3. Розтягни так, щоб покривав весь океан
4. Натисни **Build → Build Navigation**

### Крок 16: AI Controller

Кожен ворожий Blueprint автоматично використовує `ShipAIController` (вже прописано в C++ коді).  
Просто розміщуй `BP_Sloop`, `BP_Brig` на сцені — вони почнуть рухатись до гравця.

---

## ДЕНЬ 4 — Здоров'я та смерть

### Крок 17: Перевірка HP

`HealthComponent` вже підключено в C++. Щоб побачити HP:

1. Відкрий `BP_PlayerShip` → вибери `HealthComponent` → Details:
   - **Max Health** → 200 (або скільки хочеш)

При отриманні урону від ядра `HealthComponent` автоматично зменшує HP і кличе делегат `OnDeath`.

### Крок 18: Візуальний ефект смерті (Niagara)

1. Content → **Particle Systems** → правою → **Niagara System** → `NS_ShipExplosion`
2. Вибери шаблон **Fountain** і налаштуй (дим + вогонь)
3. У `EnemyShipBase.cpp` функція `HandleDeath()` — ми вже викликаємо `UGameplayStatics::SpawnEmitterAtLocation`
4. У C++ коді знайди і заміни `nullptr` на `NS_ShipExplosion`:

```cpp
// В EnemyShipBase.cpp, функція HandleDeath():
// Знайди рядок:
UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DeathEffect, GetActorLocation());
// DeathEffect — це UPROPERTY в EnemyShipBase.h, встанови його в Blueprint
```

У `BP_Sloop/BP_Brig/BP_Galleon` → Details → `DeathEffect` → `NS_ShipExplosion`

---

## ДЕНЬ 5 — Лут та ресурси

### Крок 19: Blueprint лута

1. Content → Правою → **Blueprint Class** → `LootPickup` → `BP_LootPickup`
2. Відкрий → `ResourceMesh` → Static Mesh → `Shape_Cylinder` (тимчасово)
3. Scale → `0.5, 0.5, 0.5`

### Крок 20: Налаштування LootSpawner

1. Content → Правою → **Blueprint Class** → `LootSpawner` → `BP_LootSpawner`
2. Відкрий → Details:
   - **Loot Pickup Class** → `BP_LootPickup`
3. Перетягни `BP_LootSpawner` на сцену (1 штука)

Тепер при знищенні ворожого корабля LootSpawner автоматично спавнить ресурси.

---

## ДЕНЬ 6 — Доки та апгрейди

### Крок 21: Створення Docks зони

1. Content → Правою → **Blueprint Class** → `DocksZone` → `BP_Docks`
2. Відкрий → вибери `BoxComponent` → Details → **Box Extent** → `1000, 1000, 500`
3. Перетягни на сцену — розміщуй у бухті (берег)

### Крок 22: UpgradeManager

1. Content → Правою → **Blueprint Class** → `UpgradeManager` → `BP_UpgradeManager`
2. Перетягни `BP_UpgradeManager` на сцену (1 штука, де завгодно)

### Крок 23: UI Upgrade Menu (UMG Widget)

1. Content → Правою → **User Interface → Widget Blueprint** → `WBP_UpgradeMenu`
2. Відкрий → у **Designer** створи простий UI:
   - 4 кнопки: "Корпус", "Вітрила", "Зброя", "Гармати"
   - Текст ресурсів: Золото / Дерево / Метал
3. У **Graph** для кожної кнопки **On Clicked**:
   ```
   Get Player Controller → Cast to CrownsBanePlayerController → Buy Upgrade (CategoryByte=0/1/2/3)
   ```
4. У `BP_GameMode` або в `OnEnterDocks` event — показуй `WBP_UpgradeMenu`

> **Підказка:** CategoryByte: Hull=0, Sails=1, Weapons=2, CannonCount=3

---

## ДЕНЬ 7 — Рівень розшуку, вітер, HUD

### Крок 24: WindSystem та WantedLevelManager

1. Content → Правою → **Blueprint Class** → `WindSystem` → `BP_WindSystem`
2. Content → Правою → **Blueprint Class** → `WantedLevelManager` → `BP_WantedLevelManager`
3. Content → Правою → **Blueprint Class** → `EnemySpawner` → `BP_EnemySpawner`
4. Перетягни всі три на сцену (по одному примірнику)

У `BP_EnemySpawner` → Details:
- **Sloop Class** → `BP_Sloop`
- **Brig Class** → `BP_Brig`
- **Galleon Class** → `BP_Galleon`

### Крок 25: HUD налаштування

`CrownsBaneHUD` вже підключено через `BP_GameMode`. HUD автоматично малює:
- HP bar (зелений, знизу зліва)
- Таймери перезарядки (ліво/право)
- Зірки розшуку (зверху)
- Ресурси: золото/дерево/метал
- Стрілка вітру

---

## ФІНАЛЬНИЙ ЧЕКЛИСТ

- [ ] W/S рухають корабель (з інерцією)
- [ ] A/D повертають корабель (ширший радіус на швидкості)
- [ ] ЛКМ/ПКМ стріляють гарматами з відповідного борту
- [ ] Вороги рухаються до гравця та стріляють у відповідь
- [ ] Знищений ворог залишає лут на воді
- [ ] Корабель підбирає лут, проплив крізь нього
- [ ] При в'їзді в Доки відкривається меню апгрейдів
- [ ] Апгрейди витрачають ресурси та покращують характеристики
- [ ] При 4+ зірках розшуку спавняться галеони-переслідувачі
- [ ] Прогрес зберігається між сесіями

---

## Корисні ресурси для 3D моделей (безкоштовно)

- **Fab.com** (колишній UE Marketplace) → шукай "Pirate Ship" → фільтр "Free"
- **Sketchfab** → безкоштовні моделі, імпортуй через FBX
- **KayKit Ship Pack** (itch.io) — низькополігональні кораблі для аркад, ідеально підходять

## Помилки та рішення

| Помилка | Рішення |
|---------|---------|
| `EnhancedInputComponent not found` | Edit → Project Settings → Input → Default Input Component = EnhancedInputComponent |
| Корабель не рухається | Переконайся що `IMC_Ship` призначено в `BP_PlayerShip` у полі `Ship Mapping Context` |
| Ядра не спавняться | У `BP_PlayerShip` → CannonComponent → Cannonball Class → вибери `BP_Cannonball` |
| AI не рухається | Переконайся що NavMesh покриває сцену (Build → Build Navigation) |
| Лут не підбирається | У `BP_PlayerShip` є Collision → Overlap → переконайся що `LootPickup` має OverlapEvents = true |
| Збереження не працює | Edit → Project Settings → Maps & Modes → Game Instance Class = CrownsBaneGameInstance |

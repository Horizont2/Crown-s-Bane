# Setting up new ship meshes

The game auto-adapts to any imported ship mesh — cannons compute spawn
positions from the mesh bounding box on `BeginPlay`. But a few properties
per mesh give a perfect fit.

## 1. Import FBX

1. Right-click in Content Browser → **Import to `/Content/Meshes/Ships/`**
2. Select your FBX. In the import dialog:
   - **Import Uniform Scale**: usually `1.0`. If the mesh looks tiny, try `10-100`.
   - **Combine Meshes**: ON (single mesh per ship)
   - **Auto Generate Collision**: ON (or Simple Box if it looks wrong)
   - **Import Materials**: ON if the FBX ships with them.

## 2. Assign to Blueprint

Open `BP_PlayerShip` / `BP_Sloop` / `BP_Brig` / `BP_Galleon`:

1. Select `ShipMesh` component
2. Details → **Static Mesh** → your imported mesh
3. Save. Play. The ship should fire from computed positions immediately.

## 3. Fine-tune (Details panel on BP)

If the ship sits above / below water, or cannons look wrong:

### Water floating (`AShipPawn`)

- **`WaterLineOffset`** (Category *Movement > Visual*) — cm to shift mesh Z.
  - Sits too high above waves → set negative (e.g. `-30`)
  - Sunk too deep → positive (e.g. `+40`)

### Cannon positions (`UCannonComponent`)

Fields in **Category *Cannon > Fallback*** are AUTO-COMPUTED from mesh
bounds at BeginPlay. Leave them at `0` for automatic, or override:

- **`FallbackShipHalfWidth`** — how far the cannons stand out from centre
  (default = mesh Y extent × 0.9).
- **`FallbackCannonSpacing`** — space between cannon slots along fore/aft
  (default = mesh X × 1.5 / (slots + 1)).
- **`FallbackCannonHeight`** — Z offset from ship centre to gunports
  (default = mesh Z × 0.55, i.e. deck-line).

## 4. Named sockets for perfect placement (optional)

For AAA-grade placement, add sockets manually:

1. Open your mesh in Static Mesh Editor
2. **Socket Manager** (Window → Socket Manager)
3. Add sockets named exactly:
   - `CannonLeft_0`, `CannonLeft_1`, ... `CannonLeft_7`
   - `CannonRight_0`, `CannonRight_1`, ... `CannonRight_7`
4. Position each socket at a gunport. Rotation aims the ball outward.
5. When any socket with these names exists, the code uses it instead of
   the auto-computed positions.

## 5. Common problems

| Problem | Fix |
|---------|-----|
| Ship sits above water | Reduce `WaterLineOffset` (e.g. `-40`) |
| Ship sunk in water | Increase `WaterLineOffset` (`+30`) |
| Nose dives on turn | Already clamped in code to ±3° pitch |
| Cannonballs stop instantly | Fixed: cannonball ignores owner ship collision |
| Cannonballs come from ship centre | Check auto-fallback values in log, or add named sockets |
| Mesh flickers with terrain | Set `Import Uniform Scale` correctly, re-import |

## 6. Recommended free ship packs

See `docs/FREE_SHIP_ASSETS.md` for a curated list of CC0/free packs.

# Spatial queries and tick profiling

`ObjectManager` owns the framework's broad phase. Gameplay asks for nearby
colliders through `queryBounds(min, max, candidates)` instead of traversing the
object registry. Boar support probes, character foot and uphill probes, and
`CollisionSystem` use this same query service.

## Query behavior

- Static collider bounds live in a persistent uniform grid with 128-unit cells.
  Bounds come from collision geometry, so offset shapes, polygons, compounds,
  and shapes spanning several cells are not limited to the tile artwork's cell.
- Dynamic objects are kept separately and checked at their current positions.
  Queries during movement and collision resolution therefore see live bounds.
- Results contain unique object pointers in the object registry's lexical name
  order. Only nearby candidates are sorted; querying does not scan the registry
  to reconstruct ordering.
- Unknown or unbounded shapes remain conservative candidates. Compound bounds
  are usable only when they encompass all members. Huge query regions fall back
  to a bounded scan rather than walking an arbitrarily large number of cells.
- The broad phase identifies candidates. Layer rules, one-way behavior, support
  selection, exact intersection tests, and collision response remain in their
  existing gameplay and physics code.

Object registration/removal and normal static geometry changes invalidate the
index. A shared static-geometry revision keeps managers independent of object
lifetime callbacks; editing static geometry in one world may consequently cause
another world's index to rebuild on its next query. Dynamic movement does not
increment this revision. This is intended for mostly static levels; a world with
many independently moving colliders would benefit from a dynamic spatial tree.
Dynamic updates use a snapshot: newly registered objects run on the next tick,
while objects removed or made static before their turn are skipped.

For edits made directly through a collider or renderable pointer, call
`object.refreshCollisionGeometry()` afterward. It discards the object's cloned
collider and invalidates static bounds. `world.invalidateSpatialIndex()` is the
bulk refresh operation for geometry changes that bypass the normal setters.
Raw writes to public fields cannot automatically notify the spatial index.

## Collision ordering

`CollisionSystem::update(ObjectManager&, dt)` uses the shared broad phase.
The map overload remains a brute-force reference for regression comparisons.
Dynamic pairs retain their existing order. Static candidates retain their
registry order; after a correction, the solver requeries the body's new bounds
and continues after the last processed static object. Earlier candidates are
considered again on the next solver iteration, just as with the original scan.
Contact queries include the existing touching tolerance. CCD substeps and the
narrow phase are unchanged.

## Measuring ticks

Set `AUTO_PROFILE=1` to collect inclusive region timings and candidate counts.
Collection uses fixed counters and clock samples, with no logging in individual
queries. The report is printed on exit. Nested timings overlap and must not be
added together. `ms_per_tick` uses the number of simulation ticks, which may
differ from rendered frames in deterministic mode.

From the repository root in PowerShell:

```powershell
$env:AUTO_PROFILE = '1'
$env:AUTO_BENCHMARK_SECONDS = '8'
try {
    & .\bin\framework2D_d.exe --dataPath .\bin\fantasySideScroller
} finally {
    Remove-Item Env:AUTO_PROFILE, Env:AUTO_BENCHMARK_SECONDS
}
```

The first update and a subsequent one-second warm-up are excluded. Keep the
build configuration, renderer, scene, replay, overlays, and VSync settings the
same between comparisons. Compare both time per tick and candidate counts:
counts distinguish algorithmic improvements from machine timing variation.

## Regression checks

```powershell
cmake --preset windows-debug -DFRAMEWORK_BUILD_TESTS=ON
cmake --build --preset windows-debug --parallel 8
ctest --test-dir build/cmake-windows-debug -C Debug --output-on-failure
```

Tests exercise local query results against a scan, geometry invalidation,
candidate scaling, collision behavior against the reference solver, and boar
behavior on real map assets. Assertions remain enabled in test executables.

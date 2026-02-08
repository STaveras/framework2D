# Fractured Earth — Living Design Doc

## 0. One‑sentence pitch
A survival/repair meta-hub set on a cracked planet where players loop through rapid mini-games that convert time/energy into specialized currencies, feeding long-term base upgrades, expeditions, and seasonal events.

## 1. Product goals
- **Session cadence:** 30–120s micro-sessions, plus 5–15m longer runs.
- **Retention:** layered timers (idle → tickets → dailies → weekly phases → seasons).
- **Clarity:** each mini-game has a single purpose: *convert resource A + time into currency B*.
- **Scalability:** add mini-games without touching core meta systems.

## 2. Core loop
1) Collect idle output in hub
2) Spend tickets/energy to enter a mini-game
3) Earn specialized currency (skill affects multiplier)
4) Spend currency on upgrades/unlocks
5) Unlock more mini-games + faster generation + new zones

## 3. Meta hub fantasy
Theme anchors (choose 1 primary, 1 secondary):
- Primary: **Planet Core Reactor** (repair fractures / stabilize core)
- Secondary: **Survivor Enclave** (population, warmth, morale)

Key hub screens:
- **Core** (main): idle collectors + “Play” entry
- **Workshop**: upgrades + crafting + conversions
- **Expeditions**: longer runs + map
- **Events**: rotating mini-games + limited rewards
- **Store** (optional): cosmetics + convenience

## 4. Currency system
### 4.1 Currency tiers
**Soft (high volume)**
- Scrap (S)
- Power (P)
- Food (F)
- Materials (M)

**Skill (mini-game earned, mid volume)**
- Stabilization Points (SP)
- Core Shards (CS)
- Repair Tokens (RT)

**Time-gated (tickets / stamina)**
- Reactor Charge (RC)
- Expedition Fuel (EF)

**Meta (slow, prestige)**
- Fracture Cores (FC)
- Relics (R)

### 4.2 Conversion philosophy
- Soft currencies are **broad** (many sinks).
- Skill currencies are **narrow** (few sinks) so they feel meaningful.
- Meta currencies gate **new systems** (zones, seasons, prestige paths).

### 4.3 Example sinks
- Scrap: building levels, repairs, common crafting
- Power: run mini-games, speed-ups, device upgrades
- Food: population growth, expedition provisioning
- SP/CS/RT: unlock mini-games, upgrade mini-game modifiers, event tracks
- FC/Relics: zone unlocks, global modifiers, prestige

## 5. Timers & gating
- Idle collector cap: **4–8 hours** (upgradeable)
- Ticket regen: **1 every 15–30 min** (varies per mode)
- Daily tasks: **reset 24h**
- Weekly phase: **3–7 days** (new fracture zone)
- Season: **28–35 days**

## 6. Mini-game portfolio
Design rule: each mini-game should have **(Input → Skill Test → Output)** and a “noob clear” path.

### 6.1 Fast (30–90s)
1) **Crack Seal (Timing)**: tap/hold on oscillating meter → **SP**
2) **Debris Dodge (Avoider)**: move through falling rocks → **RT**
3) **Circuit Link (Pipe/Connect)**: connect nodes under time → **CS**
4) **Thermal Vent (Rhythm)**: hit beats to vent heat → **Power + SP**

### 6.2 Mid (3–8m)
5) **Stabilize Grid (Match/Tile)**: clear unstable tiles → **SP + Materials**
6) **Drone Run (Course)**: collect samples, avoid hazards → **CS + Relics chance**

### 6.3 Long (10–20m)
7) **Expedition (Map Run)**: choose nodes (risk/reward) → **Relics + FC fragments**
8) **Wave Defense (TD-lite)**: defend relay → **FC fragments + RT**

## 7. Rewards, scoring, and fairness
- Every mini-game has:
  - **Base reward** (guaranteed)
  - **Score multiplier** (skill)
  - **Streak bonus** (optional, capped)
  - **RNG bonus roll** (only for small spice; never core currency)

Suggested reward formula:
- reward = base * (1 + scoreFactor * k) * (1 + upgradeBonus)

Anti-frustration:
- **Floor rewards** guarantee progress.
- **Soft fail**: losing early still grants partial reward.

## 8. Progression & unlocks
### 8.1 Early game (Day 0–3)
- 2 mini-games unlocked
- Teach loop via quests
- Unlock Workshop + first expedition

### 8.2 Mid game (Week 1–2)
- 5–6 mini-games
- First weekly fracture zone
- Introduce meta currency FC fragments

### 8.3 Late game
- Seasonal tracks
- Prestige / world phase resets
- Cosmetic collection + leaderboards (optional)

## 9. Content cadence
- Daily: quests + rotating “boosted” mini-game
- Weekly: fracture zone (new modifiers + limited loot)
- Seasonal: new mini-game variant + new cosmetic set

## 10. Monetization (optional, non-pay-to-win first)
- Cosmetics: skins, frames, icon borders
- Convenience: extra ticket cap, faster regen (soft), cosmetic pass
- Avoid selling direct meta currency; sell **time flexibility** at most.

## 11. Technical architecture targets for framework2D
Because the engine repo is referenced but not yet parsed here, this section is engine-agnostic and defines interfaces.

### 11.1 Game states
- BootState → TitleState → HubState
- HubState launches MiniGameState via a registry

### 11.2 Mini-game plugin contract
Each mini-game implements:
- `Init(context, seed, difficulty, modifiers)`
- `Update(dt)`
- `Render()`
- `OnInput(event)`
- `IsFinished()`
- `GetResult()` → { score, duration, rewards[] }

### 11.3 Economy service
Single source of truth:
- wallet balances
- regen timers
- transaction log
- deterministic simulation

### 11.4 Data-driven content
Store configs as JSON:
- currencies.json
- buildings.json
- minigames.json
- events.json
- quests.json

## 12. Data schemas (draft)
### 12.1 Currency definition
```json
{
  "id": "SP",
  "name": "Stabilization Points",
  "tier": "skill",
  "cap": null,
  "ui": {"icon": "sp.png", "color": "#..."}
}
```

### 12.2 Mini-game definition
```json
{
  "id": "crack_seal",
  "name": "Crack Seal",
  "ticket_cost": {"RC": 1},
  "base_rewards": [{"currency": "SP", "amount": 40}],
  "score_multiplier": {"k": 0.8, "cap": 2.5},
  "duration_sec": [30, 90],
  "modifiers": ["wind", "heat"],
  "unlocks": {"hub_level": 2}
}
```

### 12.3 Building / upgrade definition
```json
{
  "id": "core_reactor",
  "levels": [
    {"lvl": 1, "cost": [{"S": 100}], "effects": {"idle_P_per_hr": 20, "RC_cap": 6}},
    {"lvl": 2, "cost": [{"S": 250}, {"SP": 30}], "effects": {"idle_P_per_hr": 28, "RC_cap": 7}}
  ]
}
```

## 13. UI/UX notes
- Hub always shows: **ticket count**, **next regen**, **claimable idle**.
- Mini-games show: objective, timer, score, reward preview.
- Post-run: rewards + “upgrade available” callout.

## 14. Next implementation milestones
### Milestone A — Core skeleton
- HubState + Wallet + Timers
- MiniGameRegistry + 1 prototype mini-game
- JSON loading for currency + minigame defs

### Milestone B — Portfolio foundation
- Add 3 fast mini-games + unified result screen
- Upgrade screen consuming currencies
- Daily quest system

### Milestone C — Expeditions + weekly phase
- Map-run system + long-form mode
- Weekly zone modifiers + event schedule
- Save/load + transaction log

## 15. Open questions
- Primary platform first: macOS, iOS, or both?
- Input model: touch-first (iOS) vs mouse/keyboard-first (macOS)?
- Art direction: realistic vs stylized vs minimalist?
- Multiplayer/leaderboards: none, asynchronous, or competitive?


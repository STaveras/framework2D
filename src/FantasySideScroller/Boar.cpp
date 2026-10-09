#include "Boar.h"
#include "Character.h"
#include "Resources.h"
#include "../Animation.h"
#include "../AnimationUtils.h"
#include "../Kinematics2D.h"
#include "../ObjectManager.h"
#include "../Tile.h"
#include "../RuntimeProfile.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace {
constexpr float kFoot = 14.0f;
constexpr float kDetectionRange = 135.0f;
constexpr float kWallNormalThreshold = 0.55f;
constexpr float kInitialIdleSeconds = 2.0f;
constexpr float kPatrolPauseSeconds = 2.0f;
constexpr float kAggroReactionDelaySeconds = 0.25f;
constexpr float kAttackGap = 4.0f;
constexpr float kAttackReach = 24.0f;
constexpr float kAttackCooldownSeconds = 1.0f;
constexpr float kAttackPoseSeconds = 0.30f;
constexpr float kAttackDamage = 10.0f;
constexpr float kChaseSpeed = 100.0f;
constexpr float kPatrolSpeed = 28.0f;
constexpr unsigned int kSlashesToDefeat = 2;
constexpr unsigned int kHitsBeforeAggroReset = 3;
constexpr float kAggroDurationSeconds = 10.0f;
// The ground ahead may rise or fall by one step per probe hop; anything
// larger is a wall or a ledge.
constexpr float kStepUp = 5.0f;
constexpr float kStepDown = 8.0f;
constexpr float kProbeLead = 20.0f;
constexpr float kProbeHop = 4.0f;
constexpr float kProbeHopMin = 0.25f;
// A gap this narrow between neighbouring tile colliders is still ground.
constexpr float kSeamGap = 0.25f;
// Far enough past a tile's edge to sample its neighbour rather than itself.
constexpr float kNearSideOffset = 0.5f;
}

Boar::Boar(ObjectManager& world, Character& target, vector2 nearSpawn)
    : GameObject(GAME_OBJ_OBJECT), _world(world), _target(target), _spawn(nearSpawn)
{
    struct Clip { const char* name; const char* path; float duration; };
    const Clip clips[] = {
        {"Idle", "Mob/Boar/Idle/Idle-Sheet.png", 0.16f},
        {"Walk", "Mob/Boar/Walk/Walk-Base-Sheet.png", 0.12f},
        {"Run", "Mob/Boar/Run/Run-Sheet.png", 0.08f},
        {"Attack", "Mob/Boar/Hit-Vanish/Hit-Sheet.png", kAttackPoseSeconds}
    };
    for (const Clip& clip : clips) {
        auto* animation = _animationManager.create();
        auto* texture = Engine2D::getRenderer()->createTexture(BasePath(clip.path).c_str());
        const bool attack = std::strcmp(clip.name, "Attack") == 0;
        // The first Hit-Vanish frame is the boar's attack pose. The later
        // frames are the vanish effect, so they are deliberately not loaded.
        Animations::createFramesForAnimation(
            animation, texture, vector2(48, 32), _spriteManager, 0, attack ? 1 : 0);
        animation->setName(clip.name);
        animation->setMode(attack ? Animation::eOnce : Animation::eLoop);
        animation->center();
        for (auto* frame : animation->getFrames()) frame->setDuration(clip.duration);
        auto* state = addState(clip.name);
        state->setRenderable(animation);
        state->setCollidable(&_body);
    }
    setBuffered(false);
    // Land the initial spawn on actual terrain, including polygon slopes.
    float support = 0.0f;
    if (supportAt(_spawn.x, _spawn.y + kFoot, 32.0f, 320.0f, support))
        _spawn.y = support - kFoot;
    reset();
}

void Boar::gatherTerrain(vector2 min, vector2 max) const
{
    _supportCandidates.clear();
    _world.queryBounds(vector2(min.x - kSeamGap, min.y), vector2(max.x + kSeamGap, max.y), _supportCandidates);
    RuntimeProfile::count(RuntimeProfile::Counter::BoarCandidates, _supportCandidates.size());
    _terrain.clear();
    for (GameObject* object : _supportCandidates) {
        auto* tile = dynamic_cast<Tile*>(object);
        if (!tile || tile->isNonCollidingLayer()) continue;
        TerrainTile terrain{tile->getCollidable(), vector2(0.0f, 0.0f), vector2(0.0f, 0.0f)};
        // The spatial index can return whole grid cells.
        if (Kinematics2D::tryGetActiveBounds(terrain.collidable, terrain.min, terrain.max) &&
            terrain.max.x >= min.x - kSeamGap && terrain.min.x <= max.x + kSeamGap &&
            terrain.max.y >= min.y && terrain.min.y <= max.y) {
            _terrain.push_back(terrain);
        }
    }
}

bool Boar::surfaceAt(float x, float minY, float maxY, bool solidOnly, float& surface) const
{
    bool found = false;
    surface = std::numeric_limits<float>::max();
    for (const TerrainTile& terrain : _terrain) {
        // Authored tile polygons often stop a few hundredths of a pixel short
        // of the tile edge. Sample each tile at its nearest point within the
        // seam gap so a point between two tiles still finds the ground.
        if (x < terrain.min.x - kSeamGap || x > terrain.max.x + kSeamGap ||
            terrain.max.y < minY || terrain.min.y > maxY) {
            continue;
        }
        if (solidOnly && terrain.collidable->hasSurfaceFlag(SurfaceFlags::OneWay)) continue;
        const float sampleX = std::clamp(x, terrain.min.x, terrain.max.x);
        float y = 0.0f;
        if (Kinematics2D::sampleSupportY(terrain.collidable, sampleX, y) && y >= minY && y <= maxY && y < surface) {
            surface = y;
            found = true;
        }
    }
    return found;
}

bool Boar::supportAt(float x, float footY, float above, float below, float& support) const
{
    RuntimeProfile::Scope profileScope(RuntimeProfile::Region::BoarSupport);
    gatherTerrain(vector2(x, footY - above), vector2(x, footY + below));
    return surfaceAt(x, footY - above, footY + below, false, support);
}

bool Boar::isFloor(const Collidable* terrain) const
{
    // Terrain no more than a step above the feet where it is nearest the
    // boar is ground to walk on. Behind the boar that step is a step down:
    // the body still overlaps a ledge it has just walked off.
    vector2 min, max;
    float surface = 0.0f;
    const vector2 position = getPosition();
    const float nearestX = Kinematics2D::tryGetActiveBounds(terrain, min, max)
        ? std::clamp(position.x, min.x, max.x)
        : position.x;
    if (!Kinematics2D::sampleSupportY(terrain, nearestX, surface)) return false;
    const float heading = getVelocity().x != 0.0f ? getVelocity().x : static_cast<float>(_direction);
    const float step = (nearestX - position.x) * heading < 0.0f ? kStepDown : kStepUp;
    if (surface >= position.y + kFoot - step) return true;
    // Further up a ramp, a tile can sit higher than that. It is still ground
    // when the surface just short of its near edge leads up to it; a wall
    // face rises more than a step above whatever is in front of it.
    if (nearestX == position.x) return false;
    const float nearSideX = nearestX + (nearestX < position.x ? kNearSideOffset : -kNearSideOffset);
    float nearSide = 0.0f;
    return supportAt(nearSideX, surface, kStepUp, step, nearSide);
}

bool Boar::followGround(float fromX, float toX, float& surface) const
{
    // Ground can continue underneath a short block, so a solid top more than
    // a step up, within the body's height, blocks the way.
    float next = 0.0f;
    float top = 0.0f;
    if (surfaceAt(toX, surface - kStepUp, surface + kStepDown, false, next) &&
        !(surfaceAt(toX, next - _body.getHeight(), next, true, top) && top < next - kStepUp)) {
        surface = next;
        return true;
    }
    // A step where a ramp begins plus the ramp's own rise can exceed a step
    // within one hop. Split the hop until the step stands alone.
    if (std::fabs(toX - fromX) <= kProbeHopMin) return false;
    const float midX = (fromX + toX) * 0.5f;
    return followGround(fromX, midX, surface) && followGround(midX, toX, surface);
}

bool Boar::groundContinues(float x, float footY, int direction, float distance) const
{
    // Follow the surface in short hops. A single sample at the full distance
    // reads a ramp as a wall going up and as a ledge going down.
    RuntimeProfile::Scope profileScope(RuntimeProfile::Region::BoarSupport);
    // One query covers every hop: each may climb a step, with the body's
    // height above it, or drop one.
    const float hops = std::ceil(distance / kProbeHop);
    const float endX = x + direction * distance;
    gatherTerrain(vector2(std::min(x, endX), footY - hops * kStepUp - _body.getHeight()),
                  vector2(std::max(x, endX), footY + hops * kStepDown));
    float surface = footY;
    float fromX = x;
    for (float travelled = 0.0f; travelled < distance;) {
        travelled = std::min(distance, travelled + kProbeHop);
        const float hopX = x + direction * travelled;
        if (!followGround(fromX, hopX, surface)) return false;
        fromX = hopX;
    }
    return true;
}

void Boar::animate(const char* name)
{
    if (getState() != getState(name)) setState(name);
    auto* animation = static_cast<Animation*>(getRenderable());
    // Original art faces left.
    const bool mirrored = animation->getScale().x < 0.0f;
    if (mirrored != (_direction > 0)) animation->mirror(true, false);
}

void Boar::clearAggro()
{
    _aggro = false;
    _aggroRemaining = 0.0f;
    _hitsLanded = 0;
}

void Boar::reset()
{
    clearEvents();
    _direction = -1;
    _pause = kInitialIdleSeconds;
    _attackCooldown = 0.0f;
    _attackPoseTime = 0.0f;
    _damageFlash.stop(*this);
    _lastCountedPlayerAttack = nullptr;
    _slashesTaken = 0;
    clearAggro();
    _turnAfterPause = false;
    _holdingAttackPosition = false;
    _wasChasing = false;
    _canAttack = false;
    _defeated = false;
    setVelocity(vector2(0, 0));
    setState("Idle");
    animate("Idle");
    getRenderable()->setVisibility(true);
    setPosition(_spawn);
}

void Boar::handleCollisionContact(const CollisionContact& contact)
{
    if (_defeated || contact.phase == CollisionPhase::Exit || !contact.other ||
        contact.other->getType() != GAME_OBJ_TILE || !contact.normal.has_value()) {
        return;
    }

    const vector2 normal = contact.normal.value();
    const float absNormalX = std::fabs(normal.x);
    const float absNormalY = std::fabs(normal.y);
    // A floor or slope has a predominantly vertical normal. A wall has a
    // predominantly horizontal normal, and the normal points toward the
    // tile from the boar, so its sign matches the direction being pushed.
    if (absNormalX <= kWallNormalThreshold || absNormalX <= absNormalY ||
        normal.x * static_cast<float>(_direction) <= 0.0f) {
        return;
    }

    // A floor tile at a seam reports a side normal when the body's leading
    // edge stops on the boundary.
    if (isFloor(contact.otherCollidable)) return;

    // Stay contacts repeat each frame while the boar is pressed against a
    // wall. Only the first contact should start the wait; otherwise the timer
    // never reaches the delayed turn.
    if (_turnAfterPause) return;

    // Stop at the wall, wait two seconds, turn, then wait another two
    // seconds before starting the next patrol leg.
    _pause = std::max(_pause, kPatrolPauseSeconds);
    _turnAfterPause = true;
    setVelocity(vector2(0.0f, getVelocity().y));

    // Collision contacts are dispatched after movement. Enter the idle pose
    // immediately so the stop is visible during this same frame.
    animate("Idle");
}

bool Boar::shouldCollideWith(const GameObject& other) const
{
    // Contact notifications stay terrain-only; combat has its own sword/body overlap.
    return !_defeated && other.getType() == GAME_OBJ_TILE;
}

bool Boar::shouldResolvePhysicsWith(const GameObject& other) const
{
	// A living boar is a solid dynamic body. Keep the existing terrain filters,
	// but let the shared solver separate it from other dynamic bodies as well.
	// update() places the feet on the ground itself. Lifting the body onto a
	// ramp's uphill side or a step corner would leave it hanging in the air.
	if (_defeated) return false;
	if (other.getType() == GAME_OBJ_TILE) {
		// Decoration and terrain clear of the body are never resolved; skip
		// the floor test for them.
		if (static_cast<const Tile&>(other).isNonCollidingLayer()) return false;
		const Collidable* terrain = const_cast<GameObject&>(other).getCollidable();
		vector2 bodyMin, bodyMax, terrainMin, terrainMax;
		if (Kinematics2D::tryGetActiveBounds(const_cast<Boar*>(this)->getCollidable(), bodyMin, bodyMax) &&
			Kinematics2D::tryGetActiveBounds(terrain, terrainMin, terrainMax) &&
			terrainMin.x <= bodyMax.x && terrainMax.x >= bodyMin.x &&
			terrainMin.y <= bodyMax.y && terrainMax.y >= bodyMin.y &&
			isFloor(terrain)) {
			return false;
		}
	}
	return GameObject::shouldResolvePhysicsWith(other);
}

void Boar::update(float time)
{
    RuntimeProfile::Scope profileScope(RuntimeProfile::Region::BoarUpdate);
    if (_defeated) {
        return;
    }
    _damageFlash.update(*this, time);
    const vector2 position = getPosition();
    _attackPoseTime = std::max(0.0f, _attackPoseTime - time);
    _canAttack = false;

    if (_aggro) {
        _aggroRemaining -= time;
        if (_aggroRemaining <= 0.0f) clearAggro();
    }

    _pause = std::max(0.0f, _pause - time);
    if (_pause <= 0.0f && _turnAfterPause) {
        _direction = -_direction;
        _turnAfterPause = false;
        _pause = kPatrolPauseSeconds;
        _wasChasing = false;
    }

    // Climb any step the probe ahead accepted, on top of a ramp's rise over
    // the last frame's movement. The solver leaves floors to the boar, so
    // also reach back over the last frame's fall to land.
    float ground = 0.0f;
    const vector2 moved = getVelocity() * time;
    const float reach = kStepUp + std::fabs(moved.x) + std::max(0.0f, moved.y);
    const bool grounded = supportAt(position.x, position.y + kFoot, reach, 4.0f, ground);
    const vector2 delta = _target.getPosition() - position;
    const bool targetEligible = _target.getHealth() > 0.0f &&
        std::fabs(delta.x) < kDetectionRange && std::fabs(delta.y) < 40.0f;
    if (_aggro && targetEligible && delta.x != 0.0f) {
        const int targetDirection = delta.x > 0.0f ? 1 : -1;
        if (_direction != targetDirection) {
            _direction = targetDirection;
            _holdingAttackPosition = false;
            animate(getState()->getName());
        }
    }
    const bool targetInFront = delta.x * static_cast<float>(_direction) > 0.0f;
    const bool chase = targetEligible && (_aggro || targetInFront);
    if (chase && !_wasChasing && !_turnAfterPause) {
        _pause = std::min(_pause, kAggroReactionDelaySeconds);
    }
    _wasChasing = chase;
    if (_holdingAttackPosition && !chase) _holdingAttackPosition = false;

    float speed = 0.0f;
    int moveDirection = _direction;
    bool atAttackDistance = false;
    if (chase) {
        vector2 heroMin, heroMax, boarMin, boarMax;
        if (Kinematics2D::tryGetActiveBounds(_target.getCollidable(), heroMin, heroMax) &&
            Kinematics2D::tryGetActiveBounds(getCollidable(), boarMin, boarMax)) {
            // Stop with a small gap between the boar's leading body edge and
            // the player's active character hitbox.
            const float playerEdge = _direction > 0 ? heroMin.x : heroMax.x;
            const float boarFrontExtent = _direction > 0
                ? boarMax.x - position.x
                : position.x - boarMin.x;
            const float stopX = playerEdge - _direction * (kAttackGap + boarFrontExtent);
            const float remaining = (stopX - position.x) * static_cast<float>(_direction);
            atAttackDistance = std::fabs(remaining) <= 0.5f;

            if (atAttackDistance) {
                _holdingAttackPosition = true;
            } else if (std::fabs(remaining) > kAttackReach) {
                // Target left the held position beyond attack reach: drop the
                // hold so the boar repositions and re-engages instead of
                // staying planted while it walks away.
                _holdingAttackPosition = false;
            }
            if (_holdingAttackPosition) {
                const bool verticallyOverlapping = heroMin.y < boarMax.y && heroMax.y > boarMin.y;
                _canAttack = grounded && verticallyOverlapping && std::fabs(remaining) <= kAttackReach;
            } else {
                _canAttack = grounded && atAttackDistance;
            }

            // Once at the attack spot, stay planted through the whole aggro
            // exchange instead of following small movements between attacks.
            if (!_holdingAttackPosition && _pause <= 0.0f && grounded &&
                _attackPoseTime <= 0.0f && !atAttackDistance) {
                moveDirection = remaining > 0.0f ? _direction : -_direction;
                speed = std::min(kChaseSpeed, std::fabs(remaining) / std::max(time, 0.001f));
            }
        }
    }

    if (!chase && _pause <= 0.0f && grounded && _attackPoseTime <= 0.0f) {
        speed = kPatrolSpeed;
    }
    if (speed > 0.0f && grounded) {
        const bool ledgeOrWall = !groundContinues(position.x, ground, moveDirection, kProbeLead + speed * time);
        if (ledgeOrWall) {
            _pause = kPatrolPauseSeconds;
            _turnAfterPause = true;
            speed = 0.0f;
        }
    }

    const char* animation = _attackPoseTime > 0.0f
        ? "Attack"
        : (speed == 0.0f ? "Idle" : (chase ? "Run" : "Walk"));
    animate(animation);
    setVelocity(vector2(speed * moveDirection, grounded ? 0.0f : std::min(350.0f, getVelocity().y + 900.0f * time)));
    if (grounded) setPosition(position.x, ground - kFoot);
    GameObject::update(time);
}

void Boar::updateCombat(float time)
{
    _attackCooldown = std::max(0.0f, _attackCooldown - time);
    if (_defeated) return;
    vector2 heroMin, heroMax, boarMin, boarMax;
    if (!Kinematics2D::tryGetActiveBounds(_target.getCollidable(), heroMin, heroMax) ||
        !Kinematics2D::tryGetActiveBounds(getCollidable(), boarMin, boarMax)) return;
    const char* state = _target.getState()->getName();
    const bool attacking = std::strcmp(state, "Attack01") == 0 || std::strcmp(state, "Attack02") == 0;
    bool swordStrikeActive = false;
    if (attacking) {
        auto* animation = static_cast<Animation*>(_target.getRenderable());
        // First frame is anticipation; only the forward sword reach deals damage.
        swordStrikeActive = animation->getCurrentFrameIndex() > 0;
        if (swordStrikeActive) {
            if (animation->getScale().x < 0.0f) heroMin.x -= 32.0f;
            else heroMax.x += 32.0f;
        }
    } else {
        // Leaving an attack state arms the next slash, including a later
        // Attack01 animation after the player returns to locomotion.
        _lastCountedPlayerAttack = nullptr;
    }
    const bool overlaps = heroMin.x < boarMax.x && heroMax.x > boarMin.x &&
        heroMin.y < boarMax.y && heroMax.y > boarMin.y;
    const bool newSlash = attacking && swordStrikeActive && overlaps &&
        (!_lastCountedPlayerAttack || std::strcmp(_lastCountedPlayerAttack, state) != 0);
    if (_target.getHealth() > 0.0f && newSlash) {
        _lastCountedPlayerAttack = state;
        ++_slashesTaken;
        if (_slashesTaken >= kSlashesToDefeat) {
            // The boar has no separate death clip. The Hit-Vanish sheet is
            // only the single-frame attack cue, so the second slash removes it.
            _defeated = true;
            _canAttack = false;
            setVelocity(vector2(0, 0));
            getRenderable()->setVisibility(false);
            return;
        }
        _damageFlash.start(*this);
        _aggro = true;
        _aggroRemaining = kAggroDurationSeconds;
    }

    if (_target.getHealth() <= 0.0f || !_canAttack || _attackCooldown > 0.0f) return;
    _target.addHealth(-kAttackDamage);
    _attackCooldown = kAttackCooldownSeconds;
    _attackPoseTime = kAttackPoseSeconds;
    animate("Attack");
    // After a few landed hits, or once aggro times out in update(), the boar
    // calms down: it stops tracking the player, so stepping behind it no
    // longer provokes a turn and chase.
    if (++_hitsLanded >= kHitsBeforeAggroReset) clearAggro();
    if (_target.getHealth() <= 0.0f) {
        _target.setState("Dead");
    }
}

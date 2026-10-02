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
constexpr float kDamageFlashDuration = 0.64f;
constexpr float kDamageFlashHalfPeriod = 0.08f;
constexpr float kDamageFlashOpacity = 0.85f;
constexpr unsigned int kSlashesToDefeat = 2;
constexpr unsigned int kHitsBeforeAggroReset = 3;
constexpr float kAggroDurationSeconds = 10.0f;
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

bool Boar::supportAt(float x, float footY, float above, float below, float& support) const
{
    RuntimeProfile::Scope profileScope(RuntimeProfile::Region::BoarSupport);
    bool found = false;
    support = std::numeric_limits<float>::max();
    // sampleSupportY accepts points on polygon edges within a small epsilon;
    // include that tolerance in the query bounds so the spatial index cannot
    // drop an edge-touching candidate.
    constexpr float kSampleQueryEpsilon = 0.001f;
    const vector2 queryMin(x - kSampleQueryEpsilon, footY - above);
    const vector2 queryMax(x + kSampleQueryEpsilon, footY + below);
    _supportCandidates.clear();
    _world.queryBounds(queryMin, queryMax, _supportCandidates);
    RuntimeProfile::count(RuntimeProfile::Counter::BoarCandidates, _supportCandidates.size());
    for (GameObject* object : _supportCandidates) {
        auto* tile = dynamic_cast<Tile*>(object);
        if (!tile || tile->isNonCollidingLayer()) continue;
        float y = 0.0f;
        if (Kinematics2D::sampleSupportY(tile->getCollidable(), x, y) &&
            y >= footY - above && y <= footY + below && y < support) {
            support = y;
            found = true;
        }
    }
    return found;
}

void Boar::animate(const char* name)
{
    if (getState() != getState(name)) setState(name);
    auto* animation = static_cast<Animation*>(getRenderable());
    // Original art faces left.
    const bool mirrored = animation->getScale().x < 0.0f;
    if (mirrored != (_direction > 0)) animation->mirror(true, false);
}

void Boar::setDamageFlash(bool enabled)
{
    for (auto stateIt = begin(); stateIt != end(); ++stateIt) {
        GameObjectState* state = static_cast<GameObjectState*>(*stateIt);
        if (Renderable* renderable = state->getRenderable()) {
            renderable->setFlash(enabled, 0xFFFF5555, kDamageFlashOpacity);
        }
    }
}

void Boar::startDamageFlash()
{
    _damageFlashRemaining = kDamageFlashDuration;
    _damageFlashPhase = kDamageFlashHalfPeriod;
    _damageFlashOn = true;
    setDamageFlash(true);
}

void Boar::updateDamageFlash(float time)
{
    if (_damageFlashRemaining <= 0.0f) return;

    _damageFlashRemaining = std::max(0.0f, _damageFlashRemaining - time);
    _damageFlashPhase -= time;
    while (_damageFlashPhase <= 0.0f && _damageFlashRemaining > 0.0f) {
        _damageFlashOn = !_damageFlashOn;
        _damageFlashPhase += kDamageFlashHalfPeriod;
    }
    if (_damageFlashRemaining <= 0.0f) {
        _damageFlashOn = false;
        setDamageFlash(false);
    } else {
        setDamageFlash(_damageFlashOn);
    }
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
    _damageFlashRemaining = 0.0f;
    _damageFlashPhase = 0.0f;
    _damageFlashOn = false;
    setDamageFlash(false);
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
	return !_defeated && GameObject::shouldResolvePhysicsWith(other);
}

void Boar::update(float time)
{
    RuntimeProfile::Scope profileScope(RuntimeProfile::Region::BoarUpdate);
    if (_defeated) {
        return;
    }
    updateDamageFlash(time);
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

    float ground = 0.0f;
    const bool grounded = supportAt(position.x, position.y + kFoot, 3.0f, 4.0f, ground);
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
        float ahead = 0.0f;
        const float probeX = position.x + moveDirection * (20.0f + speed * time);
        const bool ledgeOrWall = !supportAt(probeX, position.y + kFoot, 5.0f, 8.0f, ahead);
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
        startDamageFlash();
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

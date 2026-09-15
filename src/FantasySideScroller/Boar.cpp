#include "Boar.h"
#include "Character.h"
#include "Resources.h"
#include "../Animation.h"
#include "../AnimationUtils.h"
#include "../Kinematics2D.h"
#include "../ObjectManager.h"
#include "../Tile.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace {
constexpr float kFoot = 14.0f;
constexpr float kPatrolRadius = 72.0f;
constexpr float kDetectionRange = 135.0f;
}

Boar::Boar(ObjectManager& world, Character& target, vector2 nearSpawn)
    : GameObject(GAME_OBJ_OBJECT), _world(world), _target(target), _spawn(nearSpawn)
{
    struct Clip { const char* name; const char* path; float duration; };
    const Clip clips[] = {
        {"Idle", "Mob/Boar/Idle/Idle-Sheet.png", 0.16f},
        {"Walk", "Mob/Boar/Walk/Walk-Base-Sheet.png", 0.12f},
        {"Run", "Mob/Boar/Run/Run-Sheet.png", 0.08f},
        {"Hit", "Mob/Boar/Hit-Vanish/Hit-Sheet.png", 0.10f}
    };
    for (const Clip& clip : clips) {
        auto* animation = _animationManager.create();
        auto* texture = Engine2D::getRenderer()->createTexture(BasePath(clip.path).c_str());
        Animations::createFramesForAnimation(animation, texture, vector2(48, 32), _spriteManager);
        animation->setName(clip.name);
        animation->setMode(std::strcmp(clip.name, "Hit") == 0 ? Animation::eOnce : Animation::eLoop);
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
    bool found = false;
    support = std::numeric_limits<float>::max();
    for (const auto& entry : _world.getObjects()) {
        auto* tile = dynamic_cast<Tile*>(entry.second);
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

void Boar::reset()
{
    clearEvents();
    _direction = -1;
    _pause = 0.6f;
    _damageCooldown = 0.0f;
    _defeated = false;
    setVelocity(vector2(0, 0));
    setState("Idle");
    animate("Idle");
    getRenderable()->setVisibility(true);
    setPosition(_spawn);
}

bool Boar::shouldCollideWith(const GameObject& other) const
{
    // Combat uses a separate sword/body overlap so attacks retain terrain collision.
    return !_defeated && other.getType() == GAME_OBJ_TILE;
}

void Boar::update(float time)
{
    if (_defeated) {
        GameObject::update(time);
        if (!static_cast<Animation*>(getRenderable())->isPlaying())
            getRenderable()->setVisibility(false);
        return;
    }
    const vector2 position = getPosition();
    float ground = 0.0f;
    const bool grounded = supportAt(position.x, position.y + kFoot, 3.0f, 4.0f, ground);
    const vector2 delta = _target.getPosition() - position;
    const bool chase = _target.getHealth() > 0.0f && std::fabs(delta.x) < kDetectionRange &&
        std::fabs(delta.y) < 40.0f;
    _pause = std::max(0.0f, _pause - time);
    float speed = 0.0f;
    if (_pause <= 0.0f && grounded) {
        if (chase) _direction = delta.x < 0.0f ? -1 : 1;
        speed = chase ? 100.0f : 28.0f;
        float ahead = 0.0f;
        const float probeX = position.x + _direction * (20.0f + speed * time);
        const bool ledgeOrWall = !supportAt(probeX, position.y + kFoot, 5.0f, 8.0f, ahead);
        const bool patrolEnd = !chase && (position.x - _spawn.x) * _direction >= kPatrolRadius;
        if (ledgeOrWall || patrolEnd) {
            _direction = -_direction;
            _pause = 0.7f;
            speed = 0.0f;
        }
    }
    animate(speed == 0.0f ? "Idle" : (chase ? "Run" : "Walk"));
    setVelocity(vector2(speed * _direction, grounded ? 0.0f : std::min(350.0f, getVelocity().y + 900.0f * time)));
    if (grounded) setPosition(position.x, ground - kFoot);
    GameObject::update(time);
}

void Boar::updateCombat(float time)
{
    _damageCooldown = std::max(0.0f, _damageCooldown - time);
    if (_defeated || _target.getHealth() <= 0.0f) return;
    vector2 heroMin, heroMax, boarMin, boarMax;
    if (!Kinematics2D::tryGetActiveBounds(_target.getCollidable(), heroMin, heroMax) ||
        !Kinematics2D::tryGetActiveBounds(getCollidable(), boarMin, boarMax)) return;
    const char* state = _target.getState()->getName();
    const bool attacking = std::strcmp(state, "Attack01") == 0 || std::strcmp(state, "Attack02") == 0;
    if (attacking) {
        auto* animation = static_cast<Animation*>(_target.getRenderable());
        // First frame is anticipation; only the forward sword reach deals damage.
        if (animation->getCurrentFrameIndex() == 0) return;
        if (animation->getScale().x < 0.0f) heroMin.x -= 32.0f;
        else heroMax.x += 32.0f;
    }
    const bool overlaps = heroMin.x < boarMax.x && heroMax.x > boarMin.x &&
        heroMin.y < boarMax.y && heroMax.y > boarMin.y;
    if (!overlaps) return;
    if (attacking) {
        _defeated = true;
        setVelocity(vector2(0, 0));
        animate("Hit");
    } else if (_damageCooldown <= 0.0f) {
        _target.addHealth(-15.0f);
        _damageCooldown = 1.0f;
        _pause = 0.5f;
        if (_target.getHealth() <= 0.0f) _target.setState("Dead");
    }
}

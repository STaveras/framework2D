// Enemy.cpp - Implementation of enemy class

#include "Enemy.h"
#include "../Camera.h"
#include "../GameState.h"
#include "../Font.h"
#include "../Debug.h"
#include "../Game.h"

#include <cstring>

// Default enemy stats by type (static because it's accessed from non-static methods)
const Enemy::Stats Enemy::_defaultStats[] = {
    // BOAR - Melee, fast, low HP
    {100.0f, 100.0f, 5.0f, 120.0f, 200.0f, 150.0f, 3.0f},
    
    // BEE - Flying, swarm
    {50.0f, 50.0f, 2.0f, 180.0f, 250.0f, 180.0f, 2.0f},
    
    // SNAIL - Slow, tanky
    {200.0f, 200.0f, 8.0f, 60.0f, 150.0f, 150.0f, 5.0f},
    
    // SKELETON - Fast, hit-and-run
    {80.0f, 80.0f, 6.0f, 140.0f, 220.0f, 160.0f, 2.5f},
    
    // ZOMBIE - Tanky, slow
    {150.0f, 150.0f, 10.0f, 70.0f, 180.0f, 180.0f, 4.0f},
    
    // BAT - Flying, swarm
    {40.0f, 40.0f, 1.5f, 190.0f, 280.0f, 200.0f, 1.5f},
};

Enemy::Enemy(void)
    : Character()
    , _health(0.0f)
    , _maxHealth(0.0f)
    , _damage(0.0f)
    , _isDead(false)
    , _isInvulnerable(false)
    , _invulnerabilityTimer(0.0f)
    , _isPatrolling(true)
    , _patrolRadius(100.0f)
    , _patrolAngle(0.0f)
    , _isAggroed(false)
    , _lastHitTime(0.0f)
    , _hitCooldown(1.0f)
    , _aiState(AIState::PATROL)
    , _deathTimer(0.0f)
    , _shouldRespawn(true)
{
    // _objType is set by the base class constructor
}

Enemy::~Enemy(void)
{
}

Enemy* Enemy::create(Type type, const vector2& position)
{
    Enemy* enemy = new Enemy();
    enemy->initialize(type, position);
    return enemy;
}

void Enemy::initialize(Type type, const vector2& startPos)
{
    // Set stats based on type
    const Stats& stats = _defaultStats[(int)type];
    _maxHealth = stats.maxHealth;
    _health = _maxHealth;
    _damage = stats.damage;
    _speed = stats.speed;
    _detectionRange = stats.detectionRange;
    _aggroRange = stats.aggroRange;
    _respawnTime = stats.respawnTime;
    
    // Set starting position
    setPosition(startPos);
    _patrolPosition = startPos;
    
    // Initialize animation states
    _initStates();
    _initTransitions();
    
    // Start in patrol mode
    _isPatrolling = true;
    _aiState = AIState::PATROL;
    _isAggroed = false;
    _target = nullptr;
    
    // Set initial velocity to 0
    setVelocity(vector2(0, 0));
}

void Enemy::reset(void)
{
    _health = _maxHealth;
    _isDead = false;
    _isInvulnerable = false;
    _invulnerabilityTimer = 0.0f;
    _isAggroed = false;
    _target = nullptr;
    _lastHitTime = 0.0f;
    _aiState = AIState::PATROL;
    setPosition(_patrolPosition);
    setVelocity(vector2(0, 0));
}

void Enemy::update(float time)
{
    Character::update(time);  // Call base class update for movement/physics
    
    if (_isDead)
    {
        // Handle death (respawn timer)
        updateDeath(time);
    }
    else
    {
        // Update AI
        updateAI(time);
        
        // Update combat state
        updateCombat(time);
        
        // Update invulnerability timer
        if (_isInvulnerable)
        {
            _invulnerabilityTimer -= time;
            if (_invulnerabilityTimer <= 0.0f)
            {
                _isInvulnerable = false;
            }
        }
    }
}

void Enemy::updateAI(float time)
{
    if (!_isAggroed || _isPatrolling)
    {
        updatePatrol(time);
    }
    else if (_aiState == AIState::CHASE)
    {
        updateChase(time);
    }
    else if (_aiState == AIState::ATTACK)
    {
        updateAttack(time);
    }
    else if (_aiState == AIState::FLEE)
    {
        updateFlee(time);
    }
    else if (_aiState == AIState::WAIT)
    {
        updateWait(time);
    }
}

void Enemy::updateCombat(float time)
{
    if (!_isAggroed || _target == nullptr)
    {
        return;
    }
    
    // Check if we can attack
    if (_aiState != AIState::ATTACK && canAttack())
    {
        // Start attacking
        attack(_target);
        _aiState = AIState::ATTACK;
    }
}

void Enemy::takeDamage(float amount, GameObject* attacker)
{
    if (_isDead || _isInvulnerable)
    {
        return;
    }
    
    // Check hit cooldown
    float currentTime = Engine2D::getTimer()->getElapsedTime();
    if (currentTime - _lastHitTime < _hitCooldown)
    {
        return;
    }
    
    _lastHitTime = currentTime;
    _health -= amount;
    
    // Visual feedback (flash white)
    _isInvulnerable = true;
    _invulnerabilityTimer = 0.5f;
    
    // Death check
    if (_health <= 0.0f)
    {
        die();
    }
}

void Enemy::heal(float amount)
{
    if (_isDead)
    {
        return;
    }
    
    _health = std::min(_health + amount, _maxHealth);
}

void Enemy::setAggro(bool aggro)
{
    _isAggroed = aggro;
    // When aggro is enabled, enemy will detect and chase player automatically
}

void Enemy::setTarget(GameObject* target)
{
    _target = target;
}

void Enemy::setPatrolMode(bool enable)
{
    _isPatrolling = enable;
    if (!enable)
    {
        _aiState = AIState::CHASE;
    }
}

void Enemy::attack(GameObject* target)
{
    // Base attack implementation (can be overridden by enemy types)
    // For now, just move toward target
    if (target)
    {
        vector2 toTarget = target->getPosition() - getPosition();
        float distance = toTarget.length();
        
        if (distance > 20.0f)  // Attack range
        {
            // Move toward target
            vector2 direction = toTarget.normalize();
            moveTo(getPosition() + direction * _speed * 0.1f, _speed);
        }
        else
        {
            // In attack range - could add attack animation here
            // For now, just stop and wait
        }
    }
}

bool Enemy::canAttack() const
{
    if (!_target)
    {
        return false;
    }
    
    vector2 toTarget = _target->getPosition() - getPosition();
    return toTarget.length() <= _aggroRange;
}

void Enemy::hit(float damage, GameObject* attacker)
{
    takeDamage(damage, attacker);
}

void Enemy::die()
{
    _isDead = true;
    _health = 0.0f;
    
    // Schedule respawn
    scheduleRespawn();
}

void Enemy::respawn(const vector2& position)
{
    setPosition(position);
    _health = _maxHealth;
    _isDead = false;
    _isAggroed = false;
    _target = nullptr;
    _aiState = AIState::PATROL;
    setVelocity(vector2(0, 0));
}

void Enemy::updateDeath(float time)
{
    _deathTimer += time;
    
    if (_deathTimer >= _respawnTime)
    {
        // Respawn
        respawn(_patrolPosition);
    }
}

void Enemy::updatePatrol(float time)
{
    if (!_isPatrolling || _isDead)
    {
        return;
    }
    
    // Move in circle around patrol position
    _patrolAngle += 0.02f;  // Rotation speed
    vector2 patrolOffset = vector2(
        cos(_patrolAngle) * _patrolRadius,
        sin(_patrolAngle) * _patrolRadius
    );
    
    vector2 targetPos = _patrolPosition + patrolOffset;
    moveTo(targetPos, _speed * 0.5f);  // Slow patrol speed
}

void Enemy::updateChase(float time)
{
    if (!_isAggroed || _isDead)
    {
        return;
    }
    
    // Get player position
    Player* player = Game::getPlayerWith(this);
    if (!player)
    {
        return;
    }
    
    GameObject* playerObj = player->getGameObject();
    if (!playerObj)
    {
        return;
    }
    
    vector2 toPlayer = playerObj->getPosition() - getPosition();
    float distance = toPlayer.length();
    
    // Check line of sight (simple distance check for now)
    if (distance > _detectionRange)
    {
        _aiState = AIState::WAIT;
        return;
    }
    
    // Move toward player
    moveTo(playerObj->getPosition(), _speed);
}

void Enemy::updateAttack(float time)
{
    if (!_isAggroed || _isDead)
    {
        _aiState = AIState::CHASE;
        return;
    }
    
    // Get player position
    Player* player = Game::getPlayerWith(this);
    if (!player)
    {
        return;
    }
    
    GameObject* playerObj = player->getGameObject();
    if (!playerObj)
    {
        return;
    }
    
    vector2 toPlayer = playerObj->getPosition() - getPosition();
    float distance = toPlayer.length();
    
    if (distance > 30.0f)  // Attack range
    {
        // Close the gap
        vector2 direction = toPlayer.normalize();
        moveTo(getPosition() + direction * 20.0f, _speed * 0.5f);
    }
    else
    {
        // In range - could add attack animation/damage here
        // For now, just stay in position
    }
}

void Enemy::updateFlee(float time)
{
    // Flee logic - move away from threat
    // Not implemented yet
}

void Enemy::updateWait(float time)
{
    // Wait logic - stay in position
    // Could add random waiting time then resume patrol
}

bool Enemy::detectPlayer()
{
    // Check if player is in detection range
    Player* player = Game::getPlayerWith(this);
    if (!player)
    {
        return false;
    }
    
    GameObject* playerObj = player->getGameObject();
    if (!playerObj)
    {
        return false;
    }
    
    return isInRange(playerObj);
}

bool Enemy::isInRange(GameObject* target)
{
    if (!target)
    {
        return false;
    }
    
    vector2 toTarget = target->getPosition() - getPosition();
    return toTarget.length() <= _detectionRange;
}

bool Enemy::canSeeTarget(GameObject* target)
{
    // Simple line-of-sight check (could be improved with raycasting)
    if (!target)
    {
        return false;
    }
    
    vector2 toTarget = target->getPosition() - getPosition();
    float distance = toTarget.length();
    
    // Check if within detection range
    if (distance > _detectionRange)
    {
        return false;
    }
    
    // For now, assume line of sight (no obstacles)
    return true;
}

void Enemy::startChase(GameObject* target)
{
    _target = target;
    _isAggroed = true;
    _aiState = AIState::CHASE;
}

void Enemy::stopChase()
{
    _isAggroed = false;
    _target = nullptr;
    _aiState = AIState::PATROL;
}

void Enemy::moveTo(vector2 targetPos, float speed)
{
    vector2 direction = targetPos - getPosition();
    float distance = direction.length();
    
    if (distance > 0.01f)
    {
        direction /= distance;
        setVelocity(direction * speed);
    }
    else
    {
        setVelocity(vector2(0, 0));
    }
}

void Enemy::handleCollisionContact(const CollisionContact& contact)
{
    // Override base implementation for enemy-specific collision handling
    // Could add damage to player on collision, etc.
    
    // For now, just call base implementation
    Character::handleCollisionContact(contact);
}

void Enemy::onStateDidEnter(State* previous, State* current)
{
    // Override base implementation
    // Could add state-specific behavior here
}

const char* Enemy::mapCollisionToCommand(const CollisionContact& contact) const
{
    // Override base implementation
    // Could customize collision response for enemies
    
    // For now, just call base implementation
    return Character::mapCollisionToCommand(contact);
}

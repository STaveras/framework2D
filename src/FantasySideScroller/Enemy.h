// Enemy.h - Base enemy class for Metroidvania + Vampire Survivors
// Derives from Character but simplifies for enemy AI and combat

#pragma once

#include "Character.h"
#include "../GameObject.h"

class Enemy : public Character
{
public:
    // Enemy types for variety
    enum class Type
    {
        BOAR,        // Melee, fast, low HP
        BEE,         // Flying, swarm behavior
        SNAIL,       // Slow, tanky
        SKELETON,    // Fast, hit-and-run
        ZOMBIE,      // Tanky, slow
        BAT,         // Flying, swarm
        CUSTOM       // For future enemy types
    };
    
    // Enemy stats structure
    struct Stats
    {
        float health = 100.0f;
        float maxHealth = 100.0f;
        float damage = 5.0f;
        float speed = 100.0f;
        float detectionRange = 200.0f;
        float aggroRange = 150.0f;
        float respawnTime = 3.0f;  // Seconds until respawn after death
    };
    
    // Enemy stats (static because accessed from non-static methods)
    static const Stats _defaultStats[];

protected:
    // Enemy-specific state
    float _health = 0.0f;
    float _maxHealth = 0.0f;
    float _damage = 0.0f;
    bool _isDead = false;
    bool _isInvulnerable = false;
    float _invulnerabilityTimer = 0.0f;
    bool _isPatrolling = true;
    vector2 _patrolPosition;
    float _patrolRadius = 100.0f;
    float _patrolAngle = 0.0f;
    
    // Combat state
    bool _isAggroed = false;
    float _lastHitTime = 0.0f;
    float _hitCooldown = 1.0f;
    
    // AI state
    enum class AIState
    {
        PATROL,
        CHASE,
        ATTACK,
        FLEE,
        WAIT
    };
    AIState _aiState = AIState::PATROL;
    
    // Movement
    float _speed = 100.0f;
    float _detectionRange = 200.0f;
    
    // Combat
    float _aggroRange = 150.0f;
    float _respawnTime = 3.0f;
    
    // Target (can be Player or other GameObject)
    GameObject* _target = nullptr;
    
    // Death handling
    float _deathTimer = 0.0f;
    bool _shouldRespawn = true;

public:
    Enemy(void);
    virtual ~Enemy(void);
    
    // Factory method
    static Enemy* create(Type type, const vector2& position);
    
    // Initialization
    void initialize(Type type, const vector2& startPos);
    void reset(void);
    
    // Update logic
    virtual void update(float time) override;
    virtual void updateAI(float time);
    virtual void updateCombat(float time);
    
    // Health management
    void takeDamage(float amount, GameObject* attacker = nullptr);
    void heal(float amount);
    float getHealth() const { return _health; }
    float getMaxHealth() const { return _maxHealth; }
    bool isAlive() const { return _health > 0.0f && !_isDead; }
    bool isDead() const { return _isDead; }
    
    // AI control
    void setAggro(bool aggro);
    bool isAggroed() const { return _isAggroed; }
    void setTarget(GameObject* target);
    GameObject* getTarget() const { return _target; }
    void setPatrolMode(bool enable);
    bool isPatrolling() const { return _isPatrolling; }
    
    // Combat
    void attack(GameObject* target);
    bool canAttack() const;
    void hit(float damage, GameObject* attacker);
    
    // Spawning/respawning
    void die();
    void respawn(const vector2& position);
    
    // Getters
    AIState getAIState() const { return _aiState; }
    float getInvulnerabilityTime() const { return _invulnerabilityTimer; }
    bool isInvulnerable() const { return _isInvulnerable; }
    
    // Override from Character
    virtual void handleCollisionContact(const CollisionContact& contact) override;
    virtual void onStateDidEnter(State* previous, State* current) override;
    virtual const char* mapCollisionToCommand(const CollisionContact& contact) const override;
    
    // Death handling
    void updateDeath(float time);
    
private:
    // AI state machine
    void updatePatrol(float time);
    void updateChase(float time);
    void updateAttack(float time);
    void updateFlee(float time);
    void updateWait(float time);
    
    // Detection
    bool detectPlayer();
    bool isInRange(GameObject* target);
    bool canSeeTarget(GameObject* target);
    
    // Movement helpers
    void startChase(GameObject* target);
    void stopChase();
    void moveTo(vector2 targetPos, float speed);
    
    // Death handling
    void handleDeath();
    void scheduleRespawn();
};

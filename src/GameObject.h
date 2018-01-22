// File: GameObject.h
#pragma once
#include "GAME_OBJ_TYPE.h"
#include "Appearance.h"
#include "CollidableGroup.h"
#include "StateMachine.h"
#include "Types.h"
class GameObject : public StateMachine
{
	GAME_OBJ_TYPE m_eType;
	void _OnAnimationStopped(const Event& e);

public:
	class GameObjectState : public State
	{
		friend GameObject;
		Renderable* _Renderable;
		Collidable* _Collidable;
        vector2 _Direction;
        double _Force;
		double _ExecuteTime;
		double _RunTime;

	public:
		Renderable* GetRenderable(void) { return _Renderable; }
		void SetRenderable(Renderable* renderable) { _Renderable = renderable; }

		Collidable* GetCollidable(void) { return _Collidable; }
		void SetCollidable(Collidable* collidable) { _Collidable = collidable; }

        vector2 getDirection(void) const { return _Direction; }
        void setDirection(vector2 direction) { _Direction = direction; }

        double getForce(void) const { return _Force; }
        void setForce(double force) { _Force = force; }

		double getExecuteTime(void) const { return _ExecuteTime; }
		void setExecuteTime(double runTime) { _ExecuteTime = runTime; }

		void OnEnter(State* prev);
		bool OnExecute(float time);
		void OnExit(void);
	};

protected:
	float m_fRotation;
	vector2 m_Position;
	vector2 m_Velocity;

public:
	GameObject(void):m_eType(GAME_OBJ_NULL), m_fRotation(0.0f){}
	GameObject(GAME_OBJ_TYPE eType):m_eType(eType),m_fRotation(0.0f){}
	~GameObject(void){}

	GAME_OBJ_TYPE GetType(void) const { return m_eType; }
	float GetRotation(void) const { return m_fRotation; }
	vector2 GetPosition(void) const { return m_Position; }
	vector2 GetVelocity(void) const { return m_Velocity; }
	void SetRotation(float fRotation) { m_fRotation = fRotation; } // UNDONE: PLEASE DON'T USE THIS ON ANYTHING OTHER THAN A CAMERA (cus collision objects don't rotate yet... :/)
	void SetPosition(float x, float y) { m_Position = vector2(x,y); }
	void SetPosition(vector2 position) { m_Position = position; }
	void SetVelocity(vector2 velocity) { m_Velocity = velocity; }

    void AddImpulse(vector2 direction, double force, double delta)
    {
        m_Velocity += ((direction * force) * delta);
    }

	GameObjectState* AddState(const char* szName);
	void SetAnimation(Animation* ani);
	void SetStateAnimation(const char* stateName, Animation* ani);

	virtual void Setup(void);
	virtual void Update(float fTime)
    {
        StateMachine::Update(fTime); m_Position += m_Velocity * fTime;
    }

	virtual void Shutdown(void);
};
// Author: Stanley Taveras

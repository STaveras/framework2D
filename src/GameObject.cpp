#include "GameObject.h"
#include "Animation.h"
#include "CollisionEvent.h"

#include "Factory.h"
#include "Square.h"
#include "Circle.h"
#include "Plane.h"
#include "CollidableGroup.h"

#include <utility>

void GameObject::onStateWillExit(State* current, State* next)
{
	(void)next;

	GameObjectState* previousState = (GameObjectState*)current;
	if (!previousState) {
		return;
	}

	this->addImpulse(previousState->getDirection(), -previousState->getForce());
	this->updateComponents();
}

void GameObject::onStateDidEnter(State* previous, State* current)
{
	(void)previous;

	GameObjectState* nextState = (GameObjectState*)current;
	if (!nextState) {
		return;
	}

	this->updateComponents();
	this->addImpulse(nextState->getDirection(), nextState->getForce());
}

bool GameObject::shouldCollideWith(const GameObject& other) const
{
	if (_collisionPredicate) {
		return _collisionPredicate(other);
	}
	return true;
}

void GameObject::setCollisionPredicate(CollisionPredicate predicate)
{
	_collisionPredicate = std::move(predicate);
}

const char* GameObject::mapCollisionToCommand(const CollisionContact& contact) const
{
	(void)contact;
	return NULL;
}

void GameObject::onCollisionContact(const CollisionContact& contact)
{
	if (!contact.other) {
		return;
	}

	if (contact.phase == CollisionPhase::Enter || contact.phase == CollisionPhase::Stay) {
		Physical::collision(contact.other);
	}

	handleCollisionContact(contact);

	if ((contact.phase == CollisionPhase::Enter || contact.phase == CollisionPhase::Stay) && _collisionEventHandler) {
		CollisionEvent legacyEvent(this, contact.other);
		_collisionEventHandler(&legacyEvent);
	}

	const char* command = mapCollisionToCommand(contact);
	if (command && command[0] != '\0') {
		sendInput(command);
	}
}


void GameObject::start(void)
{
	StateMachine::start();
	this->updateComponents();
}

void GameObject::setPosition(vector2 position) 
{
	Positionable::setPosition(position);
	
	this->updateComponents();
}

void GameObject::updateComponents()
{
	if (GameObjectState* state = this->getState()) 
	{
		if (Renderable* renderable = state->getRenderable()) {
			renderable->setPosition(this->getPosition()); 
		}
	}

	if (!_collisionObjects.empty()) {
		_collisionObjects.clear();
	}
}


void GameObject::update(float time) // lawl time as a float xfd
{
	Physical::update(time);
	StateMachine::update(time);

	updateComponents();
}

void GameObject::finish(void)
{
	StateMachine::finish();
}

Collidable* GameObject::getCollidable(void) 
{
	Collidable* collidable = (_collisionObjects.empty()) ? [&]() -> Collidable* {

		// Use the collision information from the current state
		if (GameObjectState* currentState = this->getState()) {

			Collidable* derived = nullptr;

			if (Collidable* stateCollidable = currentState->getCollidable()) {

				switch (stateCollidable->getType()) 
				{
				case COL_OBJ_SQUARE:
					derived = _collisionObjects.createDerived<Square>((Square&)*stateCollidable);
					break;

				case COL_OBJ_CIRCLE:
					derived = _collisionObjects.createDerived<Circle>((Circle&)*stateCollidable);
					break;
				case COL_OBJ_PLANE:
					derived = _collisionObjects.createDerived<Plane>((Plane&)*stateCollidable);
					break;
				case COL_OBJ_GROUP:
					derived = _collisionObjects.createDerived<CollidableGroup>((CollidableGroup&)*stateCollidable);
					break;
				default:
					return nullptr;
				}
				// Collidable information is consumed each frame; this translates local coordinates,
				// to potentially global coordinates, based on the actual position of this object's renderable
				// what we get is a shadow of the collidable
				vector2 anchor = this->getPosition();
				if (Renderable* renderable = this->getRenderable()) {
					anchor = renderable->getPosition()/* + renderable->getOffset()*/;
				}
				derived->setPosition(anchor + stateCollidable->getPosition());
				return derived;
			}
			else {
				return nullptr;
			}
		}
		return nullptr;
	}() : _collisionObjects.front();
	return collidable;
}

GameObject::GameObjectState* GameObject::addState(const char* name)
{
	GameObjectState* state = (GameObjectState*)this->getState(name);

	if (!state) {
		state = this->createDerived<GameObjectState>();
		state->setName(name);
	}
	return state;
}

Collidable* GameObject::GameObjectState::getCollidable(void)
{ 
	if (_collidable)
		return _collidable;

	else if (_renderable && _renderable->getRenderableType() == RENDERABLE_TYPE_ANIMATION) {
		if (Animation* animation = (Animation*)this->getRenderable()) {
			if (Frame* currentFrame = animation->getCurrentFrame()) {
				if (Collidable* collidable = currentFrame->getCollidable()) {
					return collidable;
				}
			}
		}
	}

	return NULL;
}

void GameObject::GameObjectState::onEnter(State* prevState)
{
	State::onEnter(prevState);

	_runTime = 0;

	if (_renderable) {

		if (prevState) {

			if (Renderable* prevRenderable = ((ObjectState*)prevState)->getRenderable()) {

				if (_preserveScaling) {

					vector2 oldCenter = _renderable->getCenter();
					vector2 newCenter(oldCenter.x * prevRenderable->getScale().x,
											oldCenter.y * prevRenderable->getScale().y);

					// We should just be checking and matching signs
					if (_renderable->getScale() != prevRenderable->getScale()) {

						if (Collidable* collidable = _collidable) {
							switch (collidable->getType()) {
							case COL_OBJ_SQUARE:
								if (oldCenter != newCenter) {
									collidable->setPosition(collidable->getPosition() - oldCenter);
									collidable->setPosition(collidable->getPosition() + newCenter);
								}
								break;
							default:
								break;
							}
						}

						_renderable->setCenter(newCenter);
						_renderable->setScale(prevRenderable->getScale());
						_renderable->setPosition(prevRenderable->getPosition());
					}
				}
			}
		}
		_renderable->setVisibility(true);

		switch (_renderable->getRenderableType())
		{
		case RENDERABLE_TYPE_ANIMATION:

			((Animation*)_renderable)->play();

			break;

		default:
			break;
		}
	}

}

bool GameObject::GameObjectState::onExecute(float time)
{
	_runTime += time;

	bool finalCheck = true;

	if (_renderable) {

		switch (_renderable->getRenderableType())
		{
		case RENDERABLE_TYPE_ANIMATION: {

			finalCheck = ((Animation*)this->getRenderable())->update(time);

			break;
		}
		default:

			break;
		}
	}

	if ((_executeTime > 0 && _runTime >= _executeTime) || !finalCheck)
		return false;

	return finalCheck;
}

void GameObject::GameObjectState::onExit(State* nextState)
{
	if (_renderable) {

		switch (_renderable->getRenderableType())
		{
		case RENDERABLE_TYPE_ANIMATION:
			((Animation*)_renderable)->stop();
			break;
		default:
			break;
		}

		_renderable->setVisibility(false);
	}

	State::onExit(nextState);
}

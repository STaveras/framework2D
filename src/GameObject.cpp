#include "GameObject.h"
#include "Animation.h"
#include "InputEvent.h"
#include "CollisionEvent.h"

#include "Factory.h"
#include "Square.h"

void GameObject::_onStateEntered(const Event& e) 
{
	this->updateComponents();

	if (GameObjectState* sendingState = (GameObjectState*)e.getSender()) {
		if (this->find(*sendingState)) { // We need to make sure the state we're entering is from our object only
			this->addImpulse(sendingState->getDirection(), sendingState->getForce());
		}
	}
}

void GameObject::_onStateExited(const Event& e)
{
	if (GameObjectState* sendingState = (GameObjectState*)e.getSender()) {
		if (this->find(*sendingState)) { 
			this->addImpulse(sendingState->getDirection(), -sendingState->getForce());
		}
	}

	this->updateComponents();
}

void GameObject::_onCollision(const Event& e) 
{
	CollisionEvent* collisionEvent = (CollisionEvent*)&e;

	GameObject* object = (GameObject*)collisionEvent->getSender();
	GameObject* otherObject = collisionEvent->involvedObject;

	Physical::collision(otherObject);

	if (_collisionEventHandler) {
		_collisionEventHandler(collisionEvent);
	}
}

void GameObject::start(void)
{
	Engine2D::getEventSystem()->registerCallback<GameObject>(EVT_COLLISION, this, &GameObject::_onCollision);
	Engine2D::getEventSystem()->registerCallback<GameObject>(EVT_GAMEOBJECT_STATE_ENTER, this, &GameObject::_onStateEntered);
	Engine2D::getEventSystem()->registerCallback<GameObject>(EVT_GAMEOBJECT_STATE_EXIT, this, &GameObject::_onStateExited);

	StateMachine::start();
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
		_collisionObjects.clear();
	}
}

void GameObject::update(float time) // lawl time as a float xfd
{
	Physical::update(time);
	StateMachine::update(time);
}

void GameObject::finish(void)
{
	StateMachine::finish();

	Engine2D::getEventSystem()->unregister<GameObject>(EVT_GAMEOBJECT_STATE_EXIT, this, &GameObject::_onStateExited);
	Engine2D::getEventSystem()->unregister<GameObject>(EVT_GAMEOBJECT_STATE_ENTER, this, &GameObject::_onStateEntered);
	Engine2D::getEventSystem()->unregister<GameObject>(EVT_COLLISION, this, &GameObject::_onCollision);
}

Collidable* GameObject::getCollidable(void) 
{
	Collidable* collidable = (_collisionObjects.empty()) ? [&]() -> Collidable* {
		// Use the collision information from the current state
		if (collidable = this->getState()->getCollidable()) {
			switch (collidable->getType()) {
			case COL_OBJ_SQUARE:
				collidable = _collisionObjects.createDerived<Square>((Square&)*collidable);
				break;
			}
			collidable->setPosition(this->getRenderable()->getPosition() + collidable->getPosition());
		}
		return collidable;
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

	else if (_renderable && _renderable->getRenderableType() == RENDERABLE_TYPE_ANIMATION)
	{
		if (Animation* animation = (Animation*)this->getRenderable()) {

			if (Frame* currentFrame = animation->getCurrentFrame()) {

				if (Collidable* collidable = currentFrame->getCollidable()) {

					return (_collidable = collidable);
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

						if (Collidable* collidable = this->getCollidable()) {
							switch (collidable->getType()) {
							case COL_OBJ_SQUARE:
								if (oldCenter != newCenter) {
									collidable->setPosition(collidable->getPosition() - oldCenter);
									collidable->setPosition(collidable->getPosition() + newCenter);
								}
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

	Engine2D::getInstance()->getEventSystem()->sendEvent(EVT_GAMEOBJECT_STATE_ENTER, this);
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

	if (_executeTime > 0 && _runTime >= _executeTime || !finalCheck)
		return false;

	return finalCheck;
}

void GameObject::GameObjectState::onExit(State* nextState)
{
	Engine2D::getInstance()->getEventSystem()->sendEvent(EVT_GAMEOBJECT_STATE_EXIT, this);

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
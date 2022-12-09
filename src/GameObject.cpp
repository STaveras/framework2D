#include "GameObject.h"
#include "Animation.h"
#include "InputEvent.h"

void GameObject::_OnStateEntered(const Event& e) 
{
	updateComponents();

	GameObjectState* sendingState = (GameObjectState*)e.getSender();
	if (sendingState) {
		if (this->find(*sendingState)) { // We need to make sure the state we're entering is from our object only
			this->addImpulse(sendingState->getDirection(), sendingState->getForce());
		}
	}
}

void GameObject::_OnStateExited(const Event& e)
{
	GameObjectState* sendingState = (GameObjectState*)e.getSender();
	if (sendingState) {
		if (this->find(*sendingState)) { 
			this->addImpulse(sendingState->getDirection(), -sendingState->getForce());
		}
	}
}

void GameObject::_OnCollision(const Event& e) {

	CollisionEvent* collidedEvent = (CollisionEvent*)&e;

	if (collisionEventHandler) {
		collisionEventHandler(collidedEvent);
	}
}

void GameObject::start(void)
{
	Engine2D::getEventSystem()->registerCallback<GameObject>(EVT_COLLISION, this, &GameObject::_OnCollision);
	Engine2D::getEventSystem()->registerCallback<GameObject>(EVT_GAMEOBJECT_STATE_ENTER, this, &GameObject::_OnStateEntered);
	Engine2D::getEventSystem()->registerCallback<GameObject>(EVT_GAMEOBJECT_STATE_EXIT, this, &GameObject::_OnStateExited);

	StateMachine::start();
}

void GameObject::updateComponents()
{
	if (this->getState()) {

		if (this->getRenderable()) {
			this->getRenderable()->setPosition(this->getPosition());
		}
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

	Engine2D::getEventSystem()->unregister<GameObject>(EVT_GAMEOBJECT_STATE_EXIT, this, &GameObject::_OnStateExited);
	Engine2D::getEventSystem()->unregister<GameObject>(EVT_GAMEOBJECT_STATE_ENTER, this, &GameObject::_OnStateEntered);
	Engine2D::getEventSystem()->unregister<GameObject>(EVT_COLLISION, this, &GameObject::_OnCollision);
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

void GameObject::GameObjectState::onEnter(State* prevState)
{
	State::onEnter(prevState);

	_runTime = 0;

	if (_renderable) {

		if (prevState) {

			Renderable* prevRenderable = ((ObjectState*)prevState)->getRenderable();

			if (prevRenderable) {

				if (_preserveScaling) {

					// We should just be checking and matching signs
					if (_renderable->getScale() != prevRenderable->getScale()) {

						this->getRenderable()->setScale(prevRenderable->getScale());

						vector2 oldCenter = this->getRenderable()->getCenter();
						vector2 newCenter(oldCenter.x * this->getRenderable()->getScale().x,
							oldCenter.y * this->getRenderable()->getScale().y);

						this->getRenderable()->setCenter(newCenter);
					}
				}

				//_renderable->setPosition(prevRenderable->getPosition());
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

			Animation* animation = (Animation*)this->getRenderable();

			finalCheck = animation->update(time);

			Frame* currentFrame = animation->getCurrentFrame();

			Sprite* sprite = currentFrame->getSprite();

			Collidable* collidable = currentFrame->getCollidable();

			if (collidable) {
				this->setCollidable(collidable);
				this->getCollidable()->setPosition((sprite->getPosition() + sprite->getOffset()) - sprite->getCenter());
			}
			break;
		}
		default:
			if (this->getCollidable()) {
				this->getCollidable()->setPosition((_renderable->getPosition() + _renderable->getOffset()) - _renderable->getCenter());
			}
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
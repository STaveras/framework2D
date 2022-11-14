#include "GameObject.h"
#include "Animation.h"
#include "InputEvent.h"

void GameObject::_OnKeyPressed(const Event& e)
{
	this->sendInput(((std::string)((InputEvent*)&e)->setActionName() + "_PRESSED").c_str(), e.getSender());
}

void GameObject::_OnKeyReleased(const Event& e)
{
	this->sendInput(((std::string)((InputEvent*)&e)->setActionName() + "_RELEASED").c_str(), e.getSender());
}

//void GameObject::_OnAnimationStopped(const Event& e)
//{
//	for (unsigned int i = 0; i < this->Size(); i++)
//		if (this->At(i) == (GameObjectState*)e.getSender())
//			return this->sendInput("ANIMATION_STOPPED");
//}

void GameObject::_OnCollision(const Event& e) {

	CollisionEvent* collidedEvent = (CollisionEvent*)&e;

	if (collisionEventHandler) {
		collisionEventHandler(collidedEvent);
	}
}

void GameObject::update(float time) // lawl time as a float xfd
{
	Physical::update(time);
	StateMachine::update(time);
}

GameObject::GameObjectState* GameObject::addState(const char* name)
{
	GameObjectState* state = (GameObjectState*)this->getState(name);

	if (!state) {
		state = this->CreateDerived<GameObjectState>();
		state->setName(name);
	}
	return state;
}

void GameObject::GameObjectState::onEnter(State* prevState)
{
	State::onEnter(prevState);

	_runTime = 0;

	if (_renderable) {

		if (_preserveMirror) {

			if (prevState) {

				Renderable* prevRenderable = ((ObjectState*)prevState)->getRenderable();

				if (prevRenderable && (_renderable->getScale() != prevRenderable->getScale())) {
					
					this->getRenderable()->setScale(prevRenderable->getScale());

					vector2 oldCenter = this->getRenderable()->getCenter();
					vector2 newCenter(oldCenter.x * this->getRenderable()->getScale().x,
									  oldCenter.y * this->getRenderable()->getScale().y);

					this->getRenderable()->setCenter(newCenter);
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

			Animation* animation = (Animation*)_renderable;

			if (!animation->update(time) && animation->getMode() == Animation::Mode::eOnce)
				finalCheck = false;

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
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

void GameObject::GameObjectState::onEnter(State* prev)
{
	_runTime = 0;

	if (_renderable) {

		if (_preserveMirror) {

			if (prev && !((ObjectState*)prev)->isPreservingMirroring()) {

				Renderable* prevRenderable = ((ObjectState*)prev)->getRenderable();

				if (_renderable->_appearance._flipHorizontalAxis != prevRenderable->_appearance._flipHorizontalAxis || 
					_renderable->_appearance._flipVerticalAxis != prevRenderable->_appearance._flipVerticalAxis) {

					_renderable->mirror(prevRenderable->_appearance._flipHorizontalAxis, 
										prevRenderable->_appearance._flipVerticalAxis);

					_renderable->center();
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

void GameObject::GameObjectState::onExit(State* next)
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

		if (!_preserveMirror && ((ObjectState*)next)->isPreservingMirroring()) {

			if (next) {

				Renderable* nextRenderable = ((ObjectState*)next)->getRenderable();

				if (nextRenderable) {

					if (_renderable->_appearance._flipHorizontalAxis != nextRenderable->_appearance._flipHorizontalAxis ||
						_renderable->_appearance._flipVerticalAxis != nextRenderable->_appearance._flipVerticalAxis) {

						nextRenderable->mirror(_renderable->_appearance._flipHorizontalAxis,
							                   _renderable->_appearance._flipVerticalAxis);

						nextRenderable->center();
					}
				}
			}
		}

		_renderable->setVisibility(false);
	}
}
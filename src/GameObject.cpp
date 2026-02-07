#include "GameObject.h"
#include "Animation.h"

#include "Factory.h"
#include "Square.h"
#include "Circle.h"
#include "Plane.h"
#include "Polygon.h"
#include "CollidableGroup.h"

#include <functional>
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

vector2 GameObject::getCollisionAnchor(void) const
{
	vector2 anchor = this->getPosition();

	if (!_useRenderableOffsetForCollisionAnchor) {
		return anchor;
	}

	if (GameObjectState* state = this->getState()) {
		if (Renderable* renderable = state->getRenderable()) {
			anchor = anchor + vector2(renderable->getOffset());
		}
	}

	return anchor;
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

			if (Collidable* stateCollidable = currentState->getCollidable()) {
				std::function<Collidable*(const Collidable*, const vector2&)> cloneCollidable =
					[&](const Collidable* source, const vector2& parentWorldPosition) -> Collidable* {
						if (!source) {
							return nullptr;
						}

						const vector2 worldPosition = parentWorldPosition + source->getPosition();

						switch (source->getType())
						{
						case COL_OBJ_SQUARE: {
							Square* clone = _collisionObjects.createDerived<Square>((const Square&)*source);
							clone->setPosition(worldPosition);
							return clone;
						}
						case COL_OBJ_CIRCLE: {
							Circle* clone = _collisionObjects.createDerived<Circle>((const Circle&)*source);
							clone->setPosition(worldPosition);
							return clone;
						}
						case COL_OBJ_PLANE: {
							Plane* clone = _collisionObjects.createDerived<Plane>((const Plane&)*source);
							clone->setPosition(worldPosition);
							return clone;
						}
						case COL_OBJ_POLYGON: {
							PolygonCollider* clone = _collisionObjects.createDerived<PolygonCollider>((const PolygonCollider&)*source);
							clone->setPosition(worldPosition);
							return clone;
						}
						case COL_OBJ_GROUP: {
							const CollidableGroup* sourceGroup = (const CollidableGroup*)source;
							if (!sourceGroup) {
								return nullptr;
							}

							CollidableGroup* cloneGroup = _collisionObjects.createDerived<CollidableGroup>();
							cloneGroup->setPosition(worldPosition);
							for (const Collidable* member : *sourceGroup) {
								if (Collidable* clonedMember = cloneCollidable(member, worldPosition)) {
									cloneGroup->push_back(clonedMember);
								}
							}
							return cloneGroup;
						}
						default:
							return nullptr;
						}
					};

				// Collidable information is consumed each frame. This translates state-local
				// collision coordinates to world-space using the object's collision anchor.
				vector2 anchor = this->getCollisionAnchor();
				return cloneCollidable(stateCollidable, anchor);
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

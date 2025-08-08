#pragma once

#ifndef _ENTITY_H_
#define _ENTITY_H_

#include "Types.h"

using EntityID = uint32_t;

struct Component {
	virtual ~Component() = default;
};

class Entity
{
    EntityID id;
    std::unordered_map<std::type_index, std::unique_ptr<Component>> components;

public:
    template<typename T>
    void addComponent(std::unique_ptr<T> component);

    template<typename T>
    T* getComponent();
};

#endif
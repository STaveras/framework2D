// File: Factory.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 3/29/2010

// TODO: Really need a way to pass constructor arguments to the factory...

#if !defined(_FACTORY_H)
#define _FACTORY_H

#include <list>
#include <map>
#include <unordered_set>

// One collection per type. Items made with create() or adopted with store()
// are owned and deleted by the Factory; items stored with owned = false are
// only referenced, so destroy(), erase() and clear() remove them without
// deleting them.
template<class Type>
class Factory: public std::list<Type*>
{
	std::unordered_set<const Type*> _borrowed;

	void _release(Type* item);

public:
	Factory(void) {}
	~Factory() { clear(); }

	//using std::list<Type*>::begin;
	//using std::list<Type*>::end;

	typedef typename std::list<Type*>::iterator factory_iterator;
	typedef typename std::list<Type*>::const_iterator const_factory_iterator;

	factory_iterator begin(void) { return std::list<Type*>::begin(); }
	factory_iterator end(void) { return std::list<Type*>::end(); }

	const_factory_iterator begin(void) const { return std::list<Type*>::begin(); }
	const_factory_iterator end(void) const { return std::list<Type*>::end(); }

	Type* at(unsigned int index);
	const Type* at(unsigned int index) const;
	size_t size(void) const { return std::list<Type*>::size(); }
	bool empty(void) const { return std::list<Type*>::empty(); }
	void erase(unsigned int index);
	void erase(factory_iterator itr);
	void erase(const_factory_iterator itr);
	void destroy(Type* item);
	void store(Type* item, bool owned = true);
	bool owns(const Type* item) const { return item && !_borrowed.count(item); }
	void clear(void);

   Type* find(const Type& itemDesc); // BROKEN
   //Type* Factory<Type>::Filter(const char *attribute, const void* value);

	template<class _Pr3>
	void sort(_Pr3 _Pred) { std::list<Type*>::sort(_Pred); }

	Type* create(void); // Items made with create and be destroyed for early clean up
	Type* create(const Type& rhs); // Copy-contructor create

	template<class Derived>
	Derived* createDerived(void);

	template<class Derived>
	Derived* createDerived(const Derived& rhs); // Copy-contructor create

	//const Type* operator[](unsigned int i) const { 
	//	std::list<Type*>::const_iterator itr = this->begin(); 
	//	while (i--) {
	//		itr++; if (itr == this->end();) break; 
	//	}
	//	return (*itr);
	//}

	const Type* operator[](unsigned int i) const { return this->at(i); }
	Type* operator[](unsigned int i) { return this->at(i); }
	bool operator==(Factory<Type>& f);
	bool operator==(const Factory<Type>& f) const;
};

#include "Factory.hpp"

#endif  //_FACTORY_H

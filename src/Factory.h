// File: Factory.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 3/29/2010

// TODO: Really need a way to pass constructor arguments to the factory...

#if !defined(_FACTORY_H)
#define _FACTORY_H

#include <list>
#include <map>

template<class Type>
class Factory
{
	std::list<Type*> m_lsItems;

public:
	Factory(void) {}
	~Factory() { Clear(); }

	typedef typename std::list<Type*>::iterator factory_iterator;
	typedef typename std::list<Type*>::const_iterator const_factory_iterator;

	Type* At(unsigned int index);
	factory_iterator Begin(void) { return m_lsItems.begin(); }
	const_factory_iterator Begin(void) const { return m_lsItems.begin(); }
	void Destroy(Type* item);	
	factory_iterator End(void) { return m_lsItems.end(); }
	const_factory_iterator End(void) const { return m_lsItems.end(); }
	bool Empty(void) const { return m_lsItems.empty(); }
	void Erase(unsigned int index);
	void Erase(factory_iterator itr) { m_lsItems.erase(itr); }
	void Erase(const_factory_iterator itr) { m_lsItems.erase(itr); }
	void Clear(void);
	void Store(Type* item);
	size_t Size(void) const { return m_lsItems.size(); }

   Type* Find(const Type& itemDesc); // BROKEN
   //Type* Factory<Type>::Filter(const char *attribute, const void* value);

	template<class _Pr3>
	void Sort(_Pr3 _Pred) { m_lsItems.sort(_Pred); }

	Type* Create(void);
	Type* Create(const Type& rhs); // Copy-contructor create

	template<class Derived>
	Derived* CreateDerived(void);

	template<class Derived>
	Derived* CreateDerived(const Derived& rhs); // Copy-contructor create

	Type* operator[](unsigned int i) { return this->At(i); }
	bool operator==(Factory<Type>& f);
	bool operator==(const Factory<Type>& f) const;
};

#include "Factory.hpp"

#endif  //_FACTORY_H

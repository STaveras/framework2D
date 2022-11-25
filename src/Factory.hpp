// File: Factory.cpp
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 3/29/2010

template<typename Type>
Type* Factory<Type>::at(unsigned int index)
{
	typename std::list<Type *>::iterator itr = this->begin();
	for (unsigned int i = 0; itr != this->end(); itr++, i++) {
		if (i == index)
			return (*itr);
	}
	return NULL;
}

template<typename Type>
Type* Factory<Type>::create()
{
	Type* item = new Type();
	this->push_back(item);
	return item;
}

template<typename Type>
Type* Factory<Type>::create(const Type& rhs)
{
	Type* item = new Type(rhs);
	this->push_back(item);
	return item;
}

template<typename Type>
void Factory<Type>::destroy(Type* item)
{
	typename std::list<Type*>::iterator itr = this->begin();

	for (;itr != this->end(); itr++)
	{
		if ((*itr) == item)
		{
			delete (*itr);
			this->erase(itr);
			break;
		}
	}
}

template<typename Type>
void Factory<Type>::clear()
{
	typename std::list<Type*>::iterator itr = this->begin();

	for(;itr != this->end(); itr++) {
		delete (*itr);
	}

	std::list<Type*>::clear();
}

template<typename Type>
void Factory<Type>::store(Type* item)
{
	this->push_back(item);
}

template<typename Type>
void Factory<Type>::erase(unsigned int index)
{
	typename std::list<Type*>::iterator itr = this->begin();

	for (unsigned int i = 0; itr != this->end(); itr++, i++)
	{
		if (i == index) {
			this->erase(itr);
		}
	}
}

template<typename Type>
void Factory<Type>::erase(factory_iterator itr) { 
	std::list<Type*>::erase(itr); 
}

template<typename Type>
void Factory<Type>::erase(const_factory_iterator itr) {
	std::list<Type*>::erase(itr); 
}

template<typename Type>
Type* Factory<Type>::find(const Type& itemDesc)
{
	Type* pReturn = NULL;
	typename std::list<Type*>::iterator itr = this->begin();

	for (;itr != this->end(); itr++) {
		if (itemDesc == *(*itr)) {
			pReturn = (*itr);
		}
	}
	return pReturn;
}

template<typename Type>
template<class Derived>
Derived* Factory<Type>::createDerived(void)
{
	Derived* item = new Derived();
	this->push_back((Type*)item); // Would using dynamic cast here be safer...?
	return item;
}

template<typename Type>
template<class Derived>
Derived* Factory<Type>::createDerived(const Derived& rhs)
{
	Derived* item = new Derived(rhs);
	this->push_back((Type*)item);
	return item;
}

template<class Type>
bool Factory<Type>::operator==(Factory<Type>& f)
{
   return (this == (const_cast<const Factory<Type>>(f))); // Why is this here again...?
}

template<typename Type>
bool Factory<Type>::operator==(const Factory<Type>& f) const
{
	if (this->size() == f.size())
	{
		typename std::list<Type*>::const_iterator i = this->begin();
		typename std::list<Type*>::const_iterator j = f.begin();

		for (; i != this->end(); i++, j++)
			if ((*i) != (*j))
				return false;

	}
	return true;
}

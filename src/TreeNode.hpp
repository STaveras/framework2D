
#pragma once

template <typename T>
int TreeNode<T>::noCreated = 0;

template <typename T>
int TreeNode<T>::noDestroyed = 0;

template <typename T>
int TreeNode<T>::getNoCreated()
{
	return noCreated;
}

template <typename T>
int TreeNode<T>::getNoDestroyed()
{
	return noDestroyed;
}

template <typename T>
TreeNode<T>::TreeNode(T element, TreeNode* p) : data(element)
{
	if( p )
	{
		parent = p;
		parent->children.push_back(this);
	}

	noCreated++;
	noDestroyed = 0;
}

template <typename T>
TreeNode<T>::TreeNode(TreeNode* p)
{
	if( p )
	{
		parent = p;
		parent->children.push_back(this);
	}

	noCreated++;
	noDestroyed = 0;
}

template <typename T>
TreeNode<T>::~TreeNode()
{
	deleteChildren();

	noDestroyed++;
}

template <typename T>
TreeNode<T>* TreeNode<T>::getParent() const
{
	return this->parent;
}

template <typename T>
T const& TreeNode<T>::getData() const
{
	return this->data;
}

template <typename T>
T& TreeNode<T>::getData()
{
	return this->data;
}

template <typename T>
std::size_t TreeNode<T>::getChildCount() const
{
	return children.size();
}

template <typename T>
TreeNode<T> const* TreeNode<T>::getChild(std::size_t index) const
{
	return children[index];
}

template <typename T>
TreeNode<T>* TreeNode<T>::getChild(std::size_t index)
{
	return children[index];
}

template <typename T>
void TreeNode<T>::deleteChildren()
{
	if( !this->getChildCount() )
		return;

	while( !children.empty() )
	{
		children.back()->deleteChildren();
		delete children.back();
		children.pop_back();
	}
}

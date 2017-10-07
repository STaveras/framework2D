
#pragma once

#include <vector>

//! \brief C++ implementation of an n-ary tree node, or a node that can
//! store an arbitrary number of children.
template <typename T>
class TreeNode
{
	static int noCreated;
	static int noDestroyed;

	std::vector<TreeNode*> children;
	TreeNode* parent;
	T data;

public:
	//! \brief Constructs a new <code>%TreeNode</code> over the specified
	//! data and parent node.
	explicit TreeNode(T element, TreeNode* p = 0);

	//! \brief Constructs a new <code>%TreeNode</code> with the specified
	//! parent node.
	//!
	//! You can use this constructor only if the type <code>T</code> is
	//! default-constructible.
	explicit TreeNode(TreeNode* p = 0);

	//! \brief Cleans up all internal memory.
	~TreeNode();

	static int getNoCreated();
	static int getNoDestroyed();

private:
	TreeNode(TreeNode const&);
	TreeNode& operator=(TreeNode const&);

public:
	//! \brief Returns the parent of this <code>%TreeNode</code>.
	TreeNode* getParent() const;

	//! \brief Returns the data stored in this <code>%TreeNode</code>.
	T const& getData() const;

	//! \brief Returns the data stored in this <code>%TreeNode</code>.
	T& getData();

	//! \brief Returns the number of child nodes stored in this
	//! <code>%TreeNode</code>.
	std::size_t getChildCount() const;

	//! \brief Returns a pointer to the index-th child node stored in this
	//! <code>%TreeNode</code>.
	//!
	//! \pre      \a index <code>\< getChildCount()</code>
	//! \post     <code>NULL != getChild(index)</code>
	TreeNode const* getChild(std::size_t index) const;

	//! \brief Returns a pointer to the index-th child node stored in this
	//! <code>%TreeNode</code>.
	//!
	//! \pre      \a index <code>\< getChildCount()</code>
	//! \post     <code>NULL != getChild(index)</code>
	TreeNode* getChild(std::size_t index);

	//! \brief Removes all child nodes and cleans up their memory.
	void deleteChildren();
};

#include "TreeNode.hpp"


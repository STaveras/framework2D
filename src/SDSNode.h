
#pragma once

#include "TreeNode.h"

#include <string>

class SDSNodeData
{
	friend class SDSParser;

public:
	enum SDSNodeType { SDS_SCOPE, SDS_PROPERTY };

private:
	SDSNodeType Type;
	std::string Name;
	std::string Value;

public:
	SDSNodeData(void):Type(SDS_SCOPE){}

	SDSNodeType GetType(void) const { return Type; }
	const char* GetName(void) const { return Name.c_str(); }
	const char* GetValue(void) const { return Value.c_str(); }
	int GetValueAsInt(void) const { return atoi(Value.c_str()); }
	double GetValueAsDouble(void) const { return atof(Value.c_str()); }

	void SetType(SDSNodeType type) { Type = type; }
	void SetName(const char* szName) { Name = szName; }
	void SetValue(const char* szValue) { Value = szValue; }
};

typedef TreeNode<SDSNodeData> SDSNode;

class SDSMetaData
{		
	friend class SDSParser;

	size_t uiUnnamedScopes;
	//size_t uiUnnamedProperties;

	SDSNode* pSDSNode;

public:
	SDSMetaData(void):uiUnnamedScopes(0)/*,uiUnnamedProperties(0)*/,pSDSNode(NULL){}
};

typedef TreeNode<SDSMetaData> SDSMetaNode;
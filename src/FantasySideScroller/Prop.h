#pragma once

#include "../GameObject.h"

#include <string>

class Prop : public GameObject
{
public:
	enum class RenderLayerHint
	{
		Back,
		Runtime,
		Front
	};

	struct Descriptor
	{
		std::string texturePath;
		RECT srcRect{ -1, -1, -1, -1 };
		vector2 pivot{ 0.0f, 0.0f };
		float scale = 1.0f;
		bool flipX = false;
		float rotationDegrees = 0.0f;
		Color tint = Color(0xFFFFFFFF);
		RenderLayerHint renderLayer = RenderLayerHint::Back;
		bool visible = true;
	};

	struct DescriptorPatch
	{
		Descriptor values;
		bool hasTexturePath = false;
		bool hasSrcRect = false;
		bool hasPivot = false;
		bool hasScale = false;
		bool hasFlipX = false;
		bool hasRotation = false;
		bool hasTint = false;
		bool hasRenderLayer = false;
		bool hasVisible = false;
	};

private:
	Image* _image = NULL;

public:
	Prop();
	~Prop() override;

	static bool ParseDescriptorJSON(const std::string& jsonValue, DescriptorPatch& outPatch, std::string* outError = NULL);
	static void MergeDescriptor(Descriptor& base, const DescriptorPatch& patch);

	bool InitializeFromDescriptor(const Descriptor& descriptor, const vector2& worldPosition, std::string* outError = NULL);
};

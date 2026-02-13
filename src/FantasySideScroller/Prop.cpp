#include "Prop.h"

#include "../Engine2D.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>

namespace {
constexpr float kPi = 3.14159265358979323846f;

bool isValidSrcRect(const RECT& rect)
{
	return rect.left >= 0 &&
		rect.top >= 0 &&
		rect.right > rect.left &&
		rect.bottom > rect.top;
}

std::string trimString(const std::string& value)
{
	size_t start = 0;
	while (start < value.size() && std::isspace((unsigned char)value[start])) {
		++start;
	}

	size_t end = value.size();
	while (end > start && std::isspace((unsigned char)value[end - 1])) {
		--end;
	}

	return value.substr(start, end - start);
}

std::string lowercaseCopy(const std::string& value)
{
	std::string lowered = value;
	std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) {
		return (char)std::tolower(c);
	});
	return lowered;
}

bool parseHexColor(const std::string& value, Color& outColor)
{
	std::string normalized = trimString(value);
	if (normalized.empty()) {
		return false;
	}

	if (normalized.rfind("0x", 0) == 0 || normalized.rfind("0X", 0) == 0) {
		normalized = normalized.substr(2);
	}
	else if (normalized[0] == '#') {
		normalized = normalized.substr(1);
	}

	if (normalized.size() == 6) {
		normalized = std::string("FF") + normalized;
	}
	if (normalized.size() != 8) {
		return false;
	}

	for (char c : normalized) {
		if (!std::isxdigit((unsigned char)c)) {
			return false;
		}
	}

	const uint32_t packedValue = (uint32_t)std::strtoul(normalized.c_str(), NULL, 16);
	outColor = Color((unsigned long)packedValue);
	return true;
}
}

Prop::Prop() :
	GameObject(GAME_OBJ_OBJECT)
{
	this->setStatic(true);
	this->setCollisionPredicate([](const GameObject&) {
		return false;
	});
	this->addState("Idle");
}

Prop::~Prop() {}

bool Prop::ParseDescriptorJSON(const std::string& jsonValue, DescriptorPatch& outPatch, std::string* outError)
{
	outPatch = DescriptorPatch();

	const std::string trimmedValue = trimString(jsonValue);
	if (trimmedValue.empty()) {
		if (outError) {
			*outError = "prop_json is empty";
		}
		return false;
	}

	simdjson::dom::parser parser;
	simdjson::padded_string paddedJson(trimmedValue);
	simdjson::simdjson_result<simdjson::dom::element> parseResult = parser.parse(paddedJson);
	if (parseResult.error()) {
		if (outError) {
			*outError = std::string("invalid prop_json: ") + simdjson::error_message(parseResult.error());
		}
		return false;
	}

	simdjson::dom::element root = parseResult.value();
	if (!root.is_object()) {
		if (outError) {
			*outError = "prop_json must be a JSON object";
		}
		return false;
	}

	auto readFloat = [](simdjson::dom::element element, float fallback = 0.0f) -> float {
		if (element.is_null()) {
			return fallback;
		}
		if (element.is_double()) {
			return (float)element.get_double();
		}
		if (element.is_int64()) {
			return (float)element.get_int64();
		}
		if (element.is_uint64()) {
			return (float)element.get_uint64();
		}
		return fallback;
	};

	auto readBool = [](simdjson::dom::element element, bool fallback = false) -> bool {
		if (element.is_null()) {
			return fallback;
		}
		if (element.is_bool()) {
			return (bool)element.get_bool();
		}
		if (element.is_int64()) {
			return element.get_int64() != 0;
		}
		if (element.is_uint64()) {
			return element.get_uint64() != 0;
		}
		return fallback;
	};

	if (!root["texture"].is_null()) {
		if (!root["texture"].is_string()) {
			if (outError) {
				*outError = "prop_json.texture must be a string";
			}
			return false;
		}
		outPatch.values.texturePath = std::string((std::string_view)root["texture"].get_string());
		outPatch.hasTexturePath = true;
	}

	if (!root["src"].is_null()) {
		if (!root["src"].is_object()) {
			if (outError) {
				*outError = "prop_json.src must be an object";
			}
			return false;
		}

		const int x = (int)std::round(readFloat(root["src"]["x"], 0.0f));
		const int y = (int)std::round(readFloat(root["src"]["y"], 0.0f));
		const int w = (int)std::round(readFloat(root["src"]["w"], 0.0f));
		const int h = (int)std::round(readFloat(root["src"]["h"], 0.0f));
		outPatch.values.srcRect = RECT{ x, y, x + w, y + h };
		outPatch.hasSrcRect = true;
	}

	if (!root["pivot"].is_null()) {
		if (!root["pivot"].is_object()) {
			if (outError) {
				*outError = "prop_json.pivot must be an object";
			}
			return false;
		}
		outPatch.values.pivot = vector2(
			readFloat(root["pivot"]["x"], 0.0f),
			readFloat(root["pivot"]["y"], 0.0f));
		outPatch.hasPivot = true;
	}

	if (!root["scale"].is_null()) {
		outPatch.values.scale = readFloat(root["scale"], 1.0f);
		outPatch.hasScale = true;
	}

	if (!root["flip_x"].is_null()) {
		outPatch.values.flipX = readBool(root["flip_x"], false);
		outPatch.hasFlipX = true;
	}

	if (!root["rotation"].is_null()) {
		outPatch.values.rotationDegrees = readFloat(root["rotation"], 0.0f);
		outPatch.hasRotation = true;
	}

	if (!root["tint"].is_null()) {
		Color parsedColor(0xFFFFFFFF);
		if (root["tint"].is_string()) {
			const std::string tintValue = std::string((std::string_view)root["tint"].get_string());
			if (!parseHexColor(tintValue, parsedColor)) {
				if (outError) {
					*outError = "prop_json.tint string is not a valid hex color";
				}
				return false;
			}
		}
		else if (root["tint"].is_int64()) {
			parsedColor = Color((unsigned long)root["tint"].get_int64());
		}
		else if (root["tint"].is_uint64()) {
			parsedColor = Color((unsigned long)root["tint"].get_uint64());
		}
		else {
			if (outError) {
				*outError = "prop_json.tint must be a string or integer";
			}
			return false;
		}

		outPatch.values.tint = parsedColor;
		outPatch.hasTint = true;
	}

	if (!root["render_layer"].is_null()) {
		if (!root["render_layer"].is_string()) {
			if (outError) {
				*outError = "prop_json.render_layer must be a string";
			}
			return false;
		}
		const std::string layerName = lowercaseCopy(std::string((std::string_view)root["render_layer"].get_string()));
		if (layerName == "back") {
			outPatch.values.renderLayer = RenderLayerHint::Back;
		}
		else if (layerName == "runtime") {
			outPatch.values.renderLayer = RenderLayerHint::Runtime;
		}
		else if (layerName == "front") {
			outPatch.values.renderLayer = RenderLayerHint::Front;
		}
		else {
			if (outError) {
				*outError = "prop_json.render_layer must be back, runtime, or front";
			}
			return false;
		}
		outPatch.hasRenderLayer = true;
	}

	if (!root["visible"].is_null()) {
		outPatch.values.visible = readBool(root["visible"], true);
		outPatch.hasVisible = true;
	}

	return true;
}

void Prop::MergeDescriptor(Descriptor& base, const DescriptorPatch& patch)
{
	if (patch.hasTexturePath) {
		base.texturePath = patch.values.texturePath;
	}
	if (patch.hasSrcRect) {
		base.srcRect = patch.values.srcRect;
	}
	if (patch.hasPivot) {
		base.pivot = patch.values.pivot;
	}
	if (patch.hasScale) {
		base.scale = patch.values.scale;
	}
	if (patch.hasFlipX) {
		base.flipX = patch.values.flipX;
	}
	if (patch.hasRotation) {
		base.rotationDegrees = patch.values.rotationDegrees;
	}
	if (patch.hasTint) {
		base.tint = patch.values.tint;
	}
	if (patch.hasRenderLayer) {
		base.renderLayer = patch.values.renderLayer;
	}
	if (patch.hasVisible) {
		base.visible = patch.values.visible;
	}
}

bool Prop::InitializeFromDescriptor(const Descriptor& descriptor, const vector2& worldPosition, std::string* outError)
{
	if (descriptor.texturePath.empty()) {
		if (outError) {
			*outError = "missing texturePath in descriptor";
		}
		return false;
	}

	GameObjectState* idle = this->getState("Idle");
	if (!idle) {
		idle = this->addState("Idle");
	}
	if (!idle) {
		if (outError) {
			*outError = "failed to create Idle state";
		}
		return false;
	}

	IRenderer* renderer = Engine2D::getRenderer();
	if (!renderer) {
		if (outError) {
			*outError = "renderer unavailable";
		}
		return false;
	}

	_image = _spriteManager.createDerived<Image>();
	if (!_image) {
		if (outError) {
			*outError = "failed to allocate prop image";
		}
		return false;
	}

	ITexture* texture = renderer->createTexture(descriptor.texturePath.c_str());
	if (!texture) {
		if (outError) {
			*outError = "failed to load prop texture";
		}
		return false;
	}
	_image->setTexture(texture);

	if (isValidSrcRect(descriptor.srcRect)) {
		_image->setSrcRect(descriptor.srcRect);
	}
	else {
		_image->setSrcRect(RECT{
			0,
			0,
			(int)texture->getWidth(),
			(int)texture->getHeight()
			});
	}

	_image->setCenter(descriptor.pivot);

	const float scaleY = descriptor.scale;
	const float scaleX = descriptor.flipX ? -descriptor.scale : descriptor.scale;
	_image->setScale(scaleX, scaleY);
	_image->setRotation(descriptor.rotationDegrees * (kPi / 180.0f));
	_image->setTint(descriptor.tint);
	_image->setVisibility(descriptor.visible);

	idle->setCollidable(NULL);
	idle->setRenderable(_image);
	idle->setForce(0.0);
	idle->setDirection(vector2(0.0f, 0.0f));

	this->setState(idle);
	this->setPosition(worldPosition);

	return true;
}

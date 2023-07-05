// TileSet.cpp

#include "TileSet.h"

#include "Engine2D.h"
#include "FileSystem.h"
#include "Square.h"

// I usually hate globals, but this one will only be accessible to TileSets
// Eventually this might grow too large if we're loading many tilesets and not clearing this
static Factory<Collidable> collisionObjects; 


TileSet* TileSet::loadFromFile(const char* fileName)
{
	TileSet* tileSet = NULL;

	simdjson::dom::parser parser;
	simdjson::dom::element root = parser.load(fileName);

	if (!root.is_null()) {

		std::string workingDirectory = FileSystem::File::GetFilePath(fileName);
		std::string_view imageName = root["image"].get_string();

		if (!imageName.empty()) {

			int64_t tileWidth = root["tilewidth"].get_int64();
			int64_t tileHeight = root["tileheight"].get_int64();
			int64_t tileCount = root["tilecount"].get_int64();

			std::string imagePath = workingDirectory + "/";
			imagePath.append(imageName);

			tileSet = new TileSet(Engine2D::getRenderer()->createTexture(imagePath.c_str()), (unsigned int)tileWidth);

			simdjson::dom::array tiles = root["tiles"].get_array();

			for (size_t i = 0; i < tiles.size(); i++)
			{
				simdjson::dom::element tile = tiles.at(i);

				// Access fields of the "tile" object here
				int64_t id = tile["id"].get_int64();

				TileSet::TileInfo tileInfo;

				if (!tile["class"].is_null() && tile["class"].is_string()) {
					std::string_view className = tile["class"].get_string();
					tileInfo._typeName = std::string(className);
				}

				if (!tile["objectgroup"].is_null() && tile["objectgroup"].is_object()) {
					// Tile properties
					simdjson::dom::element objectgroup = tile["objectgroup"];

					// Right now, the only collection of "objects" we have are for collision
					if (!objectgroup["objects"].is_null() && objectgroup["objects"].is_array()) {

						for (auto object : objectgroup["objects"]) {
							std::string_view collisionObjectType = object["class"];

							if (collisionObjectType == "square") {
								Square* square = collisionObjects.createDerived<Square>();
								square->_x = (float)object["x"].get_double();
								square->_y = (float)object["y"].get_double();
								square->setWidth((float)object["width"].get_double());
								square->setHeight((float)object["height"].get_double());
								tileInfo._collisionInfo = square;
							}
							else if (collisionObjectType == "polygon") { // TODO: Support other collision object types
#if _DEBUG
								DEBUG_MSG("polygon\n");
#endif
							}
						}
					}
				}

				if (tileInfo._typeName != "" || tileInfo._collisionInfo != NULL) {
					tileSet->_tileInfo[(int)id] = tileInfo;
				}
			}

			for (int64_t i = 0; i < tileCount; i++)
			{
				TileInfo tileInfo = tileSet->getTileInfo((int)i);

				if (!tileInfo._collisionInfo) {
					Square* square = collisionObjects.createDerived<Square>();
					square->setPosition(0, 0);
					square->setWidth(roundf((float)tileWidth));
					square->setHeight(roundf((float)tileHeight));
					tileInfo._collisionInfo = square;

					tileSet->_tileInfo[(int)i] = tileInfo;
				}
				else
					continue;
			}
		}
	}

	return tileSet;
}
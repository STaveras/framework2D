#include "Sprite.h"
#include "Vertex.h"

class Billboard {
public:
   Billboard(Sprite* sprite)
      : sprite(sprite) {
      // Initialize vertices based on the sprite's texture size
      initializeVertices();
   }

   // Accessor for vertices
   const Vertex* getVertices() const {
      return vertices;
   }

private:
   // Pointer to the associated Sprite object
   Sprite* sprite;

   // Vertices to represent the corners of the billboard
   Vertex vertices[4];

   // Function to initialize vertices based on the texture size
   void initializeVertices() {
      // Retrieve texture size
      uint32_t width = sprite->getTexture()->getWidth();
      uint32_t height = sprite->getTexture()->getHeight();

      // Define vertices to represent the quad
      vertices[0] = Vertex{ vector3(0, 0, 0), vector2(0, 0), sprite->getTintColor() };
      vertices[1] = Vertex{ vector3((float)width, 0, 0), vector2(1, 0), sprite->getTintColor() };
      vertices[2] = Vertex{ vector3(0, (float)height, 0), vector2(0, 1), sprite->getTintColor() };
      vertices[3] = Vertex{ vector3((float)width, (float)height, 0), vector2(1, 1), sprite->getTintColor() };
   }
};

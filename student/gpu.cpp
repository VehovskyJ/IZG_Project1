/*!
 * @file
 * @brief This file contains implementation of gpu
 *
 * @author Tomáš Milet, imilet@fit.vutbr.cz
 */

#include <student/gpu.hpp>

struct Triangle {
    OutVertex vertices[3];
};

// Clears the GPU memory framebuffer
void clear(GPUMemory &mem, ClearCommand cmd) {
    if (cmd.clearColor) {
        // Iterates over every pixel in the framebuffer and sets its colour
        for (uint32_t i = 0; i < mem.framebuffer.width * mem.framebuffer.height; ++i) {
            float red = cmd.color.r;
            mem.framebuffer.color[i*4+0] = (uint8_t) (red * 255.f);
            float green = cmd.color.g;
            mem.framebuffer.color[i*4+1] = (uint8_t) (green * 255.f);
            float blue = cmd.color.b;
            mem.framebuffer.color[i*4+2] = (uint8_t) (blue * 255.f);
            float alpha = cmd.color.a;
            mem.framebuffer.color[i*4+3] = (uint8_t) (alpha * 255.f);
        }
    }

    if (cmd.clearDepth) {
        float depth = cmd.depth;
        // Iterates over every pixel in the framebuffer and sets its depth
        for (uint32_t i = 0; i < mem.framebuffer.width * mem.framebuffer.height; ++i) {
            mem.framebuffer.depth[i] = depth;
        }
    }
}

// Computes vertex ID
void computeVertexID(GPUMemory &mem, DrawCommand cmd, InVertex &inVertex, uint32_t i) {
    // If indexBufferID is equal to -1, the vertex is not indexed and sets the vertex id based on a for loop in calling function
    if (cmd.vao.indexBufferID == -1) {
        inVertex.gl_VertexID = i;
        return;
    }

    // Gets the vertex ID from the index buffer data
    const uint8_t* idData = static_cast<const uint8_t*>(mem.buffers[cmd.vao.indexBufferID].data) + cmd.vao.indexOffset;

    uint32_t index;
    switch (cmd.vao.indexType) {
        case IndexType::UINT8:
            index = idData[i];
            break;
        case IndexType::UINT16:
            index = reinterpret_cast<const uint16_t*>(idData)[i];
            break;
        case IndexType::UINT32:
            index = reinterpret_cast<const uint32_t*>(idData)[i];
            break;
    }

    // Sets the vertex ID to the correct value
    inVertex.gl_VertexID = index;
}

// Reads vertex attributes and binds them to inVertex attributes
void readAttributes(InVertex &inVertex, GPUMemory &mem, DrawCommand cmd) {
    uint64_t i = 0; // attribute index
    // Itterate over every attribute
    for (auto attr : cmd.vao.vertexAttrib) {
        uint64_t offset = attr.offset;
        uint64_t stride = attr.stride;
        uint64_t bufferID = attr.bufferID;
        AttributeType type = attr.type;

        // Calculate the index of the vertex in the buffer
        uint64_t index = offset + stride * inVertex.gl_VertexID;
        // Get a pointer to the memory location of the attribute data
        auto value = (uint8_t*)mem.buffers[bufferID].data + index;

        // Stores the attribute in the appropriate variable in vertex
        switch (type) {
            case AttributeType::FLOAT:
                inVertex.attributes[i].v1 = *((float *) value);
                break;
            case AttributeType::VEC2:
                inVertex.attributes[i].v2 = *((glm::vec2 *) value);
                break;
            case AttributeType::VEC3:
                inVertex.attributes[i].v3 = *((glm::vec3 *) value);
                break;
            case AttributeType::VEC4:
                inVertex.attributes[i].v4 = *((glm::vec4 *) value);
                break;
            case AttributeType::UINT:
                inVertex.attributes[i].u1 = *((uint32_t *) value);
                break;
            case AttributeType::UVEC2:
                inVertex.attributes[i].u2 = *((glm::uvec2 *) value);
                break;
            case AttributeType::UVEC3:
                inVertex.attributes[i].u3 = *((glm::uvec3 *) value);
                break;
            case AttributeType::UVEC4:
                inVertex.attributes[i].u4 = *((glm::uvec4 *) value);
                break;
        }
        // Increments the attribute index
        ++i;
    }
}

// Assigns the vertex ID and attributes to the vertex
void runVertexAssembly(InVertex &inVertex, GPUMemory &mem, DrawCommand cmd, uint32_t i) {
    computeVertexID(mem, cmd, inVertex, i);
    readAttributes(inVertex, mem, cmd);
}

// Initializes vertices and assembles triangle from them
void triangleAssembly(Triangle &triangle, GPUMemory &mem, DrawCommand cmd, uint32_t drawID, uint32_t triangleIndex) {
    Program prg = mem.programs[cmd.programID];

    // Iterate through every vertex
    for (int i = 0; i < 3; ++i) {
        InVertex inVertex;
        OutVertex outVertex;

        inVertex.gl_DrawID = drawID;

        // Assign the vertex ID and attributes to the vertex
        runVertexAssembly(inVertex, mem, cmd, triangleIndex * 3 + i);

        // Sets the shader properties
        ShaderInterface si;
        si.textures = mem.textures;
        si.uniforms = mem.uniforms;

        prg.vertexShader(outVertex, inVertex, si);

        // Adds the outVertex to the triangle
        triangle.vertices[i] = outVertex;
    }
}

// Performs perspective division on a triangle
void perspectiveDivision(Triangle &triangle) {
    // Iterates through all vertices in triangle and divide x, y and z coordinate for every vertex by it w coordinate
    for (auto &vertice : triangle.vertices) {
        vertice.gl_Position.x /= vertice.gl_Position.w;
        vertice.gl_Position.y /= vertice.gl_Position.w;
        vertice.gl_Position.z /= vertice.gl_Position.w;
    }
}

// Performs viewport transformation on a triangle
void viewportTransformation(Triangle &triangle, GPUMemory &mem) {
    // Iterates through all vertices in the triangle
    for (auto &vertex : triangle.vertices) {
        // Apply the viewport transform to the vertex
        vertex.gl_Position.x = ((vertex.gl_Position.x + 1.0) / 2.0) * mem.framebuffer.width;
        vertex.gl_Position.y = ((vertex.gl_Position.y + 1.0) / 2.0) * mem.framebuffer.height;
        vertex.gl_Position.z = (vertex.gl_Position.z + 1.0) / 2.0;
    }
}

// Calculates cross product of a triangle
float crossProduct(const Triangle &triangle) {
    glm::vec3 a = triangle.vertices[0].gl_Position;
    glm::vec3 b = triangle.vertices[1].gl_Position;
    glm::vec3 c = triangle.vertices[2].gl_Position;

    float crossProduct = ((b.x - a.x) * (c.y - a.y)) - ((c.x - a.x) * (b.y - a.y));

    return crossProduct;
}

// Determines if a triangle is facing away from the viewer
bool isBackface(const Triangle &triangle) {
    // Calculates the dot product between the normal vector and the view direction
    float dotProduct = crossProduct(triangle);

    // Triangle is facing away if the dot product is negative
    return dotProduct < 0;
}

// Calculates barycentric coordinates
glm::vec3 calculateBarycentric(Triangle &triangle, glm::vec2 point) {
    auto a = triangle.vertices[0].gl_Position;
    auto b = triangle.vertices[1].gl_Position;
    auto c = triangle.vertices[2].gl_Position;

    // Calculates the denominator for barycentric coordinates
    double denominator = ((b.y - c.y) * (a.x - c.x) + (c.x - b.x) * (a.y - c.y));

    double u = ((b.y - c.y) * (point.x - c.x) + (c.x - b.x) * (point.y - c.y)) / denominator;
    double v = ((c.y - a.y) * (point.x - c.x) + (a.x - c.x) * (point.y - c.y)) / denominator;
    float w = 1.0f - u - v;

    return glm::vec3(u, v, w);
}

void rasterizeFragment(GPUMemory &mem, Triangle &triangle, glm::vec3 barycentric, glm::vec2 point, Program &prg) {
    auto a = triangle.vertices[0].gl_Position;
    auto b = triangle.vertices[1].gl_Position;
    auto c = triangle.vertices[2].gl_Position;

    InFragment inFragment;
    inFragment.gl_FragCoord.z = a.z * barycentric.x + b.z * barycentric.y + c.z * barycentric.z;
    inFragment.gl_FragCoord.x = point.x;
    inFragment.gl_FragCoord.y = point.y;

    int index = static_cast<int>(point.x) + static_cast<int>(point.y) * mem.framebuffer.width;

    OutFragment outFragment;
    // TODO: interpolate sracka nebo co
    ShaderInterface si;
    // TODO: idk sracka
    prg.fragmentShader(outFragment, inFragment, si);

    if (inFragment.gl_FragCoord.z < mem.framebuffer.depth[index]) {
        float alpha = outFragment.gl_FragColor.a;
        if (alpha > 0.5) {
            mem.framebuffer.depth[index] = inFragment.gl_FragCoord.z;

            float blend = 1.f - alpha;

            uint8_t r = glm::min(mem.framebuffer.color[index * 4 + 0] * blend + outFragment.gl_FragColor.r * alpha + 255.f, 255.f);
            uint8_t g = glm::min(mem.framebuffer.color[index * 4 + 1] * blend + outFragment.gl_FragColor.g * alpha + 255.f, 255.f);
            uint8_t b = glm::min(mem.framebuffer.color[index * 4 + 2] * blend + outFragment.gl_FragColor.b * alpha + 255.f, 255.f);

            mem.framebuffer.color[index * 4 + 0] = r;
            mem.framebuffer.color[index * 4 + 1] = g;
            mem.framebuffer.color[index * 4 + 2] = b;
        }
    }

}

void rasterizeTriangle(GPUMemory &mem, Triangle &triangle, Program &prg) {
    if (crossProduct(triangle) == 0.f) {
        return;
    }

    int minX = std::min(triangle.vertices[0].gl_Position.x, std::min(triangle.vertices[1].gl_Position.x, triangle.vertices[2].gl_Position.x));
    int maxX = std::max(triangle.vertices[0].gl_Position.x, std::max(triangle.vertices[1].gl_Position.x, triangle.vertices[2].gl_Position.x));
    int minY = std::min(triangle.vertices[0].gl_Position.y, std::min(triangle.vertices[1].gl_Position.y, triangle.vertices[2].gl_Position.y));
    int maxY = std::max(triangle.vertices[0].gl_Position.y, std::max(triangle.vertices[1].gl_Position.y, triangle.vertices[2].gl_Position.y));

    minX = std::max(minX, 0);
    maxX = std::min(maxX, (int)mem.framebuffer.width - 1);
    minY = std::max(minY, 0);
    maxY = std::min(maxY, (int)mem.framebuffer.height - 1);

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            auto p = glm::vec2{x + 0.5f, y + 0.5f};
            auto barycentric = calculateBarycentric(triangle, p);
            if (barycentric.x >= 0 && barycentric.y >= 0 && barycentric.z >= 0) {
                rasterizeFragment(mem, triangle, barycentric, p, prg);
            }
        }
    }
}

// Handles triangle drawing
void draw(GPUMemory &mem, DrawCommand cmd, uint32_t drawID) {
    Program prg = mem.programs[cmd.programID];
    // Iterate through all triangles
    for (uint32_t i = 0; i < cmd.nofVertices / 3; ++i) {
        Triangle triangle;
        // Assembles the triangle
        triangleAssembly(triangle, mem, cmd, drawID, i);

        // Performs perspective division
        perspectiveDivision(triangle);

        // Performs viewport transformation
        viewportTransformation(triangle, mem);

        // Skip triangle if facing way from the viewer and backfaceCulling is enabled
        if (cmd.backfaceCulling && !isBackface(triangle)) {
            rasterizeTriangle(mem, triangle, prg);
        }
    }
}

//! [gpu_execute]
void gpu_execute(GPUMemory&mem,CommandBuffer &cb){
  (void)mem;
  (void)cb;
  /// \todo Tato funkce reprezentuje funkcionalitu grafické karty.<br>
  /// Měla by umět zpracovat command buffer, čistit framebuffer a kresli.<br>
  /// mem obsahuje paměť grafické karty.
  /// cb obsahuje command buffer pro zpracování.
  /// Bližší informace jsou uvedeny na hlavní stránce dokumentace.

    uint32_t drawid = 0;

    for (uint32_t i = 0; i < cb.nofCommands; ++i) {
        CommandType type = cb.commands[i].type;
        CommandData data = cb.commands[i].data;

        // Clear command
        if (type == CommandType::CLEAR) {
            clear(mem, data.clearCommand);
        }

        // Draw command
        if (type == CommandType::DRAW) {
            draw(mem, data.drawCommand, drawid);
            ++drawid;
        }
    }
}
//! [gpu_execute]

/**
 * @brief This function reads color from texture.
 *
 * @param texture texture
 * @param uv uv coordinates
 *
 * @return color 4 floats
 */
glm::vec4 read_texture(Texture const&texture,glm::vec2 uv){
  if(!texture.data)return glm::vec4(0.f);
  auto uv1 = glm::fract(uv);
  auto uv2 = uv1*glm::vec2(texture.width-1,texture.height-1)+0.5f;
  auto pix = glm::uvec2(uv2);
  //auto t   = glm::fract(uv2);
  glm::vec4 color = glm::vec4(0.f,0.f,0.f,1.f);
  for(uint32_t c=0;c<texture.channels;++c)
    color[c] = texture.data[(pix.y*texture.width+pix.x)*texture.channels+c]/255.f;
  return color;
}

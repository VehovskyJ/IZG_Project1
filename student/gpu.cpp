/*!
 * @file
 * @brief This file contains implementation of gpu
 *
 * @author Tomáš Milet, imilet@fit.vutbr.cz
 */

#include <student/gpu.hpp>

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

void computeVertexID(GPUMemory &mem, DrawCommand cmd, InVertex &inVertex, uint32_t i) {
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

    inVertex.gl_VertexID = index;
}

void setVertexAttributes(InVertex &inVertex, GPUMemory &mem, DrawCommand cmd) {
    uint64_t i = 0;
    for (auto attr : cmd.vao.vertexAttrib) {
        uint64_t offset = attr.offset;
        uint64_t stride = attr.stride;
        uint64_t bufferID = attr.bufferID;
        AttributeType type = attr.type;

        uint64_t index = offset + stride * inVertex.gl_VertexID;
        auto value = (uint8_t*)mem.buffers[bufferID].data + index;

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
        ++i;
    }
}

void draw(GPUMemory &mem, DrawCommand cmd, uint32_t drawID) {
    Program prg = mem.programs[cmd.programID];

    for (uint32_t i = 0; i < cmd.nofVertices; ++i) {
        InVertex inVertex;
        OutVertex outVertex;

        inVertex.gl_DrawID = drawID;

        inVertex.gl_VertexID = i;
        if (cmd.vao.indexBufferID != -1) {
            computeVertexID(mem, cmd, inVertex, i);
        }

        setVertexAttributes(inVertex, mem, cmd);

        ShaderInterface si;
        si.textures = mem.textures;
        si.uniforms = mem.uniforms;

        prg.vertexShader(outVertex, inVertex, si);
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

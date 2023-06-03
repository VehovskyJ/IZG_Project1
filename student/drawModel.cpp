/*!
 * @file
 * @brief This file contains functions for model rendering
 *
 * @author Tomáš Milet, imilet@fit.vutbr.cz
 */
#include <student/drawModel.hpp>
#include <student/gpu.hpp>

///\endcond

void prepareNode(GPUMemory&mem, CommandBuffer &cmd, Node const&node, Model const&model, glm::mat4 matrix) {
    matrix *= node.modelMatrix;
    if (node.mesh >= 0) {
        Mesh mesh = model.meshes[node.mesh];

        cmd.commands[cmd.nofCommands].type = CommandType::DRAW;

        cmd.commands[cmd.nofCommands].data.drawCommand.programID = 0;
        cmd.commands[cmd.nofCommands].data.drawCommand.nofVertices = mesh.nofIndices;
        cmd.commands[cmd.nofCommands].data.drawCommand.backfaceCulling = !mesh.doubleSided;

        cmd.commands[cmd.nofCommands].data.drawCommand.vao.vertexAttrib[0] = mesh.position;
        cmd.commands[cmd.nofCommands].data.drawCommand.vao.vertexAttrib[1] = mesh.normal;
        cmd.commands[cmd.nofCommands].data.drawCommand.vao.vertexAttrib[2] = mesh.texCoord;

        cmd.commands[cmd.nofCommands].data.drawCommand.vao.indexBufferID = mesh.indexBufferID;
        cmd.commands[cmd.nofCommands].data.drawCommand.vao.indexOffset = mesh.indexOffset;
        cmd.commands[cmd.nofCommands].data.drawCommand.vao.indexType = mesh.indexType;

        glm::mat4 inverseTranspose = glm::transpose(glm::inverse(matrix));
        mem.uniforms[5 + cmd.nofCommands * 5 + 0].m4 = matrix;
        mem.uniforms[5 + cmd.nofCommands * 5 + 1].m4 = inverseTranspose;
        mem.uniforms[5 + cmd.nofCommands * 5 + 2].v4 = mesh.diffuseColor;
        mem.uniforms[5 + cmd.nofCommands * 5 + 3].i1 = mesh.diffuseTexture;
        mem.uniforms[5 + cmd.nofCommands * 5 + 4].v1 = mesh.doubleSided;

        cmd.nofCommands++;
    }

    for (size_t i=0;i<node.children.size();++i) {
        prepareNode(mem, cmd, node.children[i], model, matrix);
    }
}

/**
 * @brief This function prepares model into memory and creates command buffer
 *
 * @param mem gpu memory
 * @param commandBuffer command buffer
 * @param model model structure
 */
//! [drawModel]
void prepareModel(GPUMemory &mem, CommandBuffer &commandBuffer, Model const &model) {
    /// \todo Tato funkce připraví command buffer pro model a nastaví správně pamět grafické karty.<br>
    /// Vaším úkolem je správně projít model a vložit vykreslovací příkazy do commandBufferu.
    /// Zároveň musíte vložit do paměti textury, buffery a uniformní proměnné, které buffer command buffer využívat.
    /// Bližší informace jsou uvedeny na hlavní stránce dokumentace a v testech.
    commandBuffer.commands[0].type = CommandType::CLEAR;
    commandBuffer.commands[0].data.clearCommand.color = glm::vec4(0.1f, 0.15f, 0.1f, 1.0f);
    commandBuffer.commands[0].data.clearCommand.depth = 1e11;
    commandBuffer.commands[0].data.clearCommand.clearColor = true;
    commandBuffer.commands[0].data.clearCommand.clearDepth = true;

    commandBuffer.nofCommands = 1;

//    mem.textures = model.textures;
//    mem.buffers = model.buffers;
    for (int i = 0; i < model.textures.size(); ++i) {
        mem.textures[i] = model.textures[i];
    }
    for (int i = 0; i < model.buffers.size(); ++i) {
        mem.buffers[i] = model.buffers[i];
    }

    mem.programs[0].vertexShader = drawModel_vertexShader;
    mem.programs[0].fragmentShader = drawModel_fragmentShader;
//    mem.programs[0].vs2fs = {
//            AttributeType::VEC3,
//            AttributeType::VEC3,
//            AttributeType::VEC2,
//            AttributeType::UINT,
//    };
    mem.programs[0].vs2fs[0] = AttributeType::VEC3;
    mem.programs[0].vs2fs[1] = AttributeType::VEC3;
    mem.programs[0].vs2fs[2] = AttributeType::VEC2;
    mem.programs[0].vs2fs[3] = AttributeType::UINT;

    glm::mat4 jednotkovaMAtice = glm::mat4(1.f);
    for (const auto& root : model.roots) {
        prepareNode(mem, commandBuffer, root, model, jednotkovaMAtice);
    }
}
//! [drawModel]

/**
 * @brief This function represents vertex shader of texture rendering method.
 *
 * @param outVertex output vertex
 * @param inVertex input vertex
 * @param si shader interface
 */
//! [drawModel_vs]
void drawModel_vertexShader(OutVertex &outVertex, InVertex const &inVertex, ShaderInterface const &si) {
    (void) outVertex;
    (void) inVertex;
    (void) si;
    /// \todo Tato funkce reprezentujte vertex shader.<br>
    /// Vaším úkolem je správně trasnformovat vrcholy modelu.
    /// Bližší informace jsou uvedeny na hlavní stránce dokumentace.
}
//! [drawModel_vs]

/**
 * @brief This functionrepresents fragment shader of texture rendering method.
 *
 * @param outFragment output fragment
 * @param inFragment input fragment
 * @param si shader interface
 */
//! [drawModel_fs]
void drawModel_fragmentShader(OutFragment &outFragment, InFragment const &inFragment, ShaderInterface const &si) {
    (void) outFragment;
    (void) inFragment;
    (void) si;
    /// \todo Tato funkce reprezentujte fragment shader.<br>
    /// Vaším úkolem je správně obarvit fragmenty a osvětlit je pomocí lambertova osvětlovacího modelu.
    /// Bližší informace jsou uvedeny na hlavní stránce dokumentace.
}
//! [drawModel_fs]


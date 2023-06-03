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
    /// \todo Tato funkce reprezentujte vertex shader.<br>
    /// Vaším úkolem je správně trasnformovat vrcholy modelu.
    /// Bližší informace jsou uvedeny na hlavní stránce dokumentace.
    glm::vec3 position = inVertex.attributes[0].v3;
    glm::vec3 normalVecotr = inVertex.attributes[1].v3;

    glm::mat4 projectionViewMatrix = si.uniforms[0].m4;
    glm::mat4 modelMatrix = si.uniforms[10 + inVertex.gl_DrawID * 5 + 0].m4;
    glm::mat4 inverseTransposedMatrix = si.uniforms[10 + inVertex.gl_DrawID * 5 + 1].m4;

    outVertex.attributes[0].v3 = glm::vec3(modelMatrix * glm::vec4(position, 1.0f));
    outVertex.attributes[1].v3 = glm::vec3(inverseTransposedMatrix * glm::vec4(normalVecotr, 0.0f));
    outVertex.attributes[2].v2 = inVertex.attributes[2].v2;

    outVertex.gl_Position = projectionViewMatrix * modelMatrix * glm::vec4(position, 1.0f);

    outVertex.attributes[3].u1 = inVertex.gl_DrawID;
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
    (void)outFragment;
    (void)inFragment;
    (void)si;
    /// \todo Tato funkce reprezentujte fragment shader.<br>
    /// Vaším úkolem je správně obarvit fragmenty a osvětlit je pomocí lambertova osvětlovacího modelu.
    /// Bližší informace jsou uvedeny na hlavní stránce dokumentace.
//    glm::vec3 position = inFragment.attributes[0].v3;
//    glm::vec3 normalVecotr = glm::normalize(inFragment.attributes[1].v3);
//    glm::vec2 texCoords = inFragment.attributes[2].v2;
//
//    glm::vec3 lightPosition = glm::normalize(si.uniforms[1].v3);
//    glm::vec3 cameraPosition = si.uniforms[2].v3;
//    int texNumber = si.uniforms[10 + inFragment.attributes[3].u1 * 5 + 3].i1;
//    float doubleSided = si.uniforms[10 + inFragment.attributes[3].u1 * 5 + 4].v1;
//
//    glm::vec4 diffuseColour;
//    if (texNumber >= 0) {
//        diffuseColour = read_texture(si.textures[texNumber], texCoords);
//    } else {
//        diffuseColour = si.uniforms[10 + inFragment.attributes[3].u1 * 5 + 2].v4;
//    }
//
//    auto ambient = diffuseColour * 0.2f;
//    auto lightDirection = glm::normalize(lightPosition - position);
//    auto diffusion = diffuseColour * glm::clamp(glm::dot(lightDirection, normalVecotr), 0.0f, 1.0f);
//
//
//    if (doubleSided > 0.0f) {
//        glm::vec3 viewDirection = glm::normalize(cameraPosition - position);
//        if (glm::dot(normalVecotr, viewDirection) < 0.0f) {
//            normalVecotr = -normalVecotr;
//        }
//    }
//
//    glm::vec4 finalColor = ambient + diffusion;
//    finalColor.a = diffuseColour.a;
//
//    outFragment.gl_FragColor = finalColor;
}
//! [drawModel_fs]


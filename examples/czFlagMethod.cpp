/*!
 * @file
 * @brief This file contains implementation of czech flag rendering method
 *
 * @author Tomáš Milet, imilet@fit.vutbr.cz
 */

#include <framework/programContext.hpp>
#include <vector>

namespace czFlagMethod{

/**
 * @brief Czech flag rendering method
 */
class Method: public ::Method{
  public:
    Method(MethodConstructionData const*);
    virtual ~Method(){}
    virtual void onDraw(Frame&frame,SceneParam const&sceneParam) override;
    virtual void onUpdate(float dt) override;
    CommandBuffer           commandBuffer       ;///< command buffer
    GPUMemory               mem                 ;///< gpu memory
    
    struct Vertex{
      glm::vec2 position;
      glm::vec2 texCoord;
    };

    std::vector<Vertex  >vertices  ;///< vertex buffer   
    std::vector<uint32_t>indices   ;///< index buffer
    float                time = 0.f;///< elapsed time
    uint32_t const       NX   = 100;///< nof vertices in x direction
    uint32_t const       NY   = 10 ;///< nof vertices in y direction
};

/**
 * @brief Czech flag vertex shader
 *
 * @param outVertex out vertex
 * @param inVertex in vertex
 * @param uniforms uniform variables
 */
void vertexShader(OutVertex&outVertex,InVertex const&inVertex,ShaderInterface const&si){
  auto const& pos   = inVertex.attributes[0].v2;
  auto const& coord = inVertex.attributes[1].v2;
  auto const& mvp   = si.uniforms[0].m4;

  auto time = si.uniforms[1].v1;
  
  auto z = (coord.x*0.5f)*glm::sin(coord.x*10.f + time);
  outVertex.gl_Position = mvp*glm::vec4(pos,z,1.f);

  outVertex.attributes[0].v2 = coord;
}

/**
 * @brief Czech flag fragment shader
 *
 * @param outFragment output fragment
 * @param inFragment input fragment
 * @param uniforms uniform variables
 */
void fragmentShader(OutFragment&outFragment,InFragment const&inFragment,ShaderInterface const&){
  auto const& vCoord = inFragment.attributes[0].v2;
  if(vCoord.y > vCoord.x && 1.f-vCoord.y>vCoord.x){
    outFragment.gl_FragColor = glm::vec4(0.f,0.f,1.f,1.f);
  }else{
    if(vCoord.y < 0.5f){
      outFragment.gl_FragColor = glm::vec4(1.f,0.f,0.f,1.f);
    }else{
      outFragment.gl_FragColor = glm::vec4(1.f,1.f,1.f,1.f);
    }
  }
}

Method::Method(MethodConstructionData const*){
  for(size_t y=0;y<NY;++y)
    for(size_t x=0;x<NX;++x){
      glm::vec2 coord;
      coord.x = static_cast<float>(x) / static_cast<float>(NX-1);
      coord.y = static_cast<float>(y) / static_cast<float>(NY-1);
      auto const flagStart = glm::vec2(-1.5f,-1.f);
      auto const flagSize  = glm::vec2(+3.0f,+2.f);

      auto const position = flagStart + coord*flagSize;

      vertices.push_back({position,coord});
    }

  for(uint32_t y=0;y<NY-1;++y)
    for(uint32_t x=0;x<NX-1;++x){
      indices.push_back((y+0)*NX+(x+0));
      indices.push_back((y+0)*NX+(x+1));
      indices.push_back((y+1)*NX+(x+0));
      indices.push_back((y+1)*NX+(x+0));
      indices.push_back((y+0)*NX+(x+1));
      indices.push_back((y+1)*NX+(x+1));
    }

  mem.buffers[0].data = vertices.data();
  mem.buffers[0].size = vertices.size() * sizeof(decltype(vertices)::value_type);

  mem.buffers[1].data = indices.data();
  mem.buffers[1].size = indices.size() * sizeof(decltype(indices)::value_type);

  mem.programs[0].vertexShader   = vertexShader;
  mem.programs[0].fragmentShader = fragmentShader;
  mem.programs[0].vs2fs[0]       = AttributeType::VEC2;

  VertexArray vao;
  vao.vertexAttrib[0].bufferID   = 0                  ;
  vao.vertexAttrib[0].type       = AttributeType::VEC2;
  vao.vertexAttrib[0].stride     = sizeof(Vertex)     ;
  vao.vertexAttrib[0].offset     = 0                  ;
  vao.vertexAttrib[1].bufferID   = 0                  ;
  vao.vertexAttrib[1].type       = AttributeType::VEC2;
  vao.vertexAttrib[1].stride     = sizeof(Vertex)     ;
  vao.vertexAttrib[1].offset     = sizeof(glm::vec2)  ;
  vao.indexBufferID = 1                ;
  vao.indexType     = IndexType::UINT32;

  pushClearCommand(commandBuffer,glm::vec4(.1,.1,.1,1));
  pushDrawCommand (commandBuffer,(NX-1)*(NY-1)*6,0,vao);

}

void Method::onUpdate(float dt){
  time += dt;
}

void Method::onDraw(Frame&frame,SceneParam const&sceneParam){
  mem.framebuffer = frame;
  mem.uniforms[0].m4 = sceneParam.proj*sceneParam.view;
  mem.uniforms[1].v1 = time;
  gpu_execute(mem,commandBuffer);
}

EntryPoint main = [](){registerMethod<Method>("izg07 czFlag");};

}

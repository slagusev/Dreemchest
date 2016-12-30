/**************************************************************************
 
 The MIT License (MIT)
 
 Copyright (c) 2015 Dmitry Sovetov
 
 https://github.com/dmsovetov
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 
 **************************************************************************/

#include "OpenGL2RenderingContext.h"
#include "../VertexBufferLayout.h"
#include "../VertexFormat.h"
#include "../CommandBuffer.h"

DC_BEGIN_DREEMCHEST

namespace Renderer
{

// ** createOpenGL2RenderingContext
RenderingContextPtr createOpenGL2RenderingContext(RenderViewPtr view)
{
    if (!OpenGL2::initialize())
    {
        return RenderingContextPtr();
    }

    return RenderingContextPtr(DC_NEW OpenGL2RenderingContext(view));
}
    
// ------------------------------------------------------ OpenGL2RenderingContext::ShaderPreprocessor -------------------------------------------------- //

// ** OpenGL2RenderingContext::ShaderPreprocessor::generateBufferDefinition
String OpenGL2RenderingContext::ShaderPreprocessor::generateBufferDefinition(const RenderingContext& renderingContext, const String& type, const String& name, s32 slot) const
{
    // A static array to map from an element type to GLSL data type.
    static const String s_types[] = 
    {
          "int"
        , "float"
        , "vec2"
        , "vec3"
        , "vec4"
        , "mat4"
    };

    // First find a uniform layout by name
    const UniformElement* elements = renderingContext.findUniformLayout(type);
    
    if (elements == NULL)
    {
        return "";
    }

    String definition = "struct " + type + " {\n";

    for (const UniformElement* element = elements; element->name; element++)
    {
        String def;
        
        if (element->size > 0)
        {
            def = String(element->name.value()) + "[" + toString(element->size) + "]";
        }
        else
        {
            def = element->name.value();
        }
        
        definition += "\t" + s_types[element->type] + " " + def + ";\n";
    }
    definition += "}; uniform " + type + " cb_" + toString(slot) + ";\n#define " + name + " cb_" + toString(slot) + "\n";

    return definition;
}

// ----------------------------------------------------------------- OpenGL2RenderingContext ----------------------------------------------------------- //

// ** OpenGL2RenderingContext::OpenGL2RenderingContext
OpenGL2RenderingContext::OpenGL2RenderingContext(RenderViewPtr view)
    : OpenGLRenderingContext(view)
{
    if (m_view.valid())
    {
        m_view->makeCurrent();
    }

    m_shaderLibrary.addPreprocessor(DC_NEW ShaderPreprocessor);
    
    m_textures.emplace(0, Texture());
    m_constantBuffers.emplace(0, ConstantBuffer());
    m_vertexBuffers.emplace(0, 0);
    m_indexBuffers.emplace(0, 0);
}

// ** OpenGL2RenderingContext::applyStateBlock
PipelineFeatures OpenGL2RenderingContext::applyStateBlock(const RenderFrame& frame, const StateBlock& stateBlock)
{
    const StateBlock* blocks[] = { &stateBlock };
    return applyStates(frame, blocks, 1).features;
}
    
// ** OpenGL2RenderingContext::acquireTexture
ResourceId OpenGL2RenderingContext::acquireTexture(u8 type, u16 width, u16 height, PixelFormat format)
{
    // First search for a free render target
    for (List<Texture_>::const_iterator i = m_transientTextures.begin(), end = m_transientTextures.end(); i != end; ++i)
    {
        // Get a texture info by id
        const TextureInfo& info = m_textureInfo[*i];
        
        // Does the render target format match the requested one?
        if (type == info.type && info.width == width && info.height == height && info.pixelFormat == format)
        {
            return *i;
        }
    }
    
    static CString s_textureType[] =
    {
        "1D", "2D", "3D", "cube"
    };
    
    LogVerbose("renderingContext", "allocating a transient %s texture of size %dx%d\n", s_textureType[type], width, height);
    return allocateTexture(type, NULL, width, height, 1, format, FilterLinear);
}

// ** OpenGL2RenderingContext::releaseTexture
void OpenGL2RenderingContext::releaseTexture(ResourceId id)
{
    m_transientTextures.push_back(Texture_::create(id));
}
    
// ** OpenGL2RenderingContext::allocateTexture
ResourceId OpenGL2RenderingContext::allocateTexture(u8 type, const void* data, u16 width, u16 height, u16 mipLevels, u16 pixelFormat, u8 filter, ResourceId id)
{
    // Allocate a resource identifier if it was not passed
    if (!id)
    {
        id = allocateIdentifier<Texture_>();
    }
    
    Texture texture;
    PixelFormat   format        = static_cast<PixelFormat>(pixelFormat);
    TextureFilter textureFilter = static_cast<TextureFilter>(filter);
    
    // Create a texture instance according to a type.
    switch (type)
    {
        case TextureType2D:
            texture.id     = OpenGL2::Texture::create2D(data, width, width, mipLevels, format, textureFilter);
            texture.target = GL_TEXTURE_2D;
            break;
            
        case TextureTypeCube:
            texture.id     = OpenGL2::Texture::createCube(data, width, mipLevels, format, textureFilter);
            texture.target = GL_TEXTURE_CUBE_MAP;
            break;
            
        default:
            NIMBLE_NOT_IMPLEMENTED
    }
    
    // Construct a texture info
    TextureInfo textureInfo;
    textureInfo.width       = width;
    textureInfo.height      = height;
    textureInfo.pixelFormat = format;
    textureInfo.type        = static_cast<TextureType>(type);
    
    // Save a created texture identifier and a texture info
    m_textures.emplace(id, texture);
    m_textureInfo.emplace(id, textureInfo);
    
    return id;
}

// ** OpenGL2RenderingContext::executeCommandBuffer
void OpenGL2RenderingContext::executeCommandBuffer(const RenderFrame& frame, const CommandBuffer& commands)
{
    RequestedState requestedState;
    GLuint         id;
    
    for (s32 i = 0, n = commands.size(); i < n; i++)
    {
        // Get a render operation at specified index
        const CommandBuffer::OpCode& opCode = commands.opCodeAt( i );
        
        // Perform a draw call
        switch(opCode.type)
        {
            case CommandBuffer::OpCode::Clear:
                OpenGL2::clear(opCode.clear.color, opCode.clear.mask, opCode.clear.depth, opCode.clear.stencil);
                break;
                
            case CommandBuffer::OpCode::Execute:
                execute(frame, *opCode.execute.commands);
                break;
                
            case CommandBuffer::OpCode::UploadConstantBuffer:
            {
                ConstantBuffer& constantBuffer = m_constantBuffers[opCode.upload.id];
                NIMBLE_ABORT_IF(static_cast<s32>(constantBuffer.data.size()) < opCode.upload.buffer.size, "buffer is too small");
                memcpy(&constantBuffer.data[0], opCode.upload.buffer.data, opCode.upload.buffer.size);
            }
                break;
                
            case CommandBuffer::OpCode::UploadVertexBuffer:
                OpenGL2::Buffer::subData(GL_ARRAY_BUFFER, m_vertexBuffers[opCode.upload.id], 0, opCode.upload.buffer.size, opCode.upload.buffer.data);
                break;
                
            case CommandBuffer::OpCode::CreateInputLayout:
                m_inputLayouts.emplace(opCode.createInputLayout.id, createVertexBufferLayout(opCode.createInputLayout.format));
                break;
                
            case CommandBuffer::OpCode::CreateTexture:
                allocateTexture(  opCode.createTexture.type
                                , opCode.createTexture.buffer.data
                                , opCode.createTexture.width
                                , opCode.createTexture.height
                                , opCode.createTexture.mipLevels
                                , opCode.createTexture.format
                                , opCode.createTexture.filter
                                , opCode.createTexture.id
                                );
                break;
                
            case CommandBuffer::OpCode::CreateIndexBuffer:
                id = OpenGL2::Buffer::create(GL_ARRAY_BUFFER, opCode.createBuffer.buffer.data, opCode.createBuffer.buffer.size, GL_DYNAMIC_DRAW);
                m_indexBuffers.emplace(opCode.createBuffer.id, id);
                break;
                
            case CommandBuffer::OpCode::CreateVertexBuffer:
                id = OpenGL2::Buffer::create(GL_ELEMENT_ARRAY_BUFFER, opCode.createBuffer.buffer.data, opCode.createBuffer.buffer.size, GL_DYNAMIC_DRAW);
                m_vertexBuffers.emplace(opCode.createBuffer.id, id);
                break;
                
            case CommandBuffer::OpCode::CreateConstantBuffer:
            {
                ConstantBuffer constantBuffer;
                //constantBuffer.layout = &m_uniformLayouts[opCode.createBuffer.layout][0];
                constantBuffer.layout = m_uniformLayouts[opCode.createBuffer.layout];
                constantBuffer.data.resize(opCode.createBuffer.buffer.size);
                
                if (opCode.createBuffer.buffer.data)
                {
                    memcpy(&constantBuffer.data[0], opCode.createBuffer.buffer.data, opCode.createBuffer.buffer.size);
                }
                m_constantBuffers.emplace(opCode.createBuffer.id, constantBuffer);
            }
                break;
                
            case CommandBuffer::OpCode::DeleteConstantBuffer:
                m_constantBuffers.emplace(opCode.id, ConstantBuffer());
                releaseIdentifier(RenderResourceType::ConstantBuffer, opCode.id);
                
                for (s32 i = 0; i < State::MaxConstantBuffers; i++)
                {
                    if (static_cast<ResourceId>(m_activeState.constantBuffer[i]) == opCode.id)
                    {
                        m_activeState.constantBuffer[i] = ConstantBuffer_();
                    }
                }
                break;
                
            case CommandBuffer::OpCode::DeleteProgram:
                if (static_cast<ResourceId>(m_activeState.program) == opCode.id)
                {
                    m_activeState.program = Program();
                }
                deleteProgram(opCode.id);
                releaseIdentifier(RenderResourceType::Program, opCode.id);
                break;
                
            case CommandBuffer::OpCode::AcquireTexture:
            {
                ResourceId id = acquireTexture(opCode.transientTexture.type, opCode.transientTexture.width, opCode.transientTexture.height, opCode.transientTexture.format);
                loadTransientResource(opCode.transientTexture.id, id);
            }
                break;
                
            case CommandBuffer::OpCode::ReleaseTexture:
            {
                ResourceId id = transientResource(opCode.transientTexture.id);
                releaseTexture(id);
                unloadTransientResource(opCode.transientTexture.id);
            }
                break;
                
            case CommandBuffer::OpCode::RenderToTexture:
            case CommandBuffer::OpCode::RenderToTransientTexture:
            {
                DC_CHECK_GL_CONTEXT;
                DC_CHECK_GL;
                
                // Get a transient resource id by a slot
                ResourceId id = opCode.type == CommandBuffer::OpCode::RenderToTexture ? opCode.renderToTextures.id : transientResource(opCode.renderToTextures.id);
                NIMBLE_ABORT_IF(!id, "invalid transient identifier");
                
                // Get a render target by an id.
                const Texture&     texture = m_textures[id];
                const TextureInfo& info    = m_textureInfo[id];
                
                // Save current viewport
                GLint prevViewport[4];
                glGetIntegerv(GL_VIEWPORT, prevViewport);
                
                // Save the bound framebuffer
                GLint prevFramebuffer;
                glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFramebuffer);
                
                // Acquire the framebuffer
                s32 framebufferIndex = acquireFramebuffer(info.width, info.height);
                
                if (!framebufferIndex)
                {
                    LogVerbose("opengl2", "allocating a framebuffer of size %dx%d\n", info.width, info.height);
                    GLuint id = OpenGL2::Framebuffer::create();
                    GLuint depth = OpenGL2::Framebuffer::renderbuffer(id, info.width, info.height, GL_DEPTH_ATTACHMENT, OpenGL2::textureInternalFormat(PixelD24X8));
                    framebufferIndex = allocateFramebuffer(id, depth, info.width, info.height);
                }

                OpenGL2::Framebuffer::bind(m_framebuffers[framebufferIndex].id);
                
                if (opCode.renderToTextures.side == 255)
                {
                    OpenGL2::Framebuffer::texture2D(texture.id, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0);
                }
                else
                {
                    OpenGL2::Framebuffer::texture2D(texture.id, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + opCode.renderToTextures.side, 0);
                }

                // Set a viewport before executing an attached command buffer
                const NormalizedViewport& viewport = opCode.renderToTextures.viewport;
                glViewport(viewport.x * info.width, viewport.y * info.height, viewport.width * info.width, viewport.height * info.height);
                
                // Execute an attached command buffer
                execute(frame, *opCode.renderToTextures.commands);
                
                // Release an acquired framebuffer
                releaseFramebuffer(framebufferIndex);
                
                // Disable the framebuffer
                OpenGL2::Framebuffer::bind(prevFramebuffer);
                
                // Restore the previous viewport
                glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
            }
                break;

            case CommandBuffer::OpCode::DrawIndexed:
                // Apply rendering states from a stack
                requestedState = applyStates(frame, opCode.drawCall.states, MaxStateStackDepth);
                
                // Now activate a matching shader permutation
                compilePipelineState(requestedState);
                
                // Perform an actual draw call
                OpenGL2::drawElements(opCode.drawCall.primitives, GL_UNSIGNED_SHORT, opCode.drawCall.first, opCode.drawCall.count);
                break;
                
            case CommandBuffer::OpCode::DrawPrimitives:
                // Apply rendering states from a stack
                requestedState = applyStates(frame, opCode.drawCall.states, MaxStateStackDepth);
                
                // Now activate a matching shader permutation
                compilePipelineState(requestedState);
                
                // Perform an actual draw call
                OpenGL2::drawArrays(opCode.drawCall.primitives, opCode.drawCall.first, opCode.drawCall.count);
                break;
                
            default:
                NIMBLE_NOT_IMPLEMENTED;
        }
    }
}
    
// ** OpenGL2RenderingContext::applyStates
OpenGL2RenderingContext::RequestedState OpenGL2RenderingContext::applyStates(const RenderFrame& frame, const StateBlock* const * stateBlocks, s32 count)
{
    State states[MaxStateChanges];
    PipelineFeatures userDefined;
    
    // This will be modified by a render state changes below
    RequestedState requestedState = m_activeState;
    
    // This will notify a pipeline that we started the stage change process
    s32 stateCount = startPipelineConfiguration(stateBlocks, count, states, MaxStateChanges, userDefined);
    
    // Apply all states
    for (s32 i = 0; i < stateCount; i++)
    {
        // Get a state at specified index
        const State& state = states[i];
        
        // And now apply it
        switch (state.type)
        {
            case State::BindVertexBuffer:
                requestedState.vertexBuffer.set(state.resourceId);
                break;
                
            case State::BindIndexBuffer:
                requestedState.indexBuffer.set(state.resourceId);
                break;
                
            case State::SetInputLayout:
                requestedState.inputLayout.set(state.resourceId);
                m_pipeline.activateVertexAttributes(m_inputLayouts[state.resourceId]->features());
                break;
                
            case State::SetFeatureLayout:
                requestedState.featureLayout.set(state.resourceId);
                m_pipeline.setFeatureLayout(m_pipelineFeatureLayouts[state.resourceId].get());
                break;
                
            case State::BindConstantBuffer:
                requestedState.constantBuffer[state.data.index].set(state.resourceId);
                m_pipeline.activateConstantBuffer(state.data.index);
                break;
                
            case State::BindProgram:
                requestedState.program.set(state.resourceId);
                m_pipeline.setProgram(requestedState.program);
                break;
                
            case State::Blending:
            {
                // Decode blend factors from a command
                BlendFactor source      = state.sourceBlendFactor();
                BlendFactor destination = state.destBlendFactor();
                
                // Apply the blend state
                if(source == BlendDisabled || destination == BlendDisabled)
                {
                    glDisable(GL_BLEND);
                }
                else
                {
                    glEnable(GL_BLEND);
                    glBlendFunc(OpenGL2::convertBlendFactor(source), OpenGL2::convertBlendFactor(destination));
                }
            }
                break;
                
            case State::PolygonOffset:
            {
                f32 factor = state.polygonOffsetFactor();
                f32 units  = state.polygonOffsetUnits();
                
                if (equal3(factor, units, 0.0f))
                {
                    glDisable(GL_POLYGON_OFFSET_FILL);
                } else
                {
                    glEnable(GL_POLYGON_OFFSET_FILL);
                    glPolygonOffset(factor, units);
                }
            }
                break;
                
            case State::DepthState:
                glDepthMask(state.data.depthWrite ? GL_TRUE : GL_FALSE);
                glEnable(GL_DEPTH_TEST);
                glDepthFunc(OpenGL2::convertCompareFunction(state.function()));
                break;
                
            case State::AlphaTest:
                if (state.compareFunction == CompareDisabled)
                {
                    glDisable(GL_ALPHA_TEST);
                }
                else
                {
                    glEnable(GL_ALPHA_TEST);
                    glAlphaFunc(OpenGL2::convertCompareFunction(state.function()), state.alphaReference());
                }
                break;
                
            case State::CullFace:
                if (state.cullFace == TriangleFaceNone)
                {
                    glDisable(GL_CULL_FACE);
                }
                else
                {
                    glEnable(GL_CULL_FACE);
                    glFrontFace(GL_CCW);
                    glCullFace(OpenGL2::convertTriangleFace(static_cast<TriangleFace>(state.cullFace)));
                }
                break;
                
            case State::BindTexture:
            {
                s32 samplerIndex = state.samplerIndex();
                requestedState.texture[samplerIndex].set(state.resourceId);
                m_pipeline.activateSampler(samplerIndex);
            }
                break;
                
            case State::BindTransientTexture:
            {
                s32 samplerIndex = state.samplerIndex();
                ResourceId id = transientResource(state.resourceId);
                requestedState.texture[samplerIndex].set(id);
                m_pipeline.activateSampler(samplerIndex);
            }
                break;
                
            case State::Rasterization:
                switch (state.rasterization)
                {
                    case PolygonFill:
                        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                        break;
                    case PolygonWire:
                        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                        break;
                    default:
                        NIMBLE_NOT_IMPLEMENTED
                }
                break;
                
            case State::StencilOp:
                glStencilOp(OpenGL2::convertStencilAction(state.stencilFail()), OpenGL2::convertStencilAction(state.depthFail()), OpenGL2::convertStencilAction(state.depthStencilPass()));
                break;
                
            case State::StencilFunc:
                if (state.stencilFunction.op == CompareDisabled)
                {
                    glDisable(GL_STENCIL_TEST);
                }
                else
                {
                    glEnable(GL_STENCIL_TEST);
                    glStencilFunc(OpenGL2::convertCompareFunction(static_cast<Compare>(state.stencilFunction.op)), state.data.ref, state.stencilFunction.mask);
                }
                break;
                
            case State::StencilMask:
                NIMBLE_NOT_IMPLEMENTED;
                break;
                
            case State::ColorMask:
                glColorMask(state.mask & ColorMaskRed, state.mask & ColorMaskGreen, state.mask & ColorMaskBlue, state.mask & ColorMaskAlpha);
                break;

            default:
                NIMBLE_NOT_IMPLEMENTED
        }
    }
    
    // Finish pipeline configuration and store the final features bitmask
    requestedState.features = finishPipelineConfiguration(userDefined);
    
    return requestedState;
}

// ** OpenGL2RenderingContext::compilePipelineState
void OpenGL2RenderingContext::compilePipelineState(RequestedState requested)
{
    NIMBLE_ABORT_IF(!requested.inputLayout, "no valid input layout set");
    NIMBLE_ABORT_IF(!requested.vertexBuffer, "no valid vertex buffer set");
    
    // Bind an indexed buffer
    if (requested.indexBuffer != m_activeState.indexBuffer)
    {
        OpenGL2::Buffer::bind(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffers[requested.indexBuffer]);
    }
    
    // Bind a vertex buffer
    if (requested.vertexBuffer != m_activeState.vertexBuffer)
    {
        OpenGL2::Buffer::bind(GL_ARRAY_BUFFER, m_vertexBuffers[requested.vertexBuffer]);
    }
    
    // Bind texture samplers
    for (s32 i = 0; i < State::MaxTextureSamplers; i++)
    {
        if (requested.texture[i] == m_activeState.texture[i])
        {
        //    continue;
        }
        
        const Texture& texture = m_textures[requested.texture[i]];
        OpenGL2::Texture::bind(texture.target, texture.id, i);
    }
    
    // Switch the input layout
    if (requested.inputLayout != m_activeState.inputLayout)
    {
        // Disable the previous input layout
        if (m_activeState.inputLayout)
        {
            OpenGL2::disableInputLayout(*m_inputLayouts[m_activeState.inputLayout]);
        }
        
        // Now enable a new one
        OpenGL2::enableInputLayout(NULL, *m_inputLayouts[requested.inputLayout]);
    }
    
    Program          program  = m_pipeline.program();
    PipelineFeatures features = m_pipeline.features();
    
    // Get an active program
    NIMBLE_ABORT_IF(!program && !m_defaultProgram, "no valid program set and no default one specified");
    
    // Use a default program if nothing was set by a user
    if (!program)
    {
        program = m_defaultProgram;
    }
    
    // Switch the program one the pipeline state was changed
    if (m_pipeline.changes())
    {
        // Now compile a program permutation
        GLuint activeProgram = compileShaderPermutation(program, features, m_pipeline.featureLayout());
        OpenGL2::Program::use(activeProgram);
        
        // Accept these changes
        m_pipeline.acceptChanges();
    }

    // Update all uniforms
    updateUniforms(requested, features, program);
    
    // Update an active rendering state
    m_activeState = requested;
}
    
// ** OpenGL2RenderingContext::updateUniforms
void OpenGL2RenderingContext::updateUniforms(const RequestedState& state, PipelineFeatures features, Program program)
{
    // Bind texture samplers
    static FixedString s_samplers[] =
    {
          "Texture0"
        , "Texture1"
        , "Texture2"
        , "Texture3"
        , "Texture4"
        , "Texture5"
        , "Texture6"
        , "Texture7"
    };
    
    s32 nSamplers = sizeof(s_samplers) / sizeof(s_samplers[1]);
    NIMBLE_ABORT_IF(nSamplers != State::MaxTextureSamplers, "invalid sampler name initialization");
    
    for (s32 i = 0, n = nSamplers; i < n; i++ )
    {
        GLint location = findUniformLocation(program, features, s_samplers[i]);
        
        if (location)
        {
            OpenGL2::Program::uniform1i(location, i);
        }
    }
    
    // Process each bound constant buffer
    for (s32 i = 0; i < State::MaxConstantBuffers; i++)
    {
        // No constant buffer bound to this slot
        if (!state.constantBuffer[i])
        {
            continue;
        }
        
        // Get a constant buffer at index
        const ConstantBuffer& constantBuffer = m_constantBuffers[state.constantBuffer[i]];

        // Submit all constants to a shader
        for (const UniformElement* constant = &constantBuffer.layout[0]; constant->name; constant++)
        {
            // Create a uniform name here for now, but in future this sould be cached somewhere (probably in a ConstantBuffer instance).
            String uniform = "cb_" + toString(i) + "." + constant->name.value();

            // Lookup a uniform location by name
            GLint location = findUniformLocation(program, features, uniform.c_str());
            
            // Not found - skip
            if (location == 0)
            {
            //    LogWarning("opengl2", "a uniform location '%s' for constant buffer %d could not be found\n", constant->name.value(), i);
                continue;
            }
            
            // Submit constant to a shader
            switch (constant->type)
            {
                case UniformElement::Integer:
                    OpenGL2::Program::uniform1i(location, *reinterpret_cast<const s32*>(&constantBuffer.data[constant->offset]));
                    break;
                    
                case UniformElement::Float:
                    OpenGL2::Program::uniform1f(location, *reinterpret_cast<const f32*>(&constantBuffer.data[constant->offset]));
                    break;
                    
                case UniformElement::Vec2:
                    OpenGL2::Program::uniform2f(location, reinterpret_cast<const f32*>(&constantBuffer.data[constant->offset]), constant->size);
                    break;
                    
                case UniformElement::Vec3:
                    OpenGL2::Program::uniform3f(location, reinterpret_cast<const f32*>(&constantBuffer.data[constant->offset]), constant->size);
                    break;
                    
                case UniformElement::Vec4:
                    OpenGL2::Program::uniform4f(location, reinterpret_cast<const f32*>(&constantBuffer.data[constant->offset]), constant->size);
                    break;
                    
                case UniformElement::Matrix4:
                    OpenGL2::Program::uniformMatrix4(location, reinterpret_cast<const f32*>(&constantBuffer.data[constant->offset]));
                    break;
            }
        }
    }
}
    
// ** OpenGL2RenderingContext::compileShaderPermutation
GLuint OpenGL2RenderingContext::compileShaderPermutation(Program program, PipelineFeatures features, const PipelineFeatureLayout* featureLayout)
{
    // Lookup a shader permutation in cache
    const Permutation* permutation = NULL;

    if (lookupPermutation(program, features, &permutation))
    {
        return permutation->program;
    }

    // Now create a shader source code from a descriptor
    String shaderSourceCode[TotalShaderTypes];
    m_shaderLibrary.generateShaderCode(m_programs[program], features, featureLayout, shaderSourceCode);

    // Nothing found so we have to compile a new one
    s8 error[2048];
    
    // Compile the vertex shader
    GLuint vertexShader = OpenGL2::Program::compileShader(GL_VERTEX_SHADER, shaderSourceCode[VertexShaderType].c_str(), error, sizeof(error));
    if (vertexShader == 0)
    {
        return 0;
    }
    
    // Compile the fragment shader
    GLuint fragmentShader = OpenGL2::Program::compileShader(GL_FRAGMENT_SHADER, shaderSourceCode[FragmentShaderType].c_str(), error, sizeof(error));
    if (fragmentShader == 0)
    {
        OpenGL2::Program::deleteShader(vertexShader);
        return 0;
    }
    
    // Now link a program
    GLuint shaders[] = {vertexShader, fragmentShader};
    GLuint id = OpenGL2::Program::createProgram(shaders, 2, error, sizeof(error));
    
    if (id == 0)
    {
        OpenGL2::Program::deleteShader(vertexShader);
        OpenGL2::Program::deleteShader(fragmentShader);
        return 0;
    }
    
    // Finally save a compiled permutation
    savePermutation(program, features, id);
    
    return id;
}
    
} // namespace Renderer

DC_END_DREEMCHEST

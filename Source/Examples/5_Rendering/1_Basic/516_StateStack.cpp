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

#include <Dreemchest.h>
#include "../Examples.h"

DC_USE_DREEMCHEST

using namespace Platform;
using namespace Renderer;

// A colored vertex data structure.
struct PosColorVertex
{
    f32 position[3];
    u32 color;
};

static PosColorVertex s_vertices[] =
{
    { { -1.0f,  1.0f,  1.0f }, 0xff000000 },
    { {  1.0f,  1.0f,  1.0f }, 0xff0000ff },
    { { -1.0f, -1.0f,  1.0f }, 0xff00ff00 },
    { {  1.0f, -1.0f,  1.0f }, 0xff00ffff },
    { { -1.0f,  1.0f, -1.0f }, 0xffff0000 },
    { {  1.0f,  1.0f, -1.0f }, 0xffff00ff },
    { { -1.0f, -1.0f, -1.0f }, 0xffffff00 },
    { {  1.0f, -1.0f, -1.0f }, 0xffffffff },
};

static const u16 s_indices[] =
{
    0, 1, 2,
    1, 3, 2,
    4, 6, 5,
    5, 6, 7,
    0, 2, 4,
    4, 2, 6,
    1, 5, 3,
    5, 7, 3,
    0, 4, 1,
    4, 5, 1,
    2, 3, 6,
    6, 3, 7,
};

static String s_vertexShader =
    "cbuffer Projection projection : 0;             \n"
    "cbuffer Camera     camera     : 1;             \n"
    "cbuffer Instance   instance   : 2;             \n"

    "varying vec4 v_color;                          \n"

    "void main()                                    \n"
    "{                                              \n"
    "    v_color     = gl_Color;                    \n"
    "    gl_Position = projection.transform         \n"
    "                * camera.transform             \n"
    "                * instance.transform           \n"
    "                * gl_Vertex                    \n"
    "                ;                              \n"
    "}                                              \n"
    ;

static String s_fragmentShader =
    "varying vec4 v_color;                          \n"

    "void main()                                    \n"
    "{                                              \n"
    "    gl_FragColor = v_color;                    \n"
    "}                                              \n"
    ;

class RenderStateStack : public RenderingApplicationDelegate
{
    StateBlock8 m_renderStates;
    ConstantBuffer_ m_instanceConstantBuffer;
    
    virtual void handleLaunched(Application* application) NIMBLE_OVERRIDE
    {
        Logger::setStandardLogger();

        if (!initialize(800, 600))
        {
            application->quit(-1);
        }
        
        // Create a cube vertex and index buffers
        {
            InputLayout inputLayout = m_renderingContext->requestInputLayout(VertexFormat::Position | VertexFormat::Color);
            VertexBuffer_ vertexBuffer = m_renderingContext->requestVertexBuffer(s_vertices, sizeof(s_vertices));
            IndexBuffer_ indexBuffer = m_renderingContext->requestIndexBuffer(s_indices, sizeof(s_indices));
            
            // And bind them to a state block
            m_renderStates.bindVertexBuffer(vertexBuffer);
            m_renderStates.bindIndexBuffer(indexBuffer);
            m_renderStates.bindInputLayout(inputLayout);
        }
        
        // Create projection constant buffer
        {
            Examples::Projection projection     = Examples::Projection::perspective(60.0f, m_window->width(), m_window->height(), 0.1f, 100.0f);
            UniformLayout        layout         = m_renderingContext->requestUniformLayout("Projection", Examples::Projection::Layout);
            ConstantBuffer_      constantBuffer = m_renderingContext->requestConstantBuffer(&projection, sizeof(projection), layout);
            
            // And bind it to a state block
            m_renderStates.bindConstantBuffer(constantBuffer, 0);
        }
        
        // Create a camera constant buffer
        {
            Examples::Camera camera         = Examples::Camera::lookAt(Vec3(0.0f, 0.0f, -35.0f), Vec3(0.0f, 0.6f, 0.0f));
            UniformLayout    layout         = m_renderingContext->requestUniformLayout("Camera", Examples::Camera::Layout);
            ConstantBuffer_ constantBuffer  = m_renderingContext->requestConstantBuffer(&camera, sizeof(camera), layout);
            
            // And bind it to a state block
            m_renderStates.bindConstantBuffer(constantBuffer, 1);
        }
        
        // Finally create an empty instance constant buffer
        {
            UniformLayout layout = m_renderingContext->requestUniformLayout("Instance", Examples::Instance::Layout);
            m_instanceConstantBuffer = m_renderingContext->requestConstantBuffer(NULL, sizeof(Examples::Instance), layout);
        }
        
        // Create and bind the default shader program
        Program program = m_renderingContext->requestProgram(s_vertexShader, s_fragmentShader);
        m_renderStates.bindProgram(program);
    }
 
    virtual void handleRenderFrame(const Window::Update& e) NIMBLE_OVERRIDE
    {
        RenderFrame frame(m_renderingContext->defaultStateBlock());
        
        // Get an entry point command buffer as usual
        RenderCommandBuffer& commands = frame.entryPoint();
        
        // Here we take a reference to a render state stack...
        StateStack& stateStack = frame.stateStack();
        
        // ...and then push a default state block
        StateScope global = stateStack.push(&m_renderStates);

        commands.clear(Rgba(0.3f, 0.3f, 0.3f), ClearAll);
        
        f32 time = currentTime() * 0.001f;
        
        // Now render cubes
        for (s32 y = 0; y < 11; ++y)
        {
            for (s32 x = 0; x < 11; ++x)
            {
                // Construct an instance data from a transform matrix
                Examples::Instance instance = Examples::Instance::fromTransform(Matrix4::translation(x * 3.0f - 15.0f, y * 3.0f - 15.0f, 0.0f) *
                                                                                Matrix4::rotateXY(time + x*0.21f, time + y*0.37f));

                // Now push a new state scope to a stack, which will be automatically poped from stack
                StateScope instanceStates = stateStack.newScope();
                instanceStates->bindConstantBuffer(m_instanceConstantBuffer, 2);
                
                // Upload instance data and render a cube
                commands.uploadConstantBuffer(m_instanceConstantBuffer, &instance, sizeof(instance));
                commands.drawIndexed(0, Renderer::PrimTriangles, 0, sizeof(s_indices) / sizeof(u16));
            }
        }
        
        m_renderingContext->display(frame);
    }
};

dcDeclareApplication(new RenderStateStack)
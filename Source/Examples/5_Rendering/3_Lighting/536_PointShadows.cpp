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
#include <Framework.h>

DC_USE_DREEMCHEST

using namespace Platform;
using namespace Renderer;

static String s_vertexShader =
    "cbuffer Projection projection : 0;                                         \n"
    "cbuffer Camera     camera     : 1;                                         \n"
    "cbuffer Instance   instance   : 2;                                         \n"
    "                                                                           \n"
    "attribute vec4 a_position;                                                 \n"
    "attribute vec3 a_normal;                                                   \n"
    "                                                                           \n"
    "varying vec3 v_wsVertex;                                                   \n"
    "varying vec3 v_wsNormal;                                                   \n"
    "                                                                           \n"
    "void main()                                                                \n"
    "{                                                                          \n"
    "   v_wsNormal  = (instance.inverseTranspose                                \n"
    "               * vec4(a_normal, 1.0)).xyz                                  \n"
    "               ;                                                           \n"

    "   v_wsVertex  = (instance.transform * a_position).xyz;                    \n"

    "   gl_Position = projection.transform                                      \n"
    "               * camera.transform                                          \n"
    "               * vec4(v_wsVertex, 1.0)                                     \n"
    "               ;                                                           \n"
    "}                                                                          \n"
    ;

static String s_fragmentShader =
    "cbuffer Camera     camera   : 1;                                           \n"
    "cbuffer Instance   instance : 2;                                           \n"
    "cbuffer Light      light    : 3;                                           \n"
    "cbuffer Shadow     shadow   : 4;                                           \n"

    "uniform samplerCube Texture0;                                              \n"

    "varying vec3 v_wsVertex;                                                   \n"
    "varying vec3 v_wsNormal;                                                   \n"

    "float attenuation(vec3 point, vec3 light, vec4 factors)                    \n"
    "{                                                                          \n"
    "   float d = length(point - light);                                        \n"
    "   return 1.0 / (factors.x + factors.y * d + factors.z * d * d);           \n"
    "}                                                                          \n"

    "vec2 phong(vec3 L, vec3 N, vec3 V)                                         \n"
    "{                                                                          \n"
    "   float diffuse  = max(dot(L, N), 0.0);                                   \n"
    "   vec3  R        = reflect(L, N);                                         \n"
    "   float specular = max(dot(V, R), 0.0);                                   \n"
    "   return vec2(diffuse, specular);                                         \n"
    "}                                                                          \n"

    "vec3 lightColor(vec3 L, vec3 N, vec3 V)                                    \n"
    "{                                                                          \n"
    "   vec2 l = phong(L, N, V);                                                \n"
    "   return light.color * (l.x + pow(l.y, 32.0));                            \n"
    "}                                                                          \n"


    "float sampleShadow(vec3 L, float n, float f)                               \n"
    "{                                                                          \n"
    "   float current = length(L);                                              \n"
    "   float depth   = textureCube(Texture0, L).r;                             \n"
    "   float closest = depth * shadow.far;                                     \n"
    "   return current < closest ? 1.0 : 0.0;                                   \n"
    "}                                                                          \n"

    "void main()                                                                            \n"
    "{                                                                                      \n"
    "   vec3 normal     = normalize(v_wsNormal);                                            \n"
    "   vec3 view       = normalize(v_wsVertex - camera.position);                          \n"
    "   vec3 L          = normalize(light.position - v_wsVertex);                           \n"

    "   float s     = sampleShadow(v_wsVertex - light.position, shadow.near, shadow.far);   \n"
    "   float att   = attenuation(v_wsVertex, light.position, light.attenuation);           \n"
    "   vec2  l     = phong(L, normal, view);                                               \n"
    "   vec3  color = s * att * light.intensity * light.color * (l.x + pow(l.y, 32.0));     \n"

    "   gl_FragColor = vec4(color, 1.0);                                                    \n"
    "}                                                                                      \n"
    ;

static String s_vertexShadow =
    "cbuffer Instance instance  : 2;                                            \n"
    "cbuffer Shadow   shadow    : 4;                                            \n"

    "attribute vec4 a_position;                                                 \n"

    "varying vec4 v_wsVertex;                                                   \n"

    "void main()                                                                \n"
    "{                                                                          \n"
    "   v_wsVertex  = instance.transform * a_position;                          \n"
    "   gl_Position = shadow.projection * shadow.view * instance.transform * a_position;        \n"
    "}                                                                          \n"
    ;


static String s_fragmentShadow =
    "cbuffer Shadow   shadow    : 4;                                            \n"
    "varying vec4 v_wsVertex;                                                   \n"

    "void main()                                                                \n"
    "{                                                                          \n"
    "   float d      = length(v_wsVertex.xyz - shadow.position);                \n"
    "   gl_FragColor = vec4((d - shadow.near) / (shadow.far - shadow.near));    \n"
    "}                                                                          \n"
    ;

struct Light
{
    Vec3    position;
    Rgb     color;
    f32     intensity;
    Vec4    attenuation;
    static UniformElement Layout[];
} s_light;

UniformElement Light::Layout[] =
{
      { "position",    UniformElement::Vec3,  offsetof(Light, position)     }
    , { "color",       UniformElement::Vec3,  offsetof(Light, color)        }
    , { "intensity",   UniformElement::Float, offsetof(Light, intensity)    }
    , { "attenuation", UniformElement::Vec4,  offsetof(Light, attenuation)  }
    , { NULL }
};

struct Shadow
{
    Matrix4 projection;
    Matrix4 view;
    Vec3    position;
    f32     near;
    f32     far;
    static UniformElement Layout[];
};

UniformElement Shadow::Layout[] =
{
      { "projection",  UniformElement::Matrix4, offsetof(Shadow, projection)    }
    , { "view",        UniformElement::Matrix4, offsetof(Shadow, view)          }
    , { "position",    UniformElement::Vec3,    offsetof(Shadow, position)      }
    , { "near",        UniformElement::Float,   offsetof(Shadow, near)          }
    , { "far",         UniformElement::Float,   offsetof(Shadow, far)           }
    , { NULL }
};

class PointLights : public Framework::ApplicationDelegate
{
    StateBlock8     m_renderStates;
    ConstantBuffer_ m_lightConstantBuffer;
    ConstantBuffer_ m_shadowConstantBuffer;
    Program         m_programShadow;
    Texture_        m_envmap;
    
    virtual void handleLaunched(Application* application) NIMBLE_OVERRIDE
    {
        Logger::setStandardLogger();
        
        if (!initialize(800, 600))
        {
            application->quit(-1);
        }
        
        m_envmap = m_renderingContext->requestTextureCube(NULL, 512, 1, TextureRgba8);
        
        setCameraPosition(Vec3(0.0f, 2.0f, -2.0f));

        // Configure a light constant buffer
        {
            m_lightConstantBuffer = m_renderingContext->requestConstantBuffer(NULL, sizeof(Light), "Light", Light::Layout);
            m_renderStates.bindConstantBuffer(m_lightConstantBuffer, 3);
        }
        
        // Configure a shadow constant buffer
        {
            m_shadowConstantBuffer = m_renderingContext->requestConstantBuffer(NULL, sizeof(Shadow), "Shadow", Shadow::Layout);
            m_renderStates.bindConstantBuffer(m_shadowConstantBuffer, 4);
        }
        
        // Create a simple shader program
        Program program = m_renderingContext->requestProgram(s_vertexShader, s_fragmentShader);
        m_renderStates.bindProgram(program);

        m_programShadow = m_renderingContext->requestProgram(s_vertexShadow, s_fragmentShadow);
    }
    
    virtual void handleRenderFrame(RenderFrame& frame, StateStack& stateStack, RenderCommandBuffer& commands, f32 dt) NIMBLE_OVERRIDE
    {
        // Push the default state
        StateScope defaultScope = stateStack.push(&m_renderStates);
        
        // Update light parameters
        {
            f32 st              = sinf(time() * 0.1f);
            f32 ct              = cosf(time() * 0.1f);
            s_light.position    = Vec3(st, 1.5f, ct) * 0.7f;
            s_light.attenuation = Vec4(0.0f, 0.0f, 1.0f);
            s_light.color       = Rgb(1.0f, 1.0f, 1.0f);
            s_light.intensity   = 0.5f;
            commands.uploadConstantBuffer(m_lightConstantBuffer, &s_light, sizeof(s_light));
        }
        
        // Clear the viewport
        commands.clear(Rgba(0.3f, 0.3f, 0.3f), ClearAll);

        TransientTexture shadow = renderShadow(commands, stateStack, 512, s_light.position);
        StateScope shadowScope = stateStack.newScope();
        shadowScope->bindTexture(shadow, 0);
        renderColumnsScene(commands);
        commands.releaseTexture(shadow);
        
        // Render light sources for debugging
        renderPinkItem(commands, stateStack, m_sphere, Matrix4::translation(s_light.position) * Matrix4::scale(0.05f, 0.05f, 0.05f));

        m_renderingContext->display(frame);
    }
    
    TransientTexture renderShadow(RenderCommandBuffer& commands, StateStack& stateStack, s32 size, const Vec3& point)
    {
        // Declare an array of camera up vectors to
        Vec3 up[] =
        {
              -Vec3::axisY()
            , -Vec3::axisY()
            ,  Vec3::axisZ()
            , -Vec3::axisZ()
            , -Vec3::axisY()
            , -Vec3::axisY()
        };
        
        Vec3 target[] =
        {
               Vec3::axisX()
            , -Vec3::axisX()
            ,  Vec3::axisY()
            , -Vec3::axisY()
            ,  Vec3::axisZ()
            , -Vec3::axisZ()
        };
        
        // Render the shadow cube map
        TransientTexture shadow = commands.acquireTextureCube(size, TextureR16F | TextureLinear);
        
        // Push a program state
        StateScope shadowScope = stateStack.newScope();
        shadowScope->bindProgram(m_programShadow);
        shadowScope->setCullFace(TriangleFaceFront);
        
        // Render each cube map face
        for (s32 i = 0; i < 6; i++)
        {
            RenderCommandBuffer& renderShadow = commands.renderToCubeMap(shadow, i, TextureD16);
                
            Shadow shadow;
            shadow.near       = 0.0f;
            shadow.far        = 10.0f;
            shadow.position   = point;
            shadow.projection = Matrix4::perspective(90.0f, 1.0f, shadow.near, shadow.far);
            shadow.view       = Matrix4::lookAt(point, point + target[i], up[i]);
            
            renderShadow.clear(Rgba(0.0f, 0.0f, 0.0f), ClearAll);
            renderShadow.uploadConstantBuffer(m_shadowConstantBuffer, &shadow, sizeof(shadow));
            renderColumnsScene(renderShadow);
        }
        
        return shadow;
    }
};

dcDeclareApplication(new PointLights)

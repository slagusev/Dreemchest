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

#include "RenderSystem.h"

DC_BEGIN_DREEMCHEST

namespace Scene {

// ** RenderSystemBase::RenderSystemBase
RenderSystemBase::RenderSystemBase( RenderingContext& context, Ecs::IndexPtr cameras )
    : m_context( context )
    , m_cameras( cameras )
{
}

// ** RenderSystemBase::render
void RenderSystemBase::render( void )
{
    // Get the command buffer from a context
    Commands& commands = m_context.commands();

    // Get all cameras eligible for rendering by this system
    const Ecs::EntitySet& cameras = m_cameras->entities();

    // Process each camera
    for( Ecs::EntitySet::const_iterator i = cameras.begin(), end = cameras.end(); i != end; ++i ) {
        // Get the camera entity
        const Ecs::Entity& entity = *i->get();

        // Get Camera component from an entity
        const Camera& camera = *entity.get<Camera>();

        // Get Transform component from an entity
        const Transform& transform = *entity.get<Transform>();

        // Push render target before rendering
        commands.emitPushRenderTarget( camera.target(), camera.calculateViewProjection( transform.matrix() ), camera.viewport() );

        // Emit render operations for this camera
        emitRenderOperations( entity, camera, transform );

        // Pop render target when rendering is finished
        commands.emitPopRenderTarget();
    }
}

} // namespace Scene

DC_END_DREEMCHEST
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

#include "RenderFrame.h"
#include "CommandBuffer.h"

DC_BEGIN_DREEMCHEST

namespace Renderer
{

// ** RenderFrame::RenderFrame
RenderFrame::RenderFrame( void )
    : m_stateStack( 4096, MaxStateStackDepth )
    , m_allocator( 1024 * 100 )
{
    m_entryPoint = &createCommandBuffer();
}

// ** RenderFrame::internBuffer
const void* RenderFrame::internBuffer( const void* data, s32 size )
{
    void* interned = allocate( size );
    memcpy( interned, data, size );
    return interned;
}

// ** RenderFrame::allocate
void* RenderFrame::allocate( s32 size )
{
    void* allocated = m_allocator.allocate( size );
    NIMBLE_ABORT_IF( allocated == NULL, "render frame is out of memory" );
    return allocated;
}

// ** RenderFrame::entryPoint
const CommandBuffer& RenderFrame::entryPoint( void ) const
{
    return *m_entryPoint;
}

// ** RenderFrame::entryPoint
CommandBuffer& RenderFrame::entryPoint( void )
{
    return *m_entryPoint;
}

// ** RenderFrame::createCommandBuffer
CommandBuffer& RenderFrame::createCommandBuffer( void )
{
    m_commandBuffers.push_back( DC_NEW CommandBuffer );
    return *m_commandBuffers.back().get();
}

// ** RenderFrame::stateStack
StateStack& RenderFrame::stateStack( void )
{
    return m_stateStack;
}
    
// ** RenderFrame::clear
void RenderFrame::clear( void )
{
    m_commandBuffers.clear();
    m_entryPoint = &createCommandBuffer();

    m_allocator.reset();
}

} // namespace Renderer

DC_END_DREEMCHEST
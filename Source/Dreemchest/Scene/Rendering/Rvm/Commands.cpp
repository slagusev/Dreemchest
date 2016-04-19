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

#include "Commands.h"

DC_BEGIN_DREEMCHEST

namespace Scene {

// ---------------------------------------------------------------------- RenderCommandBuffer --------------------------------------------------------------------- //

// ** RenderCommandBuffer::drawIndexed
void RenderCommandBuffer::drawIndexed( u32 sorting, Renderer::PrimitiveType primitives, const RenderStateBlock* states[RenderStateStack::Size], s32 first, s32 count )
{
    OpCode opCode;
    opCode.type         = OpCode::DrawIndexed;
    opCode.sorting      = sorting;
    opCode.primitives   = primitives;
    opCode.first        = first;
    opCode.count        = count;
    memcpy( opCode.states, states, sizeof opCode.states );

    m_commands.push_back( opCode );
}

// ** RenderCommandBuffer::drawPrimitives
void RenderCommandBuffer::drawPrimitives( u32 sorting, Renderer::PrimitiveType primitives, const RenderStateBlock* states[RenderStateStack::Size], s32 first, s32 count )
{
    OpCode opCode;
    opCode.type         = OpCode::DrawPrimitives;
    opCode.sorting      = sorting;
    opCode.primitives   = primitives;
    opCode.first        = first;
    opCode.count        = count;
    memcpy( opCode.states, states, sizeof opCode.states );

    m_commands.push_back( opCode );
}

} // namespace Scene

DC_END_DREEMCHEST
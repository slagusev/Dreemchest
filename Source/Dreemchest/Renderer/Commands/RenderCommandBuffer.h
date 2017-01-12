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

#ifndef __DC_Renderer_RenderCommandBuffer_H__
#define __DC_Renderer_RenderCommandBuffer_H__

#include "CommandBuffer.h"

DC_BEGIN_DREEMCHEST

namespace Renderer
{
    //! A command buffer that is generated by render pass and executed by rendering context.
    class RenderCommandBuffer : public CommandBuffer
    {
    friend class RenderFrame;
    public:
        
        //! Emits a render target clear command.
        void                        clear(const Rgba& clearColor, u8 clearMask);
        
        //! Emits an acquire transient texture 2D command.
        TransientTexture            acquireTexture2D(u16 width, u16 height, PixelFormat format);
        
        //! Emits an acquire transient cube texture command.
        TransientTexture            acquireTextureCube(u16 size, PixelFormat format);
        
        //! Emits a release an transient render target command.
        void                        releaseTexture(TransientTexture id);
        
        //! Emits a rendering to a viewport of a specified render target command.
        RenderCommandBuffer&        renderToTexture(TransientTexture id, const Rect& viewport = Rect(0.0f, 0.0f, 1.0f, 1.0f));
        
        //! Emits a command to start rendering to a viewport of a specified cube map side.
        RenderCommandBuffer&        renderToCubeMap(TransientTexture id, u8 side, const Rect& viewport = Rect(0.0f, 0.0f, 1.0f, 1.0f));
        
        //! Emits a command to start rendering to a viewport of a specified cube map side.
        RenderCommandBuffer&        renderToCubeMap(Texture_ id, u8 side, const Rect& viewport = Rect(0.0f, 0.0f, 1.0f, 1.0f));
        
        //! Emits a rendering to a viewport.
        RenderCommandBuffer&        renderToTarget(const Rect& viewport = Rect(0.0f, 0.0f, 1.0f, 1.0f));
        
        //! Emits a draw indexed command that inherits all rendering states from a state stack.
        void                        drawIndexed(u32 sorting, PrimitiveType primitives, s32 first, s32 count);
        
        //! Emits a draw indexed command with a single render state block.
        void                        drawIndexed(u32 sorting, PrimitiveType primitives, s32 first, s32 count, const StateBlock& stateBlock);
        
        //! Emits a draw primitives command that inherits all rendering states from a state stack.
        void                        drawPrimitives(u32 sorting, PrimitiveType primitives, s32 first, s32 count);
        
        //! Emits a draw primitives command that inherits all rendering states from a state stack.
        void                        drawPrimitives(u32 sorting, PrimitiveType primitives, s32 first, s32 count, const StateBlock& stateBlock);
        
    protected:
        
                                    //! Constructs a RenderCommandBuffer instance.
                                    RenderCommandBuffer(RenderFrame& frame);
        
        //! Emits a draw call command.
        void                        emitDrawCall( OpCode::Type type, u32 sorting, PrimitiveType primitives, s32 first, s32 count, const StateBlock** states, s32 stateCount, const StateBlock* overrideStateBlock);
        
        //! Compiles a state block stack to an array of rendering state.
        s32                         compileStateStack(const StateBlock* const * stateBlocks, s32 count, State* states, s32 maxStates, OpCode::CompiledStateBlock* compiledStateBlock);
        
    private:
        
        RenderFrame&                m_frame;                    //!< A parent render frame that issued this command buffer.
        StateStack&                 m_stateStack;               //!< An active state stack.
        u8                          m_transientResourceIndex;   //!< A transient resource index relative to a current stack offset.
    };

} // namespace Renderer

DC_END_DREEMCHEST

#endif  /*  !__DC_Renderer_RenderCommandBuffer_H__   */

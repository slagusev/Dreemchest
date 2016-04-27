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

#ifndef __DC_Scene_Commands_H__
#define __DC_Scene_Commands_H__

#include "RenderState.h"

DC_BEGIN_DREEMCHEST

namespace Scene {

    //! A command buffer that is generated by render pass and executed by RVM.
    class RenderCommandBuffer {
    friend class RenderFrame;
    public:

        //! A single render operation.
        struct OpCode {
            //! An op-code type.
            enum Type {
                  DrawIndexed           //!< Draws a list of primitives using an index buffer.
                , DrawPrimitives        //!< Draws a list of primitives from an active vertex buffer.
                , Clear                 //!< Clears a render target.
                , Execute               //!< Executes a command buffer.
                , RenderTarget          //!< Begins rendering to a viewport.
                , UploadConstantBuffer  //!< Uploads data to a constant buffer.
            };

            Type                                type;                       //!< An op code type.
            u64                                 sorting;                    //!< A sorting key.
            union {
                struct {
                    Renderer::PrimitiveType     primitives;                 //!< A primitive type to be rendered.
                    s32                         first;                      //!< First index or primitive.
                    s32                         count;                      //!< A total number of indices or primitives to use.
                    const RenderStateBlock*     states[MaxStateStackDepth]; //!< States from this stack are applied before a rendering command.
                } drawCall;

                struct {
                    u8                          mask;                       //!< A clear mask.
                    f32                         color[4];                   //!< A color buffer clear value.
                    f32                         depth;                      //!< A depth buffer clear value.
                    s32                         stencil;                    //!< A stencil buffer clear value.
                } clear;

                struct {
                    RenderResource              id;                         //!< A render target resource to be activated.
                    u32                         viewport[4];                //!< A viewport value to be set.
                    const RenderCommandBuffer*  commands;                   //!< A command buffer to be executed after setting a viewport.
                } renderTarget;

                struct {
                    const RenderCommandBuffer*  commands;                   //!< A command buffer to be executed.
                } execute;

                struct {
                    u32                         id;                         //!< A target buffer handle.
                    const void*                 data;                       //!< A source data point.
                    s32                         size;                       //!< A total number of bytes to upload.
                } upload;
            };
        };

        //! Returns a total number of recorded commands.
        s32                         size( void ) const;

        //! Returns a command at specified index.
        const OpCode&               opCodeAt( s32 index ) const;

        //! Emits a render target clear command.
        void                        clear( const Rgba& clearColor, u8 clearMask );

        //! Emits a command buffer execution command.
        void                        execute( const RenderCommandBuffer& commands );

        //! Emits a rendering to a viewport of a specified render target command.
        RenderCommandBuffer&        renderToTarget( const Rect& viewport );

        //! Emits a constant buffer upload command.
        void                        uploadConstantBuffer( u32 id, const void* data, s32 size );

        //! Emits a draw indexed command.
        void                        drawIndexed( u32 sorting, Renderer::PrimitiveType primitives, const RenderStateBlock* states[MaxStateStackDepth], s32 first, s32 count );

        //! Emits a draw primitives command.
        void                        drawPrimitives( u32 sorting, Renderer::PrimitiveType primitives, const RenderStateBlock* states[MaxStateStackDepth], s32 first, s32 count );

    private:

                                    //! Constructs a RenderCommandBuffer instance.
                                    RenderCommandBuffer( RenderFrame& frame );

    private:

        RenderFrame&                m_frame;    //!< A parent frame instance.
        Array<OpCode>               m_commands; //!< An array of recorded commands.
    };

    // ** RenderCommandBuffer::size
    NIMBLE_INLINE s32 RenderCommandBuffer::size( void ) const
    {
        return static_cast<s32>( m_commands.size() );
    }

    // ** RenderCommandBuffer::opCodeAt
    NIMBLE_INLINE const RenderCommandBuffer::OpCode& RenderCommandBuffer::opCodeAt( s32 index ) const
    {
        DC_ABORT_IF( index < 0 || index >= size(), "index is out of range" );
        return m_commands[index];
    }

} // namespace Scene

DC_END_DREEMCHEST

#endif    /*    !__DC_Scene_Commands_H__    */
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

#ifndef __DC_Renderer_OpCode_H__
#define __DC_Renderer_OpCode_H__

DC_BEGIN_DREEMCHEST

namespace Renderer
{
    
    //! A single render operation.
    struct OpCode
    {
        //! An op-code type.
        enum Type
        {
              DrawIndexed               //!< Draws a list of primitives using an index buffer.
            , DrawPrimitives            //!< Draws a list of primitives from an active vertex buffer.
            , Clear                     //!< Clears a render target.
            , Execute                   //!< Executes a command buffer.
            , RenderToTexture           //!< Begins rendering to a persistent texture.
            , RenderToTransientTexture  //!< Begins rendering to a transient texture.
            , UploadConstantBuffer      //!< Uploads data to a constant buffer.
            , UploadVertexBuffer        //!< Uploads data to a vertex buffer.
            , AcquireTexture            //!< Acquires a transient texture instance.
            , ReleaseTexture            //!< Releases a transient texture instance.
            , CreateInputLayout         //!< Creates a new input layout from a vertex declaration.
            , CreateVertexBuffer        //!< Creates a new vertex buffer object.
            , CreateIndexBuffer         //!< Creates a new index buffer object.
            , CreateConstantBuffer      //!< Creates a new constant buffer object.
            , CreateTexture             //!< Creates a new texture.
            , DeleteTexture             //!< Destroys an allocated texture.
            , DeleteConstantBuffer      //!< Destroys an allocated constant buffer.
            , DeleteProgram             //!< Destroys a program and all it's permutations.
        };
        
        //! A data buffer used by a command.
        struct Buffer
        {
            const u8*   data;           //!< A source data pointer.
            s32         size;           //!< A buffer size.
        };
        
        Type                                type;                       //!< An op code type.
        u64                                 sorting;                    //!< A sorting key.
        union
        {
            ResourceId                      id;                         //!< A passed resource id.
            
            struct
            {
                PrimitiveType               primitives;                 //!< A primitive type to be rendered.
                s32                         first;                      //!< First index or primitive.
                s32                         count;                      //!< A total number of indices or primitives to use.
                const StateBlock**          states;                     //!< States from this stack are applied before a rendering command.
            } drawCall;
            
            struct
            {
                u8                          mask;                       //!< A clear mask.
                f32                         color[4];                   //!< A color buffer clear value.
                f32                         depth;                      //!< A depth buffer clear value.
                s32                         stencil;                    //!< A stencil buffer clear value.
            } clear;
            
            struct
            {
                ResourceId                  id;                         //!< A render target resource to be activated.
                NormalizedViewport          viewport;                   //!< A viewport value to be set.
                u8                          side;                       //!< A side of a cube map to render to.
                const CommandBuffer*        commands;                   //!< A command buffer to be executed after setting a viewport.
            } renderToTextures;
            
            struct
            {
                TransientResourceId         id;
                u16                         width;
                u16                         height;
                PixelFormat                 format;
                u8                          type;
            } transientTexture;
            
            struct
            {
                const CommandBuffer*        commands;                   //!< A command buffer to be executed.
            } execute;
            
            struct
            {
                ResourceId                  id;                         //!< A target buffer handle.
                Buffer                      buffer;                     //!< An attached data buffer.
            } upload;
            
            struct
            {
                ResourceId                  id;                         //!< Handle to an input layout being constructed.
                u8                          format;                     //!< Vertex format used by an input layout constructor.
            } createInputLayout;
            
            struct
            {
                ResourceId                  id;                         //!< Handle to a buffer object being constructed.
                Buffer                      buffer;                     //!< An attached data buffer.
                ResourceId                  layout;                     //!< Used by a constant buffer constructor.
            } createBuffer;
            
            struct
            {
                ResourceId                  id;                         //!< Handle to a texture being constructed.
                Buffer                      buffer;                     //!< An attached data buffer.
                u16                         width;                      //!< A texture width.
                u16                         height;                     //!< A texture height.
                u8                          mipLevels;                  //!< A total number of mip levels stored in a data buffer.
                PixelFormat                 format;                     //!< A texture format.
                u8                          type;                       //!< A texture type to be created.
                u8                          filter;                     //!< A requested texture filtering.
            } createTexture;
        };
    };
    
} // namespace Renderer

DC_END_DREEMCHEST

#endif    /*    !__DC_Renderer_OpCode_H__    */
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

#ifndef __DC_Font_FreetypeFont_H__
#define __DC_Font_FreetypeFont_H__

#include "../Font.h"

#include <ft2build.h>
#include <freetype/freetype.h>

DC_BEGIN_DREEMCHEST

namespace Font {

    class FreetypeFontProvider;

	// ** struct sFreetypeBitmap
    struct sFreetypeBitmap {
		int					m_key;
        int                 m_width, m_height;
        AutoPtr<Image>      m_image;
        int					m_advance, m_offset;

							sFreetypeBitmap( void );
							~sFreetypeBitmap( void );
    };

    // ** class FreetypeFont
    class FreetypeFont : public IFont {
    public:

                                FreetypeFont( FreetypeFontProvider* provider, FT_Face face );
        virtual                 ~FreetypeFont( void );

        // ** IFont
        virtual int             GetAscender( int size ) const;
        virtual int             GetDescender( int size ) const;
        virtual int             GetLineGap( int size ) const;
        virtual int             CalculateLineWidth( const char *text, int length, int size ) const;

		// ** FreetypeFont
		bool                    RenderBitmap( sFreetypeBitmap& bitmap, u16 code, int size );

    private:

        void                    SelectSize( int value ) const;
        void                    SetScaleFactor( float value ) const;

    private:

        FreetypeFontProvider*   m_provider;
        FT_Face                 m_face;
        mutable int             m_size;
    };
    
} // namespace Font

DC_END_DREEMCHEST

#endif	/*	!__DC_Font_FreetypeFont_H__	*/
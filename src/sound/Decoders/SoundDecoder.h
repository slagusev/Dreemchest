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

#ifndef __DC_SoundDecoder_H__
#define __DC_SoundDecoder_H__

#include    "../Sound.h"

namespace dreemchest {

namespace sound {

    // ** class SoundDecoder
    class SoundDecoder {
    public:

                                    SoundDecoder( void );
        virtual                     ~SoundDecoder( void );

        virtual bool                open( ISoundStream* stream );
        virtual void                close( void );
        virtual long                read( u8 *buffer, int size );
        virtual void                seek( int pos );
        virtual u32                 rate( void ) const;
        virtual SoundSampleFormat   format( void ) const;
        virtual int                 size( void ) const;

    protected:

        ISoundStream*               m_stream;
        u32                         m_rate;
        SoundSampleFormat           m_format;
    };

} // namespace sound

} // namespace dreemchest

#endif    /*    !__DC_SoundDecoder_H__    */
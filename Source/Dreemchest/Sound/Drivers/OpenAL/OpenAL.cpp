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

#include "OpenAL.h"

#include "OpenALSource.h"
#include "OpenALBuffer.h"

#include "../../Decoders/SoundDecoder.h"

#define MAX_PCM_SIZE    10024

DC_BEGIN_DREEMCHEST

namespace Sound {

// ** OpenAL::OpenAL
OpenAL::OpenAL( void )
{
    f32 lp[] = { 0.0f, 0.0f,  0.0f };
    f32 lv[] = { 0.0f, 0.0f,  0.0f };
    f32 lo[] = { 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f };

    m_device = alcOpenDevice( NULL );
    LogVerbose( "openal", "device created %x", m_device );

    m_context = alcCreateContext( m_device, NULL );
    LogVerbose( "openal", "context created %x", m_context );

    alcMakeContextCurrent( m_context );

    alListenerfv( AL_POSITION,    lp );
    alListenerfv( AL_VELOCITY,    lv );
    alListenerfv( AL_ORIENTATION, lo );

    LogVerbose( "openal", "version=%s, renderer=%s, vendor=%s", alGetString( AL_VERSION ), alGetString( AL_RENDERER ), alGetString( AL_VENDOR ) );
//    LogVerbose( "AL_EXTENSIONS: %s\n", alGetString( AL_EXTENSIONS ) );
}

OpenAL::~OpenAL( void )
{
    alcMakeContextCurrent( NULL );
    alcDestroyContext( m_context );
    alcCloseDevice( m_device );
}

// ** OpenAL::maxSources
ALuint OpenAL::maxSources( void )
{
#ifdef DC_PLATFORM_IOS
    return 32;
#endif

    return 64;
}

// ** OpenAL::soundSampleFormat
ALuint OpenAL::soundSampleFormat( SoundSampleFormat format )
{
    switch( format ) {
    case SoundSampleMono8:      return AL_FORMAT_MONO8;
    case SoundSampleMono16:     return AL_FORMAT_MONO16;
    case SoundSampleStereo8:    return AL_FORMAT_STEREO8;
    case SoundSampleStereo16:   return AL_FORMAT_STEREO16;
    }

    return 0;
}

// ** OpenAL::flushErrors
void OpenAL::flushErrors( void )
{
    while( alGetError() ) {
        // ** Do nothing
    }
}

// ** OpenAL::dumpErrors
void OpenAL::dumpErrors( const char *label )
{
    ALenum error = AL_NO_ERROR;

    do {
        error = alGetError();

        switch( error ) {
        case AL_INVALID_NAME:       LogError( "openal", "%s, invalid name\n", label );         break;
        case AL_INVALID_ENUM:       LogError( "openal", "%s, invalid enum\n", label );         break;
        case AL_INVALID_VALUE:      LogError( "openal", "%s, invalid value\n", label );        break;
        case AL_INVALID_OPERATION:  LogError( "openal", "%s, invalid operation\n", label );    break;
        case AL_OUT_OF_MEMORY:      LogError( "openal", "%s, out of memory\n", label );        break;
        }
    } while( error != AL_NO_ERROR );
}

// ** OpenAL::createSource
SoundSourcePtr OpenAL::createSource( void )
{
    return DC_NEW OpenALSource;
}

// ** OpenAL::createBuffer
SoundBufferPtr OpenAL::createBuffer( SoundDecoderPtr decoder, u32 chunks )
{
    DC_ABORT_IF( !decoder.valid(), "invalid decoder" );
    return DC_NEW OpenALBuffer( decoder, chunks, chunks == 1 ? decoder->size() : 16536 );
}

// ** OpenAL::setPosition
void OpenAL::setPosition( const Vec3& value )
{
    ALfloat v[3] = { value.x, value.y, value.z };
    alListenerfv( AL_POSITION, v );
}

} // namespace Sound
    
DC_END_DREEMCHEST
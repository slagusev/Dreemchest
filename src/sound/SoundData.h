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

#ifndef __DC_SoundData_H__
#define __DC_SoundData_H__

#include    "Sound.h"

namespace dreemchest {

namespace sound {

    // ** struct SoundDataInfo
    dcBeginSerializable( SoundDataInfo )
        dcField( String         identifier )
        dcField( String         uri )
        dcField( String         group )
        dcField( u32            type )
        dcField( u8             loading )
        dcField( f32            fadeTime )
        dcField( f32            volume )
        dcField( Vec2           volumeModifier )
        dcField( f32            pitch )
        dcField( Vec2           pitchModifier )
        dcField( bool           isLooped )
        dcField( u32            priority )
    dcEndSerializable

    // ** SoundData
    class SoundData : public RefCounted {
    friend class SoundFx;
    public:

        // ** enum eLoadingFlags
        //! Supported sound loading flags
        enum LoadingFlags {
            Stream,            //!< The sound is streamed from an asset file.
            Decode,            //!< Decode the sound data and store the result in RAM.
            LoadToRam,         //!< The sound asset file is copied to the RAM without decoding.
        };

    public:

                            //! Sound data constructor.
                            /*!
                                \param identifier Sound identifier.
                                \param uri Sound asset unique identifier.
                                \param group The sound group this sound belongs to. Pass NULL for orphan sounds.
                             */
                            SoundData( SoundFx* sfx, const char* identifier, const char* uri, const SoundGroup* group );
                            ~SoundData( void );

        //! Returns a sound identifier.
        const char*         identifier( void ) const;
        //! Sets a sound identifier.
        void                setIdentifier( const char *value );
        //! Returns a sound URI identifier.
        const char*         uri( void ) const;
        //! Sets a sound URI identifier.
        void                setUri( const char* value );
        //! Returns a pointer to a sound group (NULL for orphan sounds).
        const SoundGroup*   group( void ) const;
        //! Sets a pointer to a sound group.
        void                setGroup( const SoundGroup* value );
        //! Returns a sound type.
        u32                 type( void ) const;
        //! Sets a sound type.
        void                setType( u32 value );
        //! Returns a sound loading flag.
        LoadingFlags        loading( void ) const;
        //! Sets a sound loading flag.
        void                setLoading( LoadingFlags value );
        //! Returns a sound fade in/fade out time.
        float               fadeTime( void ) const;
        //! Sets a sound fade in/fade out time.
        void                setFadeTime( f32 value );
        //! Returns base sound playback volume.
        f32                 volume( void ) const;
        //! Sets a base sound playback volume.
        void                setVolume( f32 value );
        //! Returns base sound playback pitch.
        f32                 pitch( void ) const;
        //! Sets a base sound playback pitch.
        void                setPitch( f32 value );
        //! Returns true id this sound is looped, otherwise false.
        bool                isLooped( void ) const;
        //! Sets a sound looping flag.
        void                setLooped( bool value );
        //! Returns a sound priority.
        u32                 priority( void ) const;
        //! Sets a sound priority.
        void                setPriority( u32 value );
        //! Returns a serialized sound data.
        SoundDataInfo       data( void ) const;
        //! Loads a serialized sound data.
        void                setData( const SoundDataInfo& value );
        //! Returns a sound buffer with decoded PCM data.
        SoundBuffer*        pcm( void ) const;
        //! Sets a sound buffer with decoded PCM data.
        void                setPcm( SoundBuffer* value );
        //! Calculates and returns a sound volume with a random modifier applied.
        f32                 volumeForSound( void ) const;
        //! Calculates and returns a sound pitch with a random modifier applied.
        f32                 pitchForSound( void ) const;

    private:

        //! Parent sound fx.
        SoundFx*            m_soundFx;
        //! Sound identifier.
        String              m_identifier;
        //! Asset associated with this sound.
        String              m_uri;
        //! The group this sound belongs to.
        const SoundGroup*   m_group;
        //! Sound type.
        u32                 m_type;
        //! The flag indicating the loading behaviour for this sound.
        LoadingFlags        m_loading;
        //! Sound fade in/ fade out time in milliseconds.
        f32                 m_fadeTime;
        //! Sound volume.
        f32                 m_volume;
        //! Sound volume random multiplicative modifier.
        Vec2                m_volumeModifier;
        //! Sound pitch.
        f32                 m_pitch;
        //! Sound pitch random multiplicative modifier.
        Vec2                m_pitchModifier;
        //! The flag indicating whether this sound is looped.
        bool                m_isLooped;
        //! Sound playback priority.
        u32                 m_priority;
        //! Decoded PCM data.
        dcSoundBufferStrong m_pcm;
    };
    
} // namespace sound
    
} // namespace dreemchest

#endif        /*    __DC_SoundData_H__    */
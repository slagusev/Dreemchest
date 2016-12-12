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

#include "RenderState.h"

DC_BEGIN_DREEMCHEST

namespace Renderer
{

// ---------------------------------------------------------------------------- State ---------------------------------------------------------------------------- //

// ** State::State
State::State( void )
    : type( TotalStates )
{
}

// ** State::State
State::State( Type type, s32 id )
    : resourceId( id )
    , type( type )
{
}

// ** State::State
State::State( TriangleFace face )
    : type( CullFace )
{
    cullFace = face;
}

// ** State::State
State::State( Compare function, bool write )
    : type( DepthState )
{
    compareFunction = function;
    data.depthWrite = write;
}

// ** State::State
State::State( Compare function, f32 reference )
    : type( AlphaTest )
{
    compareFunction     = function;
    data.alphaReference = static_cast<u8>( reference * 255 );
}

// ** State::State
State::State( s32 id, ConstantBufferType type )
    : type( ConstantBuffer )
{
    resourceId = id;
    data.index = type;
}

// ** State::State
State::State( BlendFactor src, BlendFactor dst )
    : type( Blending )
{
    data.blend = (src << 4) | dst;
}

#if DEV_DEPRECATED_HAL
// ** State::State
State::State( s32 id, TextureSampler sampler, RenderTarget::Attachment attachment )
    : type( Texture )
{
    resourceId = id;
    data.index = sampler | (attachment << 4);
}
#else
// ** State::State
State::State( s32 id, TextureSampler sampler, u8 attachment )
    : type( Texture )
{
    NIMBLE_NOT_IMPLEMENTED
    resourceId = id;
    data.index = sampler | (attachment << 4);
}
#endif  /*  #if DEV_DEPRECATED_HAL  */

// ** State::State
State::State( f32 factor, f32 units )
    : type( PolygonOffset )
{
    polygonOffset.factor = factor * 128.0f;
    polygonOffset.units  = units * 128.0f;
}

// -------------------------------------------------------------------------- StateBlock -------------------------------------------------------------------------- //

// ** StateBlock::StateBlock
StateBlock::StateBlock( void )
    : m_mask( 0 )
    , m_features( 0 )
    , m_featureMask( ~0 )
    , m_count( 0 )
{
}

// ** StateBlock::bindVertexBuffer
void StateBlock::bindVertexBuffer( s32 id )
{
    pushState( State( State::VertexBuffer, id ), State::VertexBuffer );
}

// ** StateBlock::bindIndexBuffer
void StateBlock::bindIndexBuffer( s32 id )
{
    pushState( State( State::IndexBuffer, id ), State::IndexBuffer );
}

// ** StateBlock::bindInputLayout
void StateBlock::bindInputLayout( s32 id )
{
    pushState( State( State::InputLayout, id ), State::InputLayout );
}
    
// ** StateBlock::bindFeatureLayout
void StateBlock::bindFeatureLayout( s32 id )
{
    pushState( State( State::FeatureLayout, id ), State::FeatureLayout );
}

// ** StateBlock::bindConstantBuffer
void StateBlock::bindConstantBuffer( s32 id, State::ConstantBufferType type )
{
    pushState( State( id, type ), State::ConstantBuffer + type );
}

// ** StateBlock::bindProgram
void StateBlock::bindProgram( s32 id )
{
    pushState( State( State::Shader, id ), State::Shader );
}

// ** StateBlock::bindTexture
void StateBlock::bindTexture( s32 id, State::TextureSampler sampler )
{
    pushState( State( id, sampler ), State::Texture + sampler );
}

#if DEV_DEPRECATED_HAL
// ** StateBlock::bindRenderedTexture
void StateBlock::bindRenderedTexture( u8 renderTarget, State::TextureSampler sampler, RenderTarget::Attachment attachment )
{
    pushState( State( -renderTarget, sampler, attachment ), State::Texture + sampler );
}
#else
// ** StateBlock::bindRenderedTexture
void StateBlock::bindRenderedTexture( u8 renderTarget, State::TextureSampler sampler, u8 attachment )
{
    NIMBLE_NOT_IMPLEMENTED
    pushState( State( -renderTarget, sampler, attachment ), State::Texture + sampler );
}
#endif  /*  #if DEV_DEPRECATED_HAL  */

// ** StateBlock::setBlend
void StateBlock::setBlend( BlendFactor src, BlendFactor dst )
{
    pushState( State( src, dst ), State::Blending );
}

// ** StateBlock::setDepthState
void StateBlock::setDepthState( Compare function, bool write )
{
    pushState( State( function, write ), State::DepthState );
}

// ** StateBlock::enableFeatures
void StateBlock::enableFeatures( u64 bits )
{
    m_features = m_features | bits;
}

// ** StateBlock::disableFeatures
void StateBlock::disableFeatures( u64 bits )
{
    m_featureMask = m_featureMask & ~bits;
}

// ** StateBlock::setPolygonOffset
void StateBlock::setPolygonOffset( f32 factor, f32 units )
{
    pushState( State( factor, units ), State::PolygonOffset );
}

// ** StateBlock::disablePolygonOffset
void StateBlock::disablePolygonOffset( void )
{
    setPolygonOffset( 0.0f, 0.0f );
}

// ** StateBlock::setAlphaTest
void StateBlock::setAlphaTest( Compare function, f32 reference )
{
    pushState( State( function, reference ), State::AlphaTest );
}

// ** StateBlock::setCullFace
void StateBlock::setCullFace( TriangleFace face )
{
    pushState( State( face ), State::CullFace );
}

// ** StateBlock::disableBlending
void StateBlock::disableAlphaTest( void )
{
    setAlphaTest( Renderer::CompareDisabled, 0.0f );
}

// ** StateBlock::disableBlending
void StateBlock::disableBlending( void )
{
    setBlend( BlendDisabled, BlendDisabled );
}

// ** StateBlock::pushState
void StateBlock::pushState( const State& state, u32 stateBit )
{
    NIMBLE_BREAK_IF( m_mask & BIT( stateBit ), "a state setting could not be overriden" );
    NIMBLE_ABORT_IF( m_count + 1 >= MaxStates, "state block overflow" );

    // Push a state to a state block
    m_states[m_count] = state;
    m_stateBits[m_count] = BIT( stateBit );
    m_count++;

    // Update a state block bitmask
    m_mask = m_mask | BIT( stateBit );
}

// ---------------------------------------------------------------------------- StateScope ------------------------------------------------------------------------------ //

// ** StateScope::StateScope
StateScope::StateScope( StateStack& stack, StateBlock* stateBlock )
    : m_stack( stack )
    , m_stateBlock( stateBlock )
{
}

// ** StateScope::StateScope
StateScope::StateScope( const StateScope& other )
    : m_stack( other.m_stack )
    , m_stateBlock( other.m_stateBlock )
{
    NIMBLE_NOT_IMPLEMENTED
//    other.m_stateBlock = NULL;
}

// ** StateScope::~StateScope
StateScope::~StateScope( void )
{
    if( m_stateBlock ) {
        m_stack.pop();
    }
}

// ** StateScope::operator =
const StateScope& StateScope::operator = ( StateScope& other )
{
    m_stack      = other.m_stack;
    m_stateBlock = other.m_stateBlock;
    other.m_stateBlock = NULL;
    return *this;
}

// -------------------------------------------------------------------------- StateStack -------------------------------------------------------------------------- //

// ** StateStack::StateStack
StateStack::StateStack( s32 maxStateBlocks, s32 maxStackSize )
    : m_allocator( maxStateBlocks * sizeof( StateBlock ) + sizeof( StateBlock* ) * maxStackSize )
    , m_stack( NULL )
    , m_size( 0 )
{
    m_stack = reinterpret_cast<const StateBlock**>( m_allocator.allocate( sizeof( StateBlock* ) * maxStackSize ) );
}

// ** StateStack::newScope
StateScope StateStack::newScope( void )
{
    NIMBLE_ABORT_IF( (size() + 1) >= MaxStateStackDepth, "stack overflow" );

    void* allocated = m_allocator.allocate( sizeof( StateBlock ) );
    NIMBLE_ABORT_IF( allocated == NULL, "to much render state blocks allocated" );

    StateBlock* block = new( allocated ) StateBlock;
    return push( block );
}

// ** StateStack::push
StateScope StateStack::push( const StateBlock* block )
{
    if( block == NULL ) {
        return StateScope( *this, NULL );
    }

    for( s32 i = m_size; i > 0; i-- ) {
        m_stack[i] = m_stack[i - 1];
    }
    m_stack[0] = block;
    m_size++;

    return StateScope( *this, const_cast<StateBlock*>( block ) );
}

// ** StateStack::pop
void StateStack::pop( void )
{
    NIMBLE_ABORT_IF( size() == 0, "stack underflow" );

    for( s32 i = 0; i < m_size - 1; i++ ) {
        m_stack[i] = m_stack[i + 1];
    }

    m_stack[m_size--] = NULL;
}

// ** StateStack::size
s32 StateStack::size( void ) const
{
    return m_size;
}

// ** StateStack::states
const StateBlock** StateStack::states( void ) const
{
    return m_stack;
}
    
// ** StateStack::clear
void StateStack::clear( void )
{
    m_size = 0;
}

} // namespace Renderer

DC_END_DREEMCHEST
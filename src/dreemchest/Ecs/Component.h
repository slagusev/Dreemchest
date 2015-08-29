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

#ifndef __DC_Ecs_Component_H__
#define __DC_Ecs_Component_H__

#include "Ecs.h"

#define OverrideComponent( Type, Super )        \
    IoOverrideSerializableSuper( Type, Super )  \
    static const Bitset&	bit( void ) { static Bitset result = Bitset::withSingleBit( TypeIndex<Type>::idx() ); return result; }

DC_BEGIN_DREEMCHEST

namespace ecs {

	//! A base class for internal system data attached to a components.
	struct InternalBase : public RefCounted {
	};

	//! A template class used for declaring system-specific internal data.
	template<typename T>
	struct Internal : public InternalBase {
		typedef StrongPtr<T> Ptr;	//!< The internal data pointer.
	};

	//! A base class for all components.
	/*!
	A component is all the data for one aspect of the entity. Component is just a plain
	data and doesn't contain any processing logic.
	*/
	class Component : public io::Serializable {
	public:

									OverrideComponent( Component, io::Serializable )

		//! Sets the internal data.
		template<typename T>
		void						setInternal( InternalBase* value );

		//! Returns the internal data.
		template<typename T>
		typename Internal<T>::Ptr	internal( void ) const;

	private:

		//! Container type to store internal system state inside a component.
		typedef Map< TypeIdx, StrongPtr<InternalBase> > InternalDataHolder;

		InternalDataHolder			m_internal;	//!< The internal data.
	};

	// ** Component::setInternal
	template<typename T>
	inline void Component::setInternal( InternalBase* value )
	{
		m_internal[TypeIndex<T>::idx()] = value;
	}

	// ** Component::internal
	template<typename T>
	inline typename Internal<T>::Ptr Component::internal( void ) const
	{
		InternalDataHolder::const_iterator i = m_internal.find( TypeIndex<T>::idx() );
		return i != m_internal.end() ? i->second : Internal<T>::Ptr();
	}

} // namespace ecs

DC_END_DREEMCHEST

#endif	/*	!__DC_Ecs_Component_H__	*/
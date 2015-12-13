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

#ifndef __DC_Scene_Component_UserInput_H__
#define __DC_Scene_Component_UserInput_H__

#include "../Bindings.h"
#include "../Scene.h"

DC_BEGIN_DREEMCHEST

namespace Scene {

	//! Bitset flags used in transformations.
	enum CoordinateSystemFlags {
		  CSLocalX	= BIT( 0 )							//!< Use local X axis for transformations.
		, CSLocalY	= BIT( 1 )							//!< Use local Y axis for transformations.
		, CSLocalZ	= BIT( 2 )							//!< Use local Z axis for transformations.
		, CSLocal	= CSLocalX | CSLocalY | CSLocalZ	//!< Use the local coordinate system for transformations.
		, CSWorld	= 0									//!< Use the world coordinate system for transformations.
	};

	//! Identifier component.
	class Identifier : public Ecs::Component<Identifier> {
	public:

								//! Constructs the Identifier instance.
								Identifier( const String& name = "" )
									: m_name( name ) {}

		//! Returns the identifier.
		const String&			name( void ) const;

		//! Sets the identifier.
		void					setName( const String& value );

	private:

		String					m_name;	//!< Scene object name.
	};

	//! Moves the scene object transform along the coordinate axes.
	class MoveAlongAxes : public Ecs::Component<MoveAlongAxes> {
	public:

								//! Constructs MoveAlongAxes instance.
								MoveAlongAxes( f32 speed = 1.0f, u8 coordinateSystem = CSWorld, const Vec3BindingPtr& delta = Vec3BindingPtr() )
									: m_coordinateSystem( coordinateSystem ), m_speed( speed ), m_delta( delta ) {}

		//! Returns the movement speed.
		f32						speed( void ) const;

		//! Returns the coordinate system used for transformations.
		u8						coordinateSystem( void ) const;

		//! Returns the movement delta values.
		Vec3					delta( void ) const;

	private:

		u8						m_coordinateSystem;	//!< Coordinate system flags.
		f32						m_speed;			//!< Movement speed.
		Vec3BindingPtr			m_delta;			//!< Movement deltas.
	};

	//! Rotates the scene object transform around axes.
	class RotateAroundAxes : public Ecs::Component<RotateAroundAxes> {
	public:

								//! Constructs RotateAroundAxes instance.
								RotateAroundAxes( f32 speed = 1.0f, u8 coordinateSystem = CSWorld, const Vec3BindingPtr& delta = Vec3BindingPtr() )
									: m_coordinateSystem( coordinateSystem ), m_speed( speed ), m_delta( delta ) {}

		//! Returns the rotation speed.
		f32						speed( void ) const;

		//! Sets the rotation speed.
		void					setSpeed( f32 value );

		//! Returns the coordinate system used for transformations.
		u8						coordinateSystem( void ) const;

		//! Returns rotation delta values.
		Vec3					delta( void ) const;

	private:

		u8						m_coordinateSystem;	//!< Coordinate system flags.
		f32						m_speed;			//!< Rotation speed.
		Vec3BindingPtr			m_delta;			//!< Rotation delta values.
	};

} // namespace Scene

DC_END_DREEMCHEST

#endif    /*    !__DC_Scene_Component_UserInput_H__    */
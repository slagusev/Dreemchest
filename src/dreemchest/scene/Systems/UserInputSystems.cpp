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

#include "UserInputSystems.h"

DC_BEGIN_DREEMCHEST

namespace Scene {

// ** MoveInDirectionSystem::MoveInDirectionSystem
MoveInDirectionSystem::MoveInDirectionSystem( void ) : GenericEntitySystem( "MoveInDirection" )
{

}

// ** MoveInDirectionSystem::process
void MoveInDirectionSystem::process( u32 currentTime, f32 dt, Ecs::Entity& sceneObject, MoveInDirection& move, Transform& transform )
{
	Vec3 direction = move.direction();

	DC_BREAK_IF( direction.length() > 1.0f )

	// Scale direction by speed
	direction = direction * move.speed();

	// Update the transform
	switch( move.axes() ) {
	case MoveInDirection::XY:	transform.setPosition( transform.position() + Vec3( direction.x, direction.y, 0.0f )  * dt );
								break;

	case MoveInDirection::XZ:	transform.setPosition( transform.position() + Vec3( direction.x, 0.0f, -direction.y ) * dt );
								break;
	}
}

} // namespace Scene

DC_END_DREEMCHEST
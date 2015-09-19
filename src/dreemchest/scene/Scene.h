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

#ifndef __DC_Scene_H__
#define __DC_Scene_H__

#include "../Dreemchest.h"

#define HAVE_JSON

#ifndef FOO_INCLUDED
    #include <Foo/Foo.h>
#endif

#include <platform/Platform.h>
#include <platform/Input.h>
#include <platform/Window.h>

#include <renderer/Renderer.h>
#include <renderer/Hal.h>
#include <renderer/Renderer2D.h>

#include <Ecs/Entity.h>
#include <Ecs/Component.h>
#include <Ecs/EntitySystem.h>

#include <Io/DiskFileSystem.h>

#ifdef HAVE_JSON
	#include <json/json.h>
#endif

DC_BEGIN_DREEMCHEST

namespace Scene {

	DECLARE_LOG( log )

	class Scene;
	class View;
	class AssetBundle;
	class Asset;
		class Image;
		class Mesh;
		class Material;

	class Transform;
	class StaticMesh;
	class Sprite;
	class Camera;
	class Rotor;

	class Rvm;

	//! Available rendering modes.
	enum RenderingMode {
		  RenderOpaque		//!< Renders opaque.
		, RenderCutout		//!< Renders object with an alpha test.
		, RenderTranslucent	//!< Renders object with alpha blending.
		, RenderAdditive	//!< Renders objects with additive blending.
		, TotalRenderModes	//!< Total number of render modes.
	};

	//! Rendering mode mask.
	enum RenderingModeBits {
		  RenderOpaqueBit		= BIT( RenderOpaque )		//!< Opaque rendering bit.
		, RenderCutoutBit		= BIT( RenderCutout )		//!< Cutout rendering bit.
		, RenderTranslucentBit	= BIT( RenderTranslucent )	//!< Translucent rendering bit.
		, RenderAdditiveBit		= BIT( RenderAdditive )		//!< Additive rendering bit.
		, AllRenderModesBit		= RenderOpaqueBit | RenderCutoutBit | RenderTranslucentBit | RenderAdditiveBit
	};

	// Alias the Ecs::Entity type
	typedef StrongPtr<Ecs::Entity>	SceneObjectPtr;
	typedef WeakPtr<Ecs::Entity>	SceneObjectWPtr;

	// Alias the Ecs::Component type
	typedef Ecs::Component			Component;

	dcDeclarePtrs( Direction )
	dcDeclarePtrs( Scene )
	dcDeclarePtrs( View )
	dcDeclarePtrs( Transform )
	dcDeclarePtrs( Camera )
	dcDeclarePtrs( StaticMesh )
	dcDeclarePtrs( Light )
	dcDeclarePtrs( RenderPassBase )
	dcDeclarePtrs( ShaderCache )
	dcDeclarePtrs( AssetBundle )
	dcDeclarePtrs( AssetLoader )
	dcDeclarePtrs( Asset )
	dcDeclarePtrs( AssetData )
	dcDeclarePtrs( Image )
	dcDeclarePtrs( Mesh )
	dcDeclarePtrs( Material )

	dcDeclarePtrs( AssetTexture )
	dcDeclarePtrs( AssetMesh )

	dcDeclarePtrs( Box2DPhysics )
	dcDeclarePtrs( RigidBody2D )

	dcDeclarePtrs( RenderingContext )
	dcDeclarePtrs( Rvm )
	dcDeclarePtrs( RenderPassBase )
	dcDeclarePtrs( RenderingSystemBase )
	dcDeclarePtrs( UberShader )
	dcDeclarePtrs( RopEmitterBase )

	//! Container type to store scene objects.
	typedef List<SceneObjectPtr> SceneObjectsList;

	//! The root class for a scene subsystem.
	class Scene : public RefCounted {
	public:

		//! Performs a scene update.
		void							update( f32 dt );

		//! Renders a scene.
		void							render( const RenderingContextPtr& context );

		//! Creates a new scene object instance.
		SceneObjectPtr					createSceneObject( void );

		//! Returns the list of scene object with specified name.
		SceneObjectsList				findAllWithName( const String& name ) const;

		//! Returns the scene systems.
		Ecs::Systems&					systems( void );

		//! Adds a new rendering system to the scene.
		template<typename TRenderingSystem>
		void							addRenderingSystem( void );

		//! Creates a new scene.
		static ScenePtr					create( void );

	private:

										//! Constructs a Scene instance.
										Scene( void );

	private:

		//! Scene entities container.
		Ecs::Entities					m_entities;

		//! Entity update systems.
		Ecs::Systems					m_systems;

		//! Entity rendering systems.
		Array<RenderingSystemBasePtr>	m_renderingSystems;

		//! All cameras that reside in scene.
		Ecs::FamilyPtr					m_cameras;

		//! All named entities that reside in scene stored inside this family.
		Ecs::FamilyPtr					m_named;

		//! Next scene object id.
		Ecs::EntityId					m_nextEntityId;
	};

	// ** Scene::addRenderingSystem
	template<typename TRenderingSystem>
	void Scene::addRenderingSystem( void )
	{
		m_renderingSystems.push_back( DC_NEW TRenderingSystem( m_entities ) );
	}

#ifdef HAVE_JSON

	//! Loads the scene from JSON file.
	class JsonSceneLoader {
	public:

									//! Constructs the JsonSceneLoader instance.
									JsonSceneLoader( void );

		//! Loads the scene from string.
		bool						load( ScenePtr scene, const AssetBundlePtr& assets, const String& json );

	private:

		//! Returns the scene object by it's id.
		Ecs::EntityPtr				requestSceneObject( const String& id );

		//! Returns the component by it's id.
		Ecs::ComponentPtr			requestComponent( const String& id );

		//! Reads the Transform component from JSON object.
		Ecs::ComponentPtr			readTransform( const Json::Value& value );

		//! Reads the Renderer component from JSON object.
		Ecs::ComponentPtr			readRenderer( const Json::Value& value );

		//! Reads the Camera component from JSON object.
		Ecs::ComponentPtr			readCamera( const Json::Value& value );

		//! Reads the Light component from JSON object.
		Ecs::ComponentPtr			readLight( const Json::Value& value );

		//! Reads the Vec3 from a JSON object.
		static Vec3					readVec3( const Json::Value& value );

		//! Reads the Rect from a JSON object.
		static Rect					readRect( const Json::Value& value );

		//! Reads the Rgba from JSON object.
		static Rgba					readRgba( const Json::Value& value );

		//! Reads the Rgba from JSON object.
		static Rgb					readRgb( const Json::Value& value );

		//! Reads the Quat from JSON object.
		static Quat					readQuat( const Json::Value& value );

	private:

		//! Component loader type.
		typedef cClosure<Ecs::ComponentPtr(const Json::Value&)> ComponentLoader;

		//! Container to store all available component loaders.
		typedef Map<String, ComponentLoader> ComponentLoaders;

		//! Container type to store parsed scene objects.
		typedef Map<String, Ecs::EntityPtr> SceneObjects;

		//! Container type to store parsed components.
		typedef Map<String, Ecs::ComponentPtr> Components;

		AssetBundlePtr				m_assets;		//!< Available assets.
		Json::Value					m_json;			//!< Parsed JSON.
		ScenePtr					m_scene;		//!< The scene to be loaded.
		SceneObjects				m_sceneObjects;	//!< Parsed scene objects.
		Components					m_components;	//!< Parsed components.
		ComponentLoaders			m_loaders;		//!< Available component loaders.
	};

#endif	/*	HAVE_JSON	*/

} // namespace Scene

DC_END_DREEMCHEST

#ifndef DC_BUILD_LIBRARY
	#include "Components/Rendering.h"
	#include "Components/Transform.h"
	#include "Components/Physics.h"
	#include "Assets/Mesh.h"
	#include "Assets/Material.h"
	#include "Assets/Image.h"
	#include "Systems/TransformSystems.h"
	#include "Systems/Physics2D.h"
	#include "Systems/Input2DSystems.h"
	#include "Systems/AssetSystem.h"
	#include "Rendering/RenderingContext.h"
	#include "Rendering/RenderingSystem.h"
	#include "Rendering/ShaderCache.h"
	#include "Rendering/Passes/DebugPasses.h"
	#include "Rendering/Passes/BasicPasses.h"
	#include "Rendering/ForwardLighting/LightPass.h"
#endif

#endif    /*    !__DC_Scene_H__    */
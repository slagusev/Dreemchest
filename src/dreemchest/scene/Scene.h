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

#ifdef DC_CPP11_DISABLED
	#error C++11 should be enabled to use scene module.
#endif

#include <Platform/Platform.h>
#include <Platform/Input.h>
#include <Platform/Window.h>

#include <Renderer/Renderer.h>
#include <Renderer/Hal.h>
#include <Renderer/Renderer2D.h>

#include <Ecs/Entity/Entity.h>
#include <Ecs/Entity/Archetype.h>
#include <Ecs/Component/Component.h>
#include <Ecs/System/GenericEntitySystem.h>
#include <Ecs/System/ImmutableEntitySystem.h>
#include <Ecs/System/SystemGroup.h>

#include <Io/DiskFileSystem.h>
#include <Io/JsonLoader.h>

#include <Fx/ParticleSystem.h>
#include <Fx/Particles.h>
#include <Fx/Emitter.h>
#include <Fx/Renderers.h>
#include <Fx/Zones.h>

#include "PlaneClipper.h"

DC_BEGIN_DREEMCHEST

namespace Scene {

	DECLARE_LOG( log )

	class Scene;
	class RenderTarget;
	class AssetBundle;
	class Asset;
		class Image;
		class Mesh;
		class Material;
		class Terrain;

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
	typedef Ecs::EntityId			SceneObjectId;

	dcDeclarePtrs( Scene )
	dcDeclarePtrs( RenderTarget )
	dcDeclarePtrs( TextureTarget )
	dcDeclarePtrs( WindowTarget )
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
	dcDeclarePtrs( Terrain )

	dcDeclarePtrs( AssetTexture )
	dcDeclarePtrs( AssetMesh )

	dcDeclarePtrs( Box2DPhysics )
	dcDeclarePtrs( RigidBody2D )

	dcDeclarePtrs( RenderingContext )
	dcDeclarePtrs( Rvm )
	dcDeclarePtrs( RenderPassBase )
	dcDeclarePtrs( RenderingSystemBase )
	dcDeclarePtrs( Shader )
	dcDeclarePtrs( RopEmitterBase )

	dcDeclarePtrs( Vec3Binding )

	dcDeclarePtrs( SpectatorCamera )

	//! Scene systems mask.
	enum Systems {
		  UpdateSystems = BIT( 0 )
		, RenderSystems = BIT( 1 )
	};

	//! Container type to store a set of scene objects.
	typedef Set<SceneObjectPtr> SceneObjectSet;

	//! The root class for a scene subsystem.
	class Scene : public RefCounted {
	public:

		//! Performs a scene update.
		void							update( u32 currentTime, f32 dt );

		//! Renders a scene.
		void							render( const RenderingContextPtr& context );

		//! Creates a new scene object instance.
		SceneObjectPtr					createSceneObject( void );

		//! Adds new scene object to scene.
		void							addSceneObject( const SceneObjectPtr& sceneObject );

		//! Removes scene object to scene.
		void							removeSceneObject( const SceneObjectPtr& sceneObject );

		//! Creates an archetype instance.
		template<typename TArchetype>
		StrongPtr<TArchetype>			createArchetype( const SceneObjectId& id, const io::Bson& data = io::Bson::kNull );

		//! Creates a new scene object instance.
		SceneObjectPtr					createSceneObject( const SceneObjectId& id );

		//! Returns the scene object with specified id.
		SceneObjectPtr					findSceneObject( const SceneObjectId& id ) const;

		//! Returns the list of scene object with specified name.
		SceneObjectSet					findAllWithName( const String& name ) const;

		//! Returns a list of scene objects that match a specified aspect.
		SceneObjectSet					findByAspect( const Ecs::Aspect& aspect ) const;

		//! Returns cameras that reside in scene.
		const Ecs::IndexPtr&			cameras( void ) const;

		//! Returns a scene system of specified type.
		template<typename TSystem>
		WeakPtr<TSystem>				system( void ) const;

		//! Adds a new system to the scene.
		template<typename TSystem, typename ... Args>
		WeakPtr<TSystem>				addSystem( Args ... args );

		//! Adds a new rendering system to the scene.
		template<typename TRenderingSystem>
		void							addRenderingSystem( void );

		//! Creates an empty scene.
		static ScenePtr					create( void );

		//! Creates scene and loads it from JSON file.
		static ScenePtr					createFromFile( const AssetBundlePtr& assets, const String& fileName );

		//! Creates scene and loads it from JSON string.
		static ScenePtr					createFromJson( const AssetBundlePtr& assets, const String& json );

	private:

										//! Constructs a Scene instance.
										Scene( void );

	private:

		Ecs::EcsPtr						m_ecs;				//!< Internal entity component system.
		Ecs::SystemGroupPtr				m_updateSystems;	//!< Update systems group.
		Array<RenderingSystemBasePtr>	m_renderingSystems;	//!< Entity rendering systems.
		Ecs::IndexPtr					m_cameras;			//!< All cameras that reside in scene.
		Ecs::IndexPtr					m_named;			//!< All named entities that reside in scene stored inside this family.
	};

	// ** Scene::addSystem
	template<typename TSystem, typename ... Args>
	WeakPtr<TSystem> Scene::addSystem( Args ... args )
	{
		WeakPtr<TSystem> system = m_updateSystems->add<TSystem>( args... );
		m_ecs->rebuildSystems();
		return system;
	}

	// ** Scene::system
	template<typename TSystem>
	WeakPtr<TSystem> Scene::system( void ) const
	{
		return m_updateSystems->get<TSystem>();
	}

	// ** Scene::addRenderingSystem
	template<typename TRenderingSystem>
	void Scene::addRenderingSystem( void )
	{
		m_renderingSystems.push_back( DC_NEW TRenderingSystem( m_ecs ) );
		m_ecs->rebuildSystems();
	}

	// ** Scene::createArchetype
	template<typename TArchetype>
	StrongPtr<TArchetype> Scene::createArchetype( const SceneObjectId& id, const io::Bson& data )
	{
		return m_ecs->createArchetype<TArchetype>( id, data );
	}

#ifdef HAVE_JSON

	//! Loads the scene from JSON file.
	class JsonSceneLoader : public io::JsonLoaderBase {
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

		//! Reads the Particles component from JSON object.
		Ecs::ComponentPtr			readParticles( const Json::Value& value );

		//! Reads the shape module from JSON object.
		bool						readModuleShape( Fx::ParticlesWPtr particles, const Json::Value& object );

		//! Reads the color module from JSON object.
		bool						readModuleColor( Fx::ParticlesWPtr particles, const Json::Value& object );

		//! Reads the emission module from JSON object.
		bool						readModuleEmission( Fx::ParticlesWPtr particles, const Json::Value& object );

		//! Reads the size module from JSON object.
		bool						readModuleSize( Fx::ParticlesWPtr particles, const Json::Value& object );

		//! Reads the rotation module from JSON object.
		bool						readModuleAngularVelocity( Fx::ParticlesWPtr particles, const Json::Value& object );

		//! Reads the acceleration module from JSON object.
		bool						readModuleAcceleration( Fx::ParticlesWPtr particles, const Json::Value& object );

		//! Reads the velocity module from JSON object.
		bool						readModuleVelocity( Fx::ParticlesWPtr particles, const Json::Value& object );

		//! Reads the initial module from JSON object.
		bool						readModuleInitial( Fx::ParticlesWPtr particles, const Json::Value& object );

		//! Reads the color parameter from JSON object.
		void						readColorParameter( Fx::RgbParameter& parameter, const Json::Value& object );

		//! Reads the scalar parameter from JSON object.
		void						readScalarParameter( Fx::FloatParameter& parameter, const Json::Value& object );

	private:

		//! Component loader type.
		typedef cClosure<Ecs::ComponentPtr(const Json::Value&)> ComponentLoader;

		//! Particle system module loader
		typedef cClosure<bool(Fx::ParticlesWPtr, const Json::Value&)> ModuleLoader;

		//! Container type to store particle module loaders.
		typedef Map<String, ModuleLoader>	ModuleLoaders;

		//! Container to store all available component loaders.
		typedef Map<String, ComponentLoader> ComponentLoaders;

		//! Container type to store parsed scene objects.
		typedef Map<String, Ecs::EntityPtr> SceneObjects;

		//! Container type to store parsed components.
		typedef Map<String, Ecs::ComponentPtr> Components;

		AssetBundlePtr				m_assets;					//!< Available assets.
		Json::Value					m_json;						//!< Parsed JSON.
		ScenePtr					m_scene;					//!< The scene to be loaded.
		SceneObjects				m_sceneObjects;				//!< Parsed scene objects.
		Components					m_components;				//!< Parsed components.
		ComponentLoaders			m_loaders;					//!< Available component loaders.
		ModuleLoaders				m_moduleLoaders;			//!< Available module loaders.
		Fx::IMaterialFactoryPtr		m_particleMaterialFactory;	//!< Constructs particle system materials.
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
	#include "Assets/Terrain.h"
	#include "Systems/TransformSystems.h"
	#include "Systems/Physics2D.h"
	#include "Systems/AssetSystem.h"
	#include "Systems/CullingSystems.h"
	#include "Archetypes/Camera.h"
	#include "Rendering/RenderTarget.h"
	#include "Rendering/RenderingContext.h"
	#include "Rendering/RenderingSystem.h"
	#include "Rendering/ShaderCache.h"
	#include "Rendering/Passes/DebugPasses.h"
	#include "Rendering/Passes/BasicPasses.h"
	#include "Rendering/ForwardLighting/LightPass.h"
#endif

#endif    /*    !__DC_Scene_H__    */
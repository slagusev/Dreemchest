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

#ifndef __DC_Scene_Component_Rendering_H__
#define __DC_Scene_Component_Rendering_H__

#include "../Scene.h"
#include "../Assets/Mesh.h"
#include "../Assets/Image.h"

DC_BEGIN_DREEMCHEST

namespace Scene {

	//! This component is attached to a camera to render unlit meshes.
	class RenderUnlitMeshes : public Component {
		OverrideComponent( RenderUnlitMeshes, Component )
	};

	//! This component is attached to a camera to render forward lit meshes.
	class RenderForwardLit : public Component {
		OverrideComponent( RenderForwardLit, Component )
	};

	//! This component is attached to a camera to render wireframe meshes.
	class RenderWireframeMeshes : public Component {
	public:

								OverrideComponent( RenderWireframeMeshes, Component )

		//! Returns mesh wireframe rendering color.
		const Rgba&				color( void ) const;

	private:

		Rgba					m_color;	//!< Mesh wireframe color.
	};

	//! This component is attached to a camera to render sprites.
	class RenderSprites : public Component {
		OverrideComponent( RenderSprites, Component )
	};

	//! This component is attached to a camera to debug render sprite transforms.
	class RenderSpriteTransform : public Component {
		OverrideComponent( RenderSpriteTransform, Component )
	};

	//! Holds the light information.
	class Light : public Component {
	public:

									OverrideComponent( Light, Component )

		//! Available light types.
		enum Type {
			  Point
			, Spot
			, Directional
		};

									//! Constructs Light instance.
									Light( Type type = Point, const Rgb& color = Rgb( 1.0f, 1.0f, 1.0f ), f32 intensity = 1.0f, f32 range = 1.0f )
										: m_type( type ), m_color( color ), m_intensity( intensity ), m_range( range ) {}

		//! Returns the light type.
		Type						type( void ) const;

		//! Sets the light type.
		void						setType( Type value );

		//! Returns the light color.
		const Rgb&					color( void ) const;

		//! Sets the light color.
		void						setColor( const Rgb& value );

		//! Returns the light intensity.
		f32							intensity( void ) const;

		//! Sets the light intensity.
		void						setIntensity( f32 value );

		//! Returns the light influence range.
		f32							range( void ) const;

		//! Sets the light influence range.
		void						setRange( f32 value );

	private:

		Type						m_type;			//!< Light type.
		Rgb							m_color;		//!< Light color.
		f32							m_intensity;	//!< Light intensity.
		f32							m_range;		//!< Light influence range.
	};

	//! Holds the static mesh data with per-instance materials.
	class StaticMesh : public Component {
	public:

									OverrideComponent( StaticMesh, Component )

									//! Constructs StaticMesh instance.
									StaticMesh( const MeshPtr& mesh = MeshPtr() )
										: m_mesh( mesh ), m_visibility( ~0 ) {}

		//! Returns mesh to be rendered.
		const MeshPtr&				mesh( void ) const;

		//! Sets a mesh to be rendered.
		void						setMesh( const MeshPtr& value );

		//! Returns mesh bounds.
		const Bounds&				bounds( void ) const;

		//! Returns true if the mesh is visible by camera.
		bool						isVisible( u8 camera ) const;

		//! Marks the mesh as visible from camera.
		void						setVisibilityMask( u16 mask, bool value );

		//! Returns the total number of materials.
		u32							materialCount( void ) const;

		//! Returns the material by index.
		MaterialPtr					material( u32 index ) const;

		//! Sets the material by index.
		void						setMaterial( u32 index, const MaterialPtr& value );

		//! Returns a lightmap texture.
		const Renderer::TexturePtr&	lightmap( void ) const;

		//! Sets a lightmap texture.
		void						setLightmap( const Renderer::TexturePtr& value );

	private:

		FlagSet16					m_visibility;	//!< Camera visibility mask.
		MeshPtr						m_mesh;			//!< Mesh to be rendered.
		Array<MaterialPtr>			m_materials;	//!< Mesh materials array.
		Renderer::TexturePtr		m_lightmap;		//!< Lightmap texture that is rendered for this mesh.
	};

	//! Holds the sprite rendering info.
	class Sprite : public Component {
	public:
			
									OverrideComponent( Sprite, Component )

									//! Constructs the Sprite instance.
									Sprite( const ImagePtr& image = ImagePtr(), const Rgba& color = Rgba( 1.0f, 1.0f, 1.0f, 1.0f ) )
										: m_image( image ), m_color( color ) {}

		//! Returns the sprite image.
		const ImagePtr&				image( void ) const;

		//! Returns the sprite color.
		const Rgba&					color( void ) const;

	private:

		ImagePtr					m_image;	//!< Sprite image.
		Rgba						m_color;	//!< Sprite color.
	};

	//! Scene rendering view.
	class View : public RefCounted {
	public:

		virtual						~View( void ) {}

		//! Returns the viewport split by it's coordinates.
		static Rect					calculateSplitRect( u32 x, u32 y, u32 nx, u32 ny );

		//! Returns the view rect.
		Rect						rect( void ) const { return Rect( 0.0f, 0.0f, ( f32 )width(), ( f32 )height() ); }

		//! Returns the view width.
		virtual u32					width( void ) const { return 0; }

		//! Returns the view height.
		virtual u32					height( void ) const { return 0; }

		//! Begins rendering to this view.
		virtual void				begin( void ) const {}

		//! Ends rendering to this view.
		virtual void				end( void ) const {}
	};

	//! WindowView is used for rendering the scene to window.
	class WindowView : public View {
	public:

		//! Returns the window width.
		virtual u32					width( void ) const;

		//! Returns the window height.
		virtual u32					height( void ) const;

		//! Creates the WindowView instance.
		static ViewPtr				create( const Platform::WindowWPtr& window );

	private:

									//! Constructs the WindowView instance.
									WindowView( const Platform::WindowWPtr& window );

	private:

		Platform::WindowWPtr		m_window;	//!< The output window.
	};

	//! Camera component.
	class Camera : public Component {
	public:

		//! Available camera projections.
		enum Projection {
			  Perspective = 0
			, Ortho
			, OrthoCenter
		};

		//! Camera clear flags.
		enum ClearFlags {
			  ClearColor	= BIT( 0 )					//!< Camera will clear the color buffer.
			, ClearDepth	= BIT( 1 )					//!< Camera will clear the depth buffer.
			, ClearAll		= ClearColor | ClearDepth	//!< Camera will clear all buffers.
		};

									OverrideComponent( Camera, Component )

									//! Constructs Camera instance.
									Camera( Projection projection = Perspective, const ViewPtr& view = ViewPtr(), const Rgba& clearColor = Rgba(), const Rect& ndc = Rect( 0.0f, 0.0f, 1.0f, 1.0f ) )
										: m_clearMask( ClearAll ), m_id( -1 ), m_projection( projection ), m_ndc( ndc ), m_view( view ), m_clearColor( clearColor ), m_fov( 60.0f ), m_near( 0.01f ), m_far( 1000.0f ) {}

		//! Returns camera clear mask.
		u8							clearMask( void ) const;

		//! Sets camera clear mask.
		void						setClearMask( u8 value );

		//! Sets the clear color.
		void						setClearColor( const Rgba& value );

		//! Returns the clear color.
		const Rgba&					clearColor( void ) const;

		//! Returns the camera id.
		u8							id( void ) const;

		//! Sets camera id.
		void						setId( u8 value );

		//! Returns camera field of view.
		f32							fov( void ) const;

		//! Sets camera field of view.
		void						setFov( f32 value );

		//! Returns the Z-near value.
		f32							near( void ) const;

		//! Sets the Z-near value.
		void						setNear( f32 value );

		//! Returns the Z-far value.
		f32							far( void ) const;

		//! Sets the Z-far value.
		void						setFar( f32 value );

		//! Sets the normalized device coordinates to render frame to.
		void						setNdc( const Rect& value );

		//! Returns the normalized device coordinates to render frame to.
		const Rect&					ndc( void ) const;

		//! Calculates the output viewport coordinates.
		Rect						viewport( void ) const;

		//! Sets the camera render view.
		void						setView( const ViewPtr& value );

		//! Returns the camera render view.
		const ViewPtr&				view( void ) const;

		//! Calculates the projection matrix.
		Matrix4						calculateProjectionMatrix( void ) const;

		//! Calculates the view projection matrix.
		Matrix4						calculateViewProjection( const Matrix4& transform ) const;

	private:

		u8							m_clearMask;	//!< Camera clear flags.
		u8							m_id;			//!< Unique camera id.
		Projection					m_projection;	//!< Camera projection.
		Rect						m_ndc;			//!< Output normalized device coordinates.
		ViewPtr						m_view;			//!< Render view.
		Rgba						m_clearColor;	//!< The clear color.
		f32							m_fov;			//!< Camera field of view.
		f32							m_near;			//!< Z-near value.
		f32							m_far;			//!< Z-far value.
	};

} // namespace Scene

DC_END_DREEMCHEST

#endif    /*    !__DC_Scene_Component_Rendering_H__    */
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

#include "AssetTree.h"

#include "Menu.h"
#include "FileSystem.h"
#include "Document.h"
#include "MimeData.h"
#include "ObjectInspectorPrivate.h"
#include "../../Models/Qt/PropertyModelPrivate.h"
#include "../../Models/Qt/AssetsModelPrivate.h"
#include "../../Project/Project.h"

DC_BEGIN_COMPOSER

namespace Ui {

// ------------------------------------------------ QAssetSelector ------------------------------------------------ //

// ** QAssetSelector::QAssetSelector
QAssetSelector::QAssetSelector( u32 mask, QWidget* parent ) : QWidget( parent ), m_mask( mask )
{
	qRegisterMetaType<Scene::AssetWPtr>( "Scene::AssetWPtr" );

	m_line = new QLineEdit;
	m_line->installEventFilter( this );
	m_line->setReadOnly( true );

	QHBoxLayout* layout = new QHBoxLayout( this );
	layout->addWidget( m_line );
	layout->addWidget( new QToolButton );
	layout->setMargin( 0 );
	layout->setSpacing( 1 );
}

// ** QAssetSelector::eventFilter
bool QAssetSelector::eventFilter( QObject* target, QEvent* e )
{
    switch( e->type() ) {
    case QEvent::DragEnter: {
								QDragEnterEvent* de = static_cast<QDragEnterEvent*>( e );

								// MIME data does not contain an accepted format.
								if( !de->mimeData()->hasFormat( Composer::kAssetMime.c_str() ) ) {
									return true;
								}

								// Only single assets are accepted
								if( de->mimeData()->urls().size() > 1 ) {
									return true;
								}

								// Decode asset
								Scene::AssetPtr asset = Composer::instance()->assetFromMime( new MimeData( de->mimeData() ) );

								// Check asset type
								if( !asset.valid() || (asset->type() & m_mask) == 0 ) {
									return true;
								}

								// Accept this action
								de->acceptProposedAction();
							}
							return true;

    case QEvent::Drop:		{
								QDropEvent* de = static_cast<QDropEvent*>( e );

								// Decode asset
								Scene::AssetWPtr asset = Composer::instance()->assetFromMime( new MimeData( de->mimeData() ) );
								
								// Set the value
								setValue( asset );

								// Emit the signal
								emit valueChanged();
							}
							return true;
    }

    return false;
}

// ** QAssetSelector::assetChanged
Scene::AssetWPtr QAssetSelector::value( void ) const
{
	return m_asset;
}

// ** QAssetSelector::assetChanged
void QAssetSelector::setValue( const Scene::AssetWPtr& value )
{
	// Save the asset pointer
	m_asset = value;

	// Update the text field
	m_line->setText( m_asset.valid() ? QString::fromStdString( m_asset->name() ) : "" );
}

// ------------------------------------------------ AssetTree ------------------------------------------------ //

// ** AssetTree::AssetTree
AssetTree::AssetTree( Project::ProjectWPtr project ) : PrivateInterface( new QAssetTree( project ) )
{
	m_private->setParent( this );
}

// ** AssetTree::selection
FileInfoArray AssetTree::selection( void ) const
{
	return m_private->selection();
}

// ** AssetTree::expandSelectedItems
void AssetTree::expandSelectedItems( void )
{
	return m_private->expandSelectedItems();
}

// ** AssetTree::setModel
void AssetTree::setModel( AssetsModelWPtr value )
{
	m_private->setModel( value );
}

// ------------------------------------------------ QAssetTree ------------------------------------------------ //

// ** QAssetTree::QAssetTree
QAssetTree::QAssetTree( Project::ProjectWPtr project ) : m_project( project ), m_selectionChanged( false )
{
	setHeaderHidden( true );
	setDragEnabled( true );
	setDropIndicatorShown( true );
	setDragDropOverwriteMode( true );
	setSelectionMode( ExtendedSelection );
	setDragDropMode( InternalMove );
	setEditTriggers( EditTrigger::EditKeyPressed );
	viewport()->setAcceptDrops( true );

	connect( this, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(itemDoubleClicked(const QModelIndex&) ) );
	connect( selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(selectionChanged(const QItemSelection&, const QItemSelection&)) );
}

// ** QAssetTree::setParent
void QAssetTree::setParent( IAssetTreeWPtr value )
{
	m_parent = value;
}

// ** QAssetTree::setModel
void QAssetTree::setModel( AssetsModelWPtr value )
{
	m_model = value;
	QTreeView::setModel( m_model->privateInterface<QAssetsModel>() );

#if !DEV_CUSTOM_ASSET_MODEL
	QAssetsModel* model = m_model->privateInterface<QAssetsModel>();
	setRootIndex( model->root() );

	for( s32 i = 1; i < model->columnCount(); i++ ) {
		setColumnHidden( i, true );
	}
#endif	/*	!DEV_CUSTOM_ASSET_MODEL	*/
}

// ** QAssetTree::keyPressEvent
void QAssetTree::keyPressEvent( QKeyEvent *event )
{
	QAssetsModel* model = m_model->privateInterface<QAssetsModel>();

    switch( event->key() ) {
    case Qt::Key_Delete:	foreach( QModelIndex idx, selectedIndexes() ) {
								model->remove( idx );
							}
							break;
    }

    QTreeView::keyPressEvent( event );
}

// ** QAssetTree::contextMenuEvent
void QAssetTree::contextMenuEvent( QContextMenuEvent *e )
{
	IMenuPtr menu( new Menu( this ) );

    m_project->fillAssetMenu( menu, m_parent );
    menu->exec( e->globalPos().x(), e->globalPos().y() );
}

// ** QAssetTree::itemDoubleClicked
void QAssetTree::itemDoubleClicked( const QModelIndex& index )
{
	QAssetsModel* model = m_model->privateInterface<QAssetsModel>();

	// Get the file info by index
	FileInfoPtr file = model->assetFile( index );

	if( file->isDir() ) {
		return;
	}

	// Read the corresponding meta data
	Io::KeyValue data = m_model->metaData( file );

	if( !data.isObject() ) {
		return;
	}

	// Open the asset editor
	m_project->edit( data["uuid"].asString(), file );
}

// ** QAssetTree::mousePressEvent
void QAssetTree::mousePressEvent( QMouseEvent* e )
{
	// Reset the flag
	m_selectionChanged = false;

	// Run the base class method
	QTreeView::mousePressEvent( e );
}

// ** QAssetTree::mouseReleaseEvent
void QAssetTree::mouseReleaseEvent( QMouseEvent* e )
{
	QTreeView::mouseReleaseEvent( e );

	// The selection was not changed - skip
	if( !m_selectionChanged ) {
		return;
	}

	// Bind to an object inspector
	bindToInspector( selectedIndexes() );

	// Reset the flag
	m_selectionChanged = false;
}

// ** QAssetTree::selectionChanged
void QAssetTree::selectionChanged( const QItemSelection& selected, const QItemSelection& deselected )
{
	m_selectionChanged = true;
}

// ** QAssetTree::bindToInspector
void QAssetTree::bindToInspector( const QModelIndexList& indexes )
{
	// More that one item selected - do nothing
	if( indexes.size() > 1 ) {
		return;
	}

	// Get the inspector widget
	ObjectInspectorWPtr inspector = Composer::instance()->window()->objectInspector();

	// The asset was deselected - hide inspector
	if( indexes.empty() ) {
		inspector->setModel( PropertyModelWPtr() );
		return;
	}

	// Get the asset file by index
	FileInfoPtr file = m_model->privateInterface<QAssetsModel>()->assetFile( indexes[0] );

	// No meta data found - skip
	if( !m_model->hasMetaData( file ) ) {
		return;
	}

	// Extract the UUID from file asset
	String uuid = m_model->uuid( file );

	// Find asset by UUID
	Scene::AssetWPtr asset = m_project->assets()->findAsset( uuid );
	DC_BREAK_IF( !asset.valid() );
	
	// Bind the selected asset to an object inspector.
	switch( asset->type() ) {
	case Scene::Asset::Material:	inspector->setModel( createMaterialModel( castTo<Scene::Material>( asset.get() ) ) );
									break;
	default:						inspector->setModel( PropertyModelWPtr() );
	}
}

// ** QAssetTree::selection
FileInfoArray QAssetTree::selection( void ) const
{
	QAssetsModel* model = m_model->privateInterface<QAssetsModel>();

	FileInfoArray result;

	foreach( QModelIndex idx, selectedIndexes() ) {
	#if DEV_CUSTOM_ASSET_MODEL
		result.push_back( model->asset( idx ).absoluteFilePath().toStdString() );
	#else
		result.push_back( model->assetFile( idx ) );
	#endif
	}

	return result;
}

// ** QAssetTree::expandSelectedItems
void QAssetTree::expandSelectedItems( void )
{
	foreach( QModelIndex idx, selectedIndexes() ) {
		expand( idx );
	}
}

} // namespace Ui

DC_END_COMPOSER
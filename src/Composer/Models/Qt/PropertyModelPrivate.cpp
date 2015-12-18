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

#include "PropertyModelPrivate.h"

DC_BEGIN_COMPOSER

// ** QPropertyModel::QPropertyModel
QPropertyModel::QPropertyModel( QObject* parent ) : QAbstractItemModel( parent )
{

}

// ** QPropertyModel::propertyCount
int QPropertyModel::propertyCount( void ) const
{
	return ( int )m_properties.size();
}

// ** QPropertyModel::propertyName
QString QPropertyModel::propertyName( int index ) const
{
	return m_properties[index]->name();
}

// ** QPropertyModel::propertyType
QString QPropertyModel::propertyType( int index ) const
{
	return m_properties[index]->type();
}

// ** QPropertyModel::rowCount
int QPropertyModel::rowCount( const QModelIndex& parent ) const
{
	return 1;
}

// ** QPropertyModel::parent
QModelIndex QPropertyModel::parent( const QModelIndex& child ) const
{
	return QModelIndex();
}

// ** QPropertyModel::columnCount
int QPropertyModel::columnCount( const QModelIndex& parent ) const
{
	return propertyCount();
}

// ** QPropertyModel::data
QVariant QPropertyModel::data( const QModelIndex& index, int role ) const
{
	int idx = index.column();
	return m_properties[idx]->get( idx );
}

// ** QPropertyModel::setData
bool QPropertyModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
	int idx = index.column();
	m_properties[idx]->set( idx, value );

	return true;
}

// ** QPropertyModel::index
QModelIndex QPropertyModel::index( int row, int column, const QModelIndex& parent ) const
{
	return createIndex( row, column, ( void* )NULL );
}

DC_END_COMPOSER
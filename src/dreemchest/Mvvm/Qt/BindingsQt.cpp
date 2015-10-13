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

#include "BindingsQt.h"

DC_BEGIN_DREEMCHEST

namespace mvvm {

// ---------------------------------------- QtVisibilityBinding ---------------------------------------- //

// ** QtVisibilityBinding::handleValueChanged
void QtVisibilityBinding::handleValueChanged( void )
{
    widget()->setVisible( m_property->get() );
}

// ----------------------------------------- QtEnabledBinding ---------------------------------------- //

// ** QtEnabledBinding::handleValueChanged
void QtEnabledBinding::handleValueChanged( void )
{
    widget()->setEnabled( m_property->get() );
}

// ---------------------------------------- QtPushButtonBinding ---------------------------------------- //

// ** QtPushButtonBinding::handleViewChanged
void QtPushButtonBinding::handleViewChanged( void )
{
	m_property->invoke();
}

// ---------------------------------------- QtListWidgetBinding ---------------------------------------- //

// ** QtListWidgetBinding::handleValueChanged
void QtListWidgetBinding::handleValueChanged( void )
{
	QListWidget* list = widget();

	list->clear();

	for( s32 i = 0, n = ( s32 )m_property->size(); i < n; i++ ) {
		list->addItem( m_property->get( i )->get().c_str() );
	}
}

// ---------------------------------------- QtStackedWidgetBinding ---------------------------------------- //

// ** QtStackedWidgetBinding::handleValueChanged
void QtStackedWidgetBinding::handleValueChanged( void )
{
	QStackedWidget* stackedWidget = widget();
	QObject*		parent		  = stackedWidget->parent();

	QWidget* page = parent->findChild<QWidget*>( m_property->get().c_str() );
	DC_BREAK_IF( page == NULL );

	stackedWidget->setCurrentWidget( page );
}
/*
// ----------------------------------------- MyGUICaptionBinding ----------------------------------------- //

// ** MyGUICaptionBinding::MyGUICaptionBinding
MyGUICaptionBinding::MyGUICaptionBinding( View* view, const String& name, const Property& property )
    : MyGUIPropertyBinding( view, name, property )
{

}

// ** MyGUICaptionBinding::handlePropertyChanged
void MyGUICaptionBinding::handlePropertyChanged( const Value& value )
{
    if( value != m_widget->getCaption().asUTF8() ) {
        m_widget->setCaption( value );
    }
}
*/
// ----------------------------------------- QtLineEditBinding ----------------------------------------- //

// ** QtLineEditBinding::handleValueChanged
void QtLineEditBinding::handleValueChanged( void )
{
	const String& value		= m_property->get();
	QLineEdit*	  lineEdit	= widget();

    if( value.c_str() != lineEdit->text() ) {
        lineEdit->setText( value.c_str() );
    }
}

// ** QtLineEditBinding::handleViewChanged
void QtLineEditBinding::handleViewChanged( void )
{
	m_property->set( widget()->text().toUtf8().constData() );
}

// ----------------------------------------- QtLabelBinding ----------------------------------------- //

// ** QtLabelBinding::handleValueChanged
void QtLabelBinding::handleValueChanged( void )
{
	const String& value = m_property->get();
	QLabel*	      label	= widget();

    if( value.c_str() != label->text() ) {
        label->setText( value.c_str() );
    }
}

// ------------------------------------------------- QtBindingFactory ------------------------------------------------- //

// ** QtBindingFactory::QtBindingFactory
QtBindingFactory::QtBindingFactory( void )
{
	registerBinding<QtLabelBinding, QLabel>();
	registerBinding<QtLineEditBinding, QLineEdit>();
	registerBinding<QtStackedWidgetBinding, QStackedWidget>();
	registerBinding<QtListWidgetBinding, QListWidget>();
	registerBinding<QtEnabledBinding, QWidget>( "enabled" );
	registerBinding<QtVisibilityBinding, QWidget>( "visible" );
	registerBinding<QtPushButtonBinding, QPushButton>( "click" );
}

// ** QtBindingFactory::create
BindingFactoryPtr QtBindingFactory::create( void )
{
	return BindingFactoryPtr( DC_NEW QtBindingFactory );
}

// --------------------------------------------------- QtBindings --------------------------------------------------- //

// ** QtBindings::QtBindings
QtBindings::QtBindings( const BindingFactoryPtr& factory, const ObjectWPtr& root, QWidget* widget ) : Bindings( factory, root ), m_widget( widget )
{

}

// ** QtBindings::create
BindingsPtr QtBindings::create( const BindingFactoryPtr& factory, const ObjectWPtr& root, QWidget* widget )
{
	return BindingsPtr( DC_NEW QtBindings( factory, root, widget ) );
}

// ** QtBindings::resolveWidgetPrototypeChain
WidgetPrototypeChain QtBindings::resolveWidgetPrototypeChain( const String& name ) const
{
	QWidget* widget = m_widget->findChild<QWidget*>( name.c_str() );
	
	if( !widget ) {
		return WidgetPrototypeChain();
	}

	const QMetaObject*	 metaObject = widget->metaObject();
	WidgetPrototypeChain result;

	while( metaObject ) {
		CString className = metaObject->className();
		result.push_back( StringHash( className ) );
		metaObject = metaObject->superClass();
	}

	return result;
}

// ** QtBindings::findWidget
Widget QtBindings::findWidget( const String& name ) const
{
	return m_widget->findChild<QWidget*>( name.c_str() );
}

} // namespace mvvm

DC_END_DREEMCHEST

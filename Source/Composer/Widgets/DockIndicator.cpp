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

#include "DockIndicator.h"

DC_BEGIN_COMPOSER

extern char* IndicatorImage[];

namespace Ui {

// ** DockIndicator::DockIndicator
DockIndicator::DockIndicator( QMainWindow* window, const QColor& color, int transparency )
	: QWidget( window, Qt::Widget ), m_color( color ), m_transparency( transparency ), m_window( window ), m_grabbed( NULL ), m_underlying( NULL )
{
	// Install the event filter
	m_window->installEventFilter( this );

	// Create indicator image
	m_indicator = QImage( IndicatorImage );
}

// ** DockIndicator::indicatorCenter
QPoint DockIndicator::indicatorCenter( void ) const
{
	Q_ASSERT( m_underlying != NULL );
	return m_underlying->pos() + QPoint( m_underlying->width() / 2, m_underlying->height() / 2 );
}

// ** DockIndicator::begin
void DockIndicator::begin( QDockWidget* dock )
{
	Q_ASSERT( dock );
	Q_ASSERT( m_grabbed == NULL );

	// Set the widget
	m_grabbed = dock;

	// Show an resize the indicator
	show();
	resize( m_window->size() );

	// Render the dock widget to image
	m_dock = widgetToImage( m_grabbed, QPainter::CompositionMode_DestinationOut ); 
}

// ** DockIndicator::end
void DockIndicator::end( QDockWidget* dock )
{
	Q_ASSERT( m_grabbed != NULL );
	Q_ASSERT( m_grabbed == dock );

	// Place the dock widget to an underlying dock.
	if( m_underlying ) {
		int pane = activePane();

		if( pane ) {
			placeToPane( dock, m_underlying, activePane() );
		}
	}

	// Hide the indicator
	hide();

	// Destroy the dock image
	m_dock = QImage();

	// Set grabbed widget to NULL
	m_grabbed = NULL;
}

// ** DockIndicator::widgetToImage
QImage DockIndicator::widgetToImage( QWidget* widget, QPainter::CompositionMode mode ) const
{
	// Render the dock widget to image
#ifdef DC_QT4_ENABLED
	QImage im = QPixmap::grabWidget( widget ).toImage();
#elif DC_QT5_ENABLED
	QImage im = widget->grab().toImage();
#endif	/*	DC_QT4_ENABLED	*/

	// Construct the destination image
	QImage result = QImage( im.size(), QImage::Format_ARGB32_Premultiplied );

	// Pain dock widget image with blending.
	QPainter p( &result );
	p.drawImage( QPoint(), im );
	p.setBrush( QColor( 0, 0, 0, 100 ) );
	p.setCompositionMode( mode );
	p.drawRect( im.rect() );
	p.end();

	return result;
}

// ** DockIndicator::activePaneRect
QRect DockIndicator::activePaneRect( void ) const
{
	Q_ASSERT( m_underlying != NULL );

	QRect rect = m_underlying->rect();
	rect.moveTo( m_underlying->pos() );

	int pane = activePane();

	switch( pane ) {
	case Qt::AllDockWidgetAreas:	return rect;
	case Qt::TopDockWidgetArea:		return rect.adjusted( 0, 0, 0, -rect.height() / 2 );
	case Qt::BottomDockWidgetArea:	return rect.adjusted( 0, rect.height() / 2, 0, 0 );
	case Qt::LeftDockWidgetArea:	return rect.adjusted( 0, 0, -rect.width() / 2, 0 );
	case Qt::RightDockWidgetArea:	return rect.adjusted( rect.width() / 2, 0, 0, 0 );
	}

	return QRect();
}

// ** DockIndicator::activePane
int DockIndicator::activePane( void ) const
{
	if( !m_underlying ) {
		return 0;
	}

	QPoint offset = m_cursor - indicatorCenter();
	
	if ( QRect( -Radius, -Radius, Radius*2, Radius*2 ).contains( offset ) )
		return Qt::AllDockWidgetAreas;
	else if ( QRect( -Radius, -Radius*3, Radius*2, Radius*2 ).contains( offset ) )
		return Qt::TopDockWidgetArea;
	else if ( QRect( -Radius, Radius, Radius*2, Radius*2 ).contains( offset ) )
		return Qt::BottomDockWidgetArea;
	else if ( QRect( -Radius*3, -Radius, Radius*2, Radius*2 ).contains( offset ) )
		return Qt::LeftDockWidgetArea;
	else if ( QRect( Radius, -Radius, Radius*2, Radius*2 ).contains( offset ) )
		return Qt::RightDockWidgetArea;

	return 0;
}

class QRenderingFrame;

// ** DockIndicator::update
void DockIndicator::update( const QPoint& cursor )
{
	if( !m_grabbed ) {
		return;
	}

	m_underlying	= findDockAtPoint( cursor );
	m_cursor		= m_window->mapFromGlobal( cursor );

	QWidget::update();
}

// ** DockIndicator::paintEvent
void DockIndicator::paintEvent( QPaintEvent* e )
{
	QPainter p( this );

	if( !m_underlying ) {
		return;
	}

	QPoint offset = QPoint( m_indicator.width() / 2, m_indicator.height() / 2 );

	p.save();

	QPen pen( m_color.dark( 120 ) );

	p.setPen( pen );
	p.setBrush( QColor( m_color.red(), m_color.green(), m_color.blue(), m_transparency ) );
	p.drawRect( activePaneRect() );

	p.drawImage( m_cursor, m_dock );
	p.drawImage( indicatorCenter() - offset, m_indicator );
	p.restore();
}

// ** DockIndicator::eventFilter
bool DockIndicator::eventFilter( QObject* sender, QEvent* e )
{
	// Get the event type
	QEvent::Type type = e->type();

	// Process an event from a dock widget
	if( QDockWidget* dock = qobject_cast<QDockWidget*>( sender ) ) {
        // Type cast the mouse event
        QMouseEvent* me = static_cast<QMouseEvent*>( e );

        if( me->button() != Qt::LeftButton ) {
            return false;
        }

        // Process the mouse event
		switch( type ) {
		case QEvent::MouseButtonPress:		begin( dock );	return true;
		case QEvent::MouseButtonRelease:	end( dock );	return false;
		}
	}

	// Process an event from a main window
	if( QMainWindow* window = qobject_cast<QMainWindow*>( sender ) ) {
		switch( type ) {
		case QEvent::ChildAdded:	raise();

									if( QDockWidget* dock = qobject_cast<QDockWidget*>( static_cast<QChildEvent*>( e )->child() ) ) {
										dock->installEventFilter( this );
									}
									break;

		case QEvent::ChildRemoved:	if( QDockWidget* dock = qobject_cast<QDockWidget*>( static_cast<QChildEvent*>( e )->child() ) ) {
										Q_ASSERT( false );
									}

		case QEvent::MouseMove:		update( static_cast<QMouseEvent*>( e )->globalPos() );
									break;
		}
	}

	return false;
}

// ** DockIndicator::findDockAtPoint
QDockWidget* DockIndicator::findDockAtPoint( const QPoint& point ) const
{
	// Get the lost of all dock widgets
	QList<QDockWidget*> dockWidgets = m_window->findChildren<QDockWidget*>();

	// For each dock widget in list
	foreach( QDockWidget* dock, dockWidgets ) {
		// Skip the dock widget being grabbed
		if( dock == m_grabbed ) {
			continue;
		}

		// Check if the point is inside the dock widget
		if( dock->rect().contains( dock->mapFromGlobal( point ) ) ) {
			return dock;
		}
	}

	// Nothing found
	return NULL;
}

// ** DockIndicator::placeToPane
void DockIndicator::placeToPane( QDockWidget* dock, QDockWidget* destination, int pane )
{
	Q_ASSERT( pane );
	Q_ASSERT( dock );
	Q_ASSERT( destination );

	int w = destination->width() / 2;
	int h = destination->height() / 2;

	switch( pane ) {
	case Qt::AllDockWidgetAreas:	m_window->tabifyDockWidget( destination, dock );
									break;

	case Qt::RightDockWidgetArea:	m_window->splitDockWidget( destination, dock, Qt::Horizontal );
									setDockSize( destination, QSize( w, destination->height() ) );
									setDockSize( dock, QSize( w, dock->height() ) );
									break;

	case Qt::LeftDockWidgetArea:	m_window->splitDockWidget( destination, dock, Qt::Horizontal );
									m_window->splitDockWidget( dock, destination, Qt::Horizontal );
									setDockSize( destination, QSize( w, destination->height() ) );
									setDockSize( dock, QSize( w, dock->height() ) );
									break;

	case Qt::BottomDockWidgetArea:	m_window->splitDockWidget( destination, dock, Qt::Vertical );
									setDockSize( destination, QSize( destination->width(), h ) );
									setDockSize( dock, QSize( dock->width(), h ) );
									break;

	case Qt::TopDockWidgetArea:		m_window->splitDockWidget( destination, dock, Qt::Vertical );
									m_window->splitDockWidget( dock, destination, Qt::Vertical );
									setDockSize( destination, QSize( destination->width(), h ) );
									setDockSize( dock, QSize( dock->width(), h ) );
									break;
	}
}

// ** DockIndicator::setDockSize
void DockIndicator::setDockSize( QDockWidget* dock, const QSize& value ) const
{
	Q_ASSERT( dock );

	QSize min = dock->minimumSize();
	QSize max = dock->maximumSize();

	dock->setMinimumSize( value );
	dock->setMaximumSize( value );

	m_window->update();

	qApp->processEvents();

	dock->setMinimumSize( min );
	dock->setMaximumSize( max );
}

} // namespace Ui

DC_END_COMPOSER

/* XPM */
static char * IndicatorImage[]={
"88 88 164 2",
"Qt c None",
".A c #3b4963",
"ai c #3c4a65",
".z c #3e4c66",
"ak c #3e4c67",
"ah c #3f4d68",
".y c #414f6a",
"#c c #4170ca",
"ag c #42506c",
"#b c #4372cb",
".x c #45536c",
"#a c #4574cd",
"af c #46546e",
"#S c #4662bc",
"#Q c #4662bd",
"#R c #4663bd",
".w c #495671",
"## c #4976ce",
"ae c #4a5773",
"#O c #4c4faa",
".U c #4c77ce",
"#P c #4d4fa9",
"#N c #4d4faa",
".v c #4d5b75",
"#. c #4d7ad1",
"ad c #4e5c77",
"aj c #4f3f97",
".T c #4f7ad0",
".9 c #507dd2",
".B c #51409a",
".u c #515e79",
"#0 c #52419b",
".V c #52419c",
"ac c #52607b",
".S c #547ed3",
".8 c #5580d5",
".t c #56637e",
"ab c #576580",
".R c #5882d5",
".7 c #5a84d7",
".s c #5b6883",
"aa c #5c6a85",
".6 c #5d88d9",
".Q c #5e87d8",
".r c #5f6c88",
"a# c #616e8a",
".5 c #628bdb",
".q c #63718c",
"ao c #656b96",
"ap c #656b97",
"a. c #65738e",
".P c #658cdc",
".4 c #668fde",
"at c #676d95",
".p c #677591",
"#9 c #697793",
".o c #6b7a95",
".3 c #6b93e0",
".O c #6c93df",
"#8 c #6d7c97",
".2 c #6e96e2",
".n c #6f7d9a",
"#7 c #717f9c",
".1 c #7299e4",
".m c #73819d",
".N c #7398e3",
"am c #74819d",
"#6 c #75839f",
".0 c #759ce5",
".l c #7684a0",
".j c #7685a0",
"aB c #777ba3",
".Z c #779de7",
"al c #7885a2",
"#5 c #7886a3",
".k c #7887a2",
".M c #799ee7",
"aA c #7a7ea5",
"#4 c #7a89a5",
".Y c #7a9fe8",
"ar c #7b7fa6",
".L c #80a5eb",
"ax c #8186a5",
"aw c #868ba9",
"ay c #878ca9",
".K c #87aaef",
"aF c #888ca9",
"aE c #888daa",
".J c #8db0f2",
".I c #93b5f5",
".H c #99b9f7",
".G c #9dbdfb",
"aC c #a0a3ba",
".F c #a0c0fc",
"az c #a3a6bd",
"au c #a7aac1",
"aD c #a8a8a8",
"av c #b1b3ca",
"aq c #b4c8e1",
"an c #b4c9e1",
".# c #b5b5b5",
"#f c #b6cbe2",
"#g c #b7cbe2",
"#e c #b7cbe3",
"#h c #b7cce3",
"#k c #bbcee4",
"#i c #bbcee5",
"#m c #bbcfe4",
"#l c #bbcfe5",
"#j c #bccee5",
"aG c #bdbdbd",
".W c #c0c0c0",
"#o c #c0d2e7",
"#p c #c0d2e8",
"#r c #c0d3e7",
"#q c #c1d2e7",
"#n c #c1d2e8",
".a c #c3c3c3",
"#V c #c4c4c4",
".b c #c5c5c5",
"#t c #c5d6ea",
"#u c #c5d7ea",
".c c #c6c6c6",
"#s c #c6d6ea",
"#v c #c6d7ea",
"#Z c #c8c8c8",
".d c #c9c9c9",
"#z c #cbdaec",
"#x c #cbdaed",
"#y c #cbdbec",
"#w c #cbdbed",
".e c #cccccc",
".f c #cecece",
"#T c #cfcfcf",
"#F c #cfdeef",
"#D c #cfdfef",
"aH c #d0d0d0",
"#B c #d0deef",
"#E c #d0def0",
"#A c #d0dfef",
"#C c #d0dff0",
".g c #d1d1d1",
"#W c #d2d2d2",
"#d c #d4d4d4",
"#I c #d4e1f1",
"#H c #d4e1f2",
"#G c #d4e2f1",
"#K c #d4e2f2",
".C c #d5d5d5",
"#J c #d5e2f1",
".h c #d7d7d7",
"#L c #d7e4f3",
".i c #d8d8d8",
"#U c #d9d9d9",
".D c #dcdcdc",
"#3 c #dedede",
"#Y c #dfdfdf",
".E c #e0e0e0",
"#X c #e1e1e1",
"#2 c #e2e2e2",
"#1 c #e3e3e3",
".X c #e4e4e4",
"as c #e5e6ee",
"#M c #ffffff",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.a.a.b.c.c.c.c.c.c.c.c.c.c.c.c.c.c.c.c.c.c.c.c.c.c.c.c.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.a.c.d.e.f.f.f.f.f.f.f.f.f.f.f.f.f.f.f.f.f.f.f.f.f.f.f.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.b.d.g.h.i.i.i.i.i.i.i.i.i.i.i.i.i.i.i.i.i.i.i.i.i.i.i.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.e.h.j.k.l.m.n.o.p.q.r.s.t.u.v.w.v.w.x.y.z.A.B.C.D.E.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.B.F.F.G.H.I.J.K.L.M.N.O.P.Q.P.Q.R.S.T.U.V.W.i.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.B.Y.Z.0.1.2.3.4.5.6.7.8.9#..9#.###a#b#c.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.B#e#e#f#g#g#g#e#e#g#e#h#e#g#g#g#e#g#g#e.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.B#i#i#j#i#k#l#i#k#l#l#k#i#k#k#m#l#j#i#k.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.B#n#o#p#o#o#o#o#q#o#o#r#o#o#p#o#r#o#q#o.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.B#s#t#u#t#t#v#u#s#s#u#t#t#t#t#t#u#u#u#s.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.B#w#w#x#x#w#w#x#w#y#z#z#w#w#w#x#w#w#w#x.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.B#A#B#B#C#B#B#B#B#D#C#B#E#A#A#A#F#F#A#A.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.B#G#H#I#H#J#H#K#G#G#K#G#K#K#G#H#I#G#K#G.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.B#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.B#L.V#L.V#L.V#L.V#L.V#L.V#L.V#L.V#L.V#L.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.B#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.B#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.B#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.B#M#M#M#M#M#M#M#M#M.V#M#M#M#M#M#M#M#M#M.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.B#M#M#M#M#M#M#M#M.V.V.V#M#M#M#M#M#M#M#M.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.B#M#M#M#M#M#M#M#N#N#N#O#P#M#M#M#M#M#M#M.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.B#M#M#M#M#M#M#Q#R#S#S#S#S#Q#M#M#M#M#M#M.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.#.c.f.i.B#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M.V.##d.X.#.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.a.a.c#T#U.B#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M.V.##d.X.E.D.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.a#V#V.d#W.D.B#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M.V.##d.X#X#Y.h.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.a#V#Z#Z#T.h#Y#0#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M.V.##d.X#1#2#3.h.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.a#V#Z#T#T.h#3#2.V.V.V.V.V.V.V.V.V.V.V.V.V.V.V.V.V.V.V.V.V.##d.X.X.X#2#3.h.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.a#V#Z#T.h.h#3#2.X.i.W.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.W.i.X.X.X.X#2#3.h.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
".#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.a#V#Z#T.h#3#2.X.X.E.i#d#d#d#d#d#d#d#d#d#d#d#d#d#d#d#d#d#d#d.i.E.X.X.X.X.X#2#3.h.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#",
".#.a.a.b.c.c.c.c.c.c.c.c.c.c.c.c.c.c.c.c.c.c.c.c.c.d#T.h#3#2.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X#2#3.h#T.d.c.c.c.c.c.c.c.c.c.c.c.c.c.c.c.c.c.c.c.c.#",
".#.a.c.d.e.f.f.f.f.f.f.f.f.f.f.f.f.f.f.f.f.f.f.f#T#W.h#3#2.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X#2#3.h#W#T.f.f.f.f.f.f.f.f.f.f.f.f.f.f.f.f.f.f.f.#",
".#.b.d.g.h.i.i.i.i.i.i.i.i.i.i.i.i.i.i.i.i.i.i.i#U.D#Y#2.X.X.X.X.V#4#4#5#5#6#7#8#9a.a#aaabacadaeadaeafagahai.V.i.E.X.X.X.X.X.X.X#2#Y.D#U.i.i.i.i.i.i.i.i.i.i.i.i.i.i.i.i.i.i.i.#",
".#.c.e.haj.k.k.l.k.l.m.n.o.p.q.r.s.t.u.v.w.v.w.x.yakai.V.i.E.X.X.V.G.H.F.F.G.H.I.J.K.L.M.N.O.P.Q.P.Q.R.S.T.U.V.W.i.X.X.X.V#4#4#5#4alam.n.o.p.q.r.s.t.u.v.w.v.w.x.y.z.A.B.C.D.E.#",
".#.c.f.i.B.G.H.F.H.F.G.H.I.J.K.L.M.N.O.P.Q.P.Q.R.S.T.U.V.W.i.X.X.V.0.1.Z.Z.0.1.2.3.4.5.6.7.8.9#..9#.###a#b#c.V.##d.X.X.X.V.G.H.F.H.F.G.H.I.J.K.L.M.N.O.P.Q.P.Q.R.S.T.U.V.W.i.X.#",
".#.c.f.i.B.0.1.Z.1.Z.0.1.2.3.4.5.6.7.8.9#..9#.###a#b#c.V.##d.X.X.V#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M.V.##d.X.X.X.V.0.1.Z.1.Z.0.1.2.3.4.5.6.7.8.9#..9#.###a#b#c.V.##d.X.#",
".#.c.f.i.Banan#g#j#o#u#w#F#G#L#L#M#M#M#M#M#M#M#M#M#M#M.V.##d.X.X.V#M#Lao#Lap#L.V#L.V#L.V#L.V#L.V#Lap#Lao#L#M.V.##d.X.X.X.V#M#M#M#M#M#M#M#M#M#M#M#L#L#I#B#x#u#p#j#fanan.V.##d.X.#",
".#.c.f.i.Banan#e#l#r#u#w#F#I#L.V#M#M#M#M#M#M#M#M#M#M#M.V.##d.X.X.V#Mao#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#Lao#M.V.##d.X.X.X.V#M#M#M#M#M#M#M#M#M#M#M.V#L#H#C#x#t#o#i#ganan.V.##d.X.#",
".#.c.f.i.Banan#g#m#o#t#x#A#H#L#L#M#M#M#M#M#M#M#M#M#M#M.V.##d.X.X.V#M#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#M.V.##d.X.X.X.V#M#M#M#M#M#M#M#M#M#M#M#L#L#J#B#w#t#o#k#gaqan.V.##d.X.#",
".#.c.f.i.Banan#g#k#p#t#w#A#G#L.V#M#M#M#M#M#M#M#M#M#M#M.V.##d.X.X.V#M.V#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L.V#M.V.##d.X.X.X.V#M#M#M#M#M#M#M#M#M#M#M.V#L#H#B#w#v#o#l#ganan.V.##d.X.#",
".#.c.f.i.Banan#g#k#o#t#w#A#K#L#L#M#M#M#M#M#M#M#M#M#M#M.V.##d.X.X.V#M#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#M.V.##d.X.X.X.V#M#M#M#M#M#M#M#M#M#M#M#L#L#K#B#x#u#o#i#eanaq.V.##d.X.#",
".#.c.f.i.Baqan#e#i#o#t#w#E#K#L.V#M#M#M#M#M#M#Q#M#M#M#M.V.##d.X.X.V#M.V#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L.V#M.V.##d.X.X.X.V#M#M#M#M#Q#M#M#M#M#M#M.V#L#G#B#w#s#q#k#eanan.V.##d.X.#",
".#.c.f.i.Banaq#h#k#r#t#z#B#G#L#L#M#M#M#M#M#P#S#M#M#M#M.V.##d.X.X.V#M#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#M.V.##d.X.X.X.V#M#M#M#M#R#N#M#M#M#M#M#L#L#G#D#y#s#o#l#ganan.V.##d.X.#",
".#.c.f.i.Banan#e#l#o#u#z#C#K#L.V#M#M#M#M.V#O#S#M#M#M#M.V.##d.X.X.V#Map#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#Lap#M.V.##d.X.X.X.V#M#M#M#M#S#N.V#M#M#M#M.V#L#K#C#z#u#o#l#eanan.V.##d.X.#",
".#.c.f.i.Banan#g#l#o#s#y#D#G#L#L#M#M#M.V.V#N#S#M#M#M#M.V.##d.X.X.V#M#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#M.V.##d.X.X.X.V#M#M#M#M#S#N.V.V#M#M#M#L#L#G#B#z#t#r#k#haqan.V.##d.X.#",
".#.c.f.i.Banan#e#k#q#s#w#B#G#L.V#M#M#M#M.V#N#S#M#M#M#M.V.##d.X.X.V#M.V#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L.V#M.V.##d.X.X.X.V#M#M#M#M#S#O.V#M#M#M#M.V#L#K#E#w#t#o#i#eanaq.V.##d.X.#",
".#.c.f.i.Baqan#e#i#o#u#x#B#K#L#L#M#M#M#M#M#N#R#M#M#M#M.V.##d.X.X.V#M#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#M.V.##d.X.X.X.V#M#M#M#M#S#P#M#M#M#M#M#L#L#K#A#w#t#o#k#ganan.V.##d.X.#",
".#.c.f.i.Banan#g#l#o#v#w#B#H#L.V#M#M#M#M#M#M#Q#M#M#M#M.V.##d.X.X.V#M.V#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#L#Lao#M.V.##d.X.X.X.V#M#M#M#M#Q#M#M#M#M#M#M.V#L#G#A#w#t#p#k#ganan.V.##d.X.#",
".#.c.f.i.Banaq#g#k#o#t#w#B#J#L#L#M#M#M#M#M#M#M#M#M#M#M.V.##d.X.X.V#M#L#L#L#L#L#L#L.V#L.V#L.V#L.V#Lap#Lao#M#M.V.##d.X.X.X.V#M#M#M#M#M#M#M#M#M#M#M#L#L#H#A#x#t#o#m#ganan.V.##d.X.#",
".#.c.f.i.Banan#g#i#o#t#x#C#H#L.V#M#M#M#M#M#M#M#M#M#M#M.V.##d.X.X.V#Map#L#L#L#L#Lap#M#M#M#M#M#M#M#M#M#M#M#M#M.V.##d.X.X.X.V#M#M#M#M#M#M#M#M#M#M#M.V#L#I#F#w#u#r#l#eanan.V.##d.X.#",
".#.c.f.i.Banan#f#j#p#u#x#B#I#L#L#M#M#M#M#M#M#M#M#M#M#M.V.##d.X.X.V#M#L#L#L#L#L#L#L#M.V.V.V.V.V.V.V.V.V.V.V.V.V.##d.X.X.X.V#M#M#M#M#M#M#M#M#M#M#M#L#L#G#F#w#u#o#j#ganan.V.##d.X.#",
".#.c.f.i.Banan#e#i#o#t#w#B#H#L.V#M#M#M#M#M#M#M#M#M#M#M.V.##d.X.X.V#Mao#L#L#L#L#Lao#M.V#M#M#M#M#M.V#M#M#M#M#M.V.##d.X.X.X.V#M#M#M#M#M#M#M#M#M#M#M.V#L#K#A#w#u#q#i#ganan.V.##d.X.#",
".#.c.f.i.Banan#e#i#n#s#w#A#G#L#L#M#M#M#M#M#M#M#M#M#M#M.V.##d.X.Xaras#Mao#Lao#Lao#Masatas.b.#.basatas.b.#.basat.##d.X.X.X.V#M#M#M#M#M#M#M#M#M#M#M#L#L#G#A#x#s#o#k#eanan.V.##d.X.#",
".#.c.f.i.B.V.V.V.V.V.V.V.V.V.V.V.V.V.V.V.V.V.V.V.V.V.V.V.##d.X.Xauavas#M#M#M#M#Masavawavas#Masavaxavas#Masavay.##d.X.X.X.V.V.V.V.V.V.V.V.V.V.V.V.V.V.V.V.V.V.V.V.V.V.V.V.##d.X.#",
".#.c.f.i.C.W.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.W.i.X.X.iazaA.V.V.V.V.VaBaCaDaEat.VataFaDaFat.VatayaDaG.i.X.X.X.i.W.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.W.i.X.#",
".#.c.f.i.D.i#d#d#d#d#d#d#d#d#d#d#d#d#d#d#d#d#d#d#d#d#d#d.i.E.X.X.EaHaG.#.#.#.#.#.#.#aG.baG.#.#.#aG.baG.#.#.#aGaH.E.X.X.X.E.i#d#d#d#d#d#d#d#d#d#d#d#d#d#d#d#d#d#d#d#d#d#d.i.E.X.#",
".#.c.f.i.E.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.E.i#d#d#d#d#d#d#d.i.D.i#d#d#d.i.D.i#d#d#d.i.E.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.#",
".#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.E#X#1.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.D#Y#2.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.h#3#2.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.h#3#2.X.X.X.X#4#4#5#6#7#8#9a.a#aaabacadaeadaeafagahai.V.i.E.X.X.X.X.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.h#3#2.X.X.X.V.F.F.G.H.I.J.K.L.M.N.O.P.Q.P.Q.R.S.T.U.V.W.i.X.X.X.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.h#3#2.X.X.V.Y.Z.0.1.2.3.4.5.6.7.8.9#..9#.###a#b#c.V.##d.X.X.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.h#3#2.X.V#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M.V.##d.X.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.h#3#2.V#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.##T.h#Y#0#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.d#W.D.B#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c#T#U.B#M#M#M#M#M#M#Q#S#S#S#S#R#Q#M#M#M#M#M#M.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.B#M#M#M#M#M#M#M#P#O#N#N#N#M#M#M#M#M#M#M.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.B#M#M#M#M#M#M#M#M.V.V.V#M#M#M#M#M#M#M#M.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.B#M#M#M#M#M#M#M#M#M.V#M#M#M#M#M#M#M#M#M.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.B#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.B#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M#M.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.B#L.V#L.V#L.V#L.V#L.V#L.V#L.V#L.V#L.V#L.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.B#A#A#F#F#A#A#A#E#B#C#D#B#B#B#B#C#B#B#A.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.B#A#A#F#F#A#A#A#E#B#C#D#B#B#B#B#C#B#B#A.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.B#x#w#w#w#x#w#w#w#z#z#y#w#x#w#w#x#x#w#w.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.B#s#u#u#u#t#t#t#t#t#u#s#s#u#v#t#t#u#t#s.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.B#o#q#o#r#o#p#o#o#r#o#o#q#o#o#o#o#p#o#n.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.B#k#i#j#l#m#k#k#i#k#l#l#k#i#l#k#i#j#i#i.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.B#e#g#g#e#g#g#g#e#h#e#g#e#e#g#g#g#f#e#e.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.Bananananananananaqanananananaqanananan.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.Banananananananaqananananaqanananananan.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.B.V.V.V.V.V.V.V.V.V.V.V.V.V.V.V.V.V.V.V.V.##d.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.C.W.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.W.i.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.D.i#d#d#d#d#d#d#d#d#d#d#d#d#d#d#d#d#d#d#d.i.E.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.c.f.i.E.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.X.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt"};
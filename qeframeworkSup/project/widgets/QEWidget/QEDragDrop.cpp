/*  QEDragDrop.cpp
 *
 *  This file is part of the EPICS QT Framework, initially developed at the Australian Synchrotron.
 *
 *  The EPICS QT Framework is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  The EPICS QT Framework is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with the EPICS QT Framework.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Copyright (c) 2012,2014,2016,2017 Australian Synchrotron
 *
 *  Author:
 *    Andrew Rhyder
 *  Contact details:
 *    andrew.rhyder@synchrotron.org.au
 */

/*
  This class adds common drag/drop support to all QE widgets if required.

  This class manages the common drag and drop functions leaving the actual QE widgets class
  to supply and accept the data transfered (typically a PV name).

  All QE widgets (eg, QELabel, QELineEdit) have an instance of this class as they based on
  QEWidget which itself uses this class as a base class.

  To implement any sort of drag/drop a QE widgets (like any other widget) must implement the following
  virtual functions of its base QWidget:
        dragEnterEvent()
        dropEvent()
        mousePressEvent()

  To make use of the common QE drag drop support provided by this class, the above functions can be
  defined to simply call the equivelent drag/drop functions defined in this class as follows:
        void dragEnterEvent(QDragEnterEvent *event) { qcaDragEnterEvent( event [,allowSelfDrop] ); }
        void dropEvent(QDropEvent *event)           { qcaDropEvent( event [,allText] ); }
        void mousePressEvent(QMouseEvent *event)    { qcaMousePressEvent( event ); }

  The parameter allowSelfDrop controls if a widget i allowd to drop onto iself, defaults to false).
  The parameter allText controls if all or only first part of text is dropped, defaults to false).

  To allow this class to obtain text for dragging, or deliver text dropped, the QE widgets also needs to
  implement the following functions defined in this class:
        setDrop()
        getDrop()

  Typically, the text dragged and dropped is the underlying PV

 */

#include <QEDragDrop.h>
#include <QDebug>
#include <QWidget>
#include <QEWidget.h>
#include <QMimeData>
#include <QDrag>
#include <QClipboard>
#include <QApplication>
#include <QEPlatform.h>
#include <QGraphicsOpacityEffect>
#include <QLinearGradient>

#define DEBUG qDebug () << "QEDragDrop" << __LINE__ << __FUNCTION__ << "  "


//==============================================================================
// place holder
QEDragDropObject::QEDragDropObject( QObject* parent ) :
    QObject( parent )
{}

// place holder
QEDragDropObject::~QEDragDropObject() { }

void QEDragDropObject::sendRequestAction( const QEActionRequests& request )
{
    emit requestAction( request );
}


//==============================================================================
// Construction.
QEDragDrop::QEDragDrop( QEWidget* qewIn, QWidget* ownerIn )
{
    // Sanity check.
    if( ownerIn == NULL )
    {
        qWarning( "QEDragDrop constructor called with a null 'owner'" );
        exit( EXIT_FAILURE );
    }

    // Keep a handle on the underlying QWidget and QE widgets
    owner = ownerIn;
    qew = qewIn;

    // Create the signaller object - note: owned and deleted the widget
    object = new QEDragDropObject( ownerIn );
}

// Set the consumer of the signal generated by this object
// (send via the associated contextMenuObject object).
void QEDragDrop::setDragDropConsumer( QObject* consumer )
{
    if( consumer )
    {
        QObject::connect( object,  SIGNAL ( requestAction( const QEActionRequests& ) ),
                          consumer,  SLOT ( requestAction( const QEActionRequests& ) ) );
    }
}

// Start a 'drag'
void QEDragDrop::qcaDragEnterEvent(QDragEnterEvent *event, const bool allowSelfDrop)
{

    // Flag a move is starting (never a copy)
    if (event->mimeData()->hasText())
    {
        if ( event->source() == owner )
        {
            if( allowSelfDrop )
            {
                event->setDropAction( Qt::MoveAction );
                event->accept();
            } else {
                event->ignore();
            }
        } else {
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }
}

// Perform a 'drop'
void QEDragDrop::qcaDropEvent(QDropEvent *event, const bool allText)
{
    // If no text available, do nothing
    if( !event->mimeData()->hasText())
    {
        event->ignore();
        return;
    }

    // Get the drop data
    const QMimeData *mime = event->mimeData();

    // If there is any text, drop the text
    if( !mime->text().isEmpty() )
    {
        if( allText )
        {
            // Carry out the drop action. Drop all the availble text.
            setDrop( mime->text() );
        }
        else
        {
            // Get the component textual parts
            QStringList pieces = mime->text().split(QRegExp("\\s+"),
                                                    QString::SkipEmptyParts);

            // Carry out the drop action
            // Assume only the first text part is of interest
            setDrop( pieces[0] );
        }
    }

    // There is no text. If there is any image data, drop the image
    else if( !mime->imageData().isNull() )
    {
        QVariant image = mime->imageData();
        setDrop( image );
    }

    // Tell the dropee that the drop has been acted on
    if (event->source() == owner )
    {
        event->setDropAction(Qt::CopyAction);
        event->accept();
    }
    else
    {
        event->acceptProposedAction();
    }
}


// Prepare to drag and/or intercept other mouse press events.
//
void QEDragDrop::qcaMousePressEvent(QMouseEvent* event)
{
    if( !event ) return;

    const QEvent::Type type = event->type();
    const Qt::MouseButton button =  event->button();

    switch( type ){

    case QEvent::MouseButtonPress:
        // Use only left button presses
        if( button == Qt::LeftButton )
        {
            initiateDragDrop( event );
        }

        // Not drag drop per se, but here is where we handle button events.
        //
        else if( button == MIDDLE_BUTTON )
        {
            postPvInformation();
        }

        // Ignore other than left/middle button presses
        // Right button picked up by context menu
        //
        else
        {
            event->ignore();
        }
        break;

    case  QEvent::MouseButtonDblClick:
        if( button ==  Qt::LeftButton )
        {
            examinePVProperties();
        }

        else if( button == MIDDLE_BUTTON )
        {
            plotInStripChart();
        }

        // Ignore other than left/middle button presses
        //
        else {
            event->ignore();
        }
        break;

    default:
        DEBUG << "Unexpected mouse press type:" << type << event;
        event->ignore();
        break;
    }
}

// Prepare to drag.
//
void QEDragDrop::initiateDragDrop (QMouseEvent* event)
{
    // Make the hotspot match the point clicked over
    QPoint hotSpot = event->pos();

    // Set up the transfer data
    QMimeData *mimeData = new QMimeData;
    QVariant dropData = getDrop();
    switch( dropData.type() )
    {
        default:
        case QVariant::String:
            mimeData->setText( dropData.toString() );
            break;

        case QVariant::Image:
            mimeData->setImageData( dropData );
            break;
    }

    mimeData->setData( "application/x-hotspot",
                       QByteArray::number( hotSpot.x() )
                       + " " + QByteArray::number( hotSpot.y()) );

    // Determine the size of the copy of the object that is dragged
    // It will be the full size unless it exceeds a maximum height or width, in which case it is scaled
    QSize pixSize = owner->size();
    double widthScale = (double)(pixSize.width())/100.0;
    double heightScale = (double)(pixSize.height())/50.0;
    double scale = std::max( widthScale, heightScale );
    if( scale > 1.0 )
    {
        pixSize.setWidth( int( pixSize.width() / scale ) );
        pixSize.setHeight( int( pixSize.height() / scale ) );
    }

    // Get a copy of the object
    QPixmap pixmap( pixSize );
    owner->render( &pixmap );

    // Set up the drag
    QDrag *drag = new QDrag( owner );
    drag->setMimeData( mimeData );
    drag->setPixmap( pixmap );
    drag->setHotSpot( hotSpot );

    // Carry out the drag operation
    drag->exec( Qt::CopyAction, Qt::CopyAction );
}

void QEDragDrop::postPvInformation()
{
    if( !qew ) return;

    // Extract pv name(s), copy to clip board and post as information.
    QString pvName = qew->copyVariable ();
    QClipboard* cb = QApplication::clipboard();
    cb->setText( pvName );

    message_types mt( MESSAGE_TYPE_INFO, MESSAGE_KIND_STATUS );
    qew->sendMessage( pvName, mt );
}

void QEDragDrop::examinePVProperties()
{
    if( !qew ) return;

    // Extract pv name(s), create action request
    QString pvName = qew->copyVariable ();
    QEActionRequests request( QEActionRequests::actionPvProperties(), pvName );
    if( !pvName.isEmpty() ) object->sendRequestAction( request );
}

void QEDragDrop::plotInStripChart()
{
    if( !qew ) return;

    // Extract pv name(s), create action request.
    QString pvName = qew->copyVariable ();
    QEActionRequests request( QEActionRequests::actionStripChart(), pvName );
    if( !pvName.isEmpty() ) object->sendRequestAction( request );
}

// allow drop (Enable/disable as a drop site for drag and drop)
void QEDragDrop::setAllowDrop( bool allowDropIn )
{
    allowDrop = allowDropIn;
    owner->setAcceptDrops( allowDrop );
}

bool QEDragDrop::getAllowDrop() const
{
    return allowDrop;
}

// end
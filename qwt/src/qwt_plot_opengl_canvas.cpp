/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt_plot_opengl_canvas.h"
#include "qwt_plot.h"
#include <qevent.h>
#include <qopenglframebufferobject.h>
#include <qopenglpaintdevice.h>

class QwtPlotOpenGLCanvas::PrivateData
{
public:
    PrivateData():
        fbo( NULL )
    {
    }

    ~PrivateData()
    {
        delete fbo;
    }

    int numSamples;
    QOpenGLFramebufferObject* fbo;
};


/*! 
  \brief Constructor

  \param plot Parent plot widget
  \sa QwtPlot::setCanvas()
*/
QwtPlotOpenGLCanvas::QwtPlotOpenGLCanvas( QwtPlot *plot ):
    QOpenGLWidget( plot ),
    QwtPlotAbstractGLCanvas( this )
{
    QSurfaceFormat fmt = format();
    fmt.setSamples( 4 );

    init( fmt );
}

QwtPlotOpenGLCanvas::QwtPlotOpenGLCanvas( const QSurfaceFormat &format, QwtPlot *plot ):
    QOpenGLWidget( plot ),
    QwtPlotAbstractGLCanvas( this )
{
    init( format );
}

void QwtPlotOpenGLCanvas::init( const QSurfaceFormat &format )
{
    d_data = new PrivateData;
    d_data->numSamples = format.samples();

    setFormat( format );

#if 1
    setAttribute( Qt::WA_OpaquePaintEvent, true );
#endif
}

//! Destructor
QwtPlotOpenGLCanvas::~QwtPlotOpenGLCanvas()
{
    delete d_data;
}

/*!
  Paint event

  \param event Paint event
  \sa QwtPlot::drawCanvas()
*/
void QwtPlotOpenGLCanvas::paintEvent( QPaintEvent *event )
{
    QOpenGLWidget::paintEvent( event );
}

/*!
  Qt event handler for QEvent::PolishRequest and QEvent::StyleChange
  \param event Qt Event
  \return See QGLWidget::event()
*/
bool QwtPlotOpenGLCanvas::event( QEvent *event )
{
    const bool ok = QOpenGLWidget::event( event );

    if ( event->type() == QEvent::PolishRequest ||
        event->type() == QEvent::StyleChange )
    {
        // assuming, that we always have a styled background
        // when we have a style sheet

        setAttribute( Qt::WA_StyledBackground,
            testAttribute( Qt::WA_StyleSheet ) );
    }

    return ok;
}

void QwtPlotOpenGLCanvas::replot()
{
    QwtPlotAbstractGLCanvas::replot();
}

void QwtPlotOpenGLCanvas::invalidateBackingStore()
{
    // happens for each replot -> better set a flag only
    delete d_data->fbo;
    d_data->fbo = NULL;
}

QPainterPath QwtPlotOpenGLCanvas::borderPath( const QRect &rect ) const
{
    return borderPath2( rect );
}

void QwtPlotOpenGLCanvas::initializeGL()
{
}

void QwtPlotOpenGLCanvas::paintGL()
{
    const bool hasFocusIndicator = 
        hasFocus() && focusIndicator() == CanvasFocusIndicator;

    if ( testPaintAttribute( QwtPlotOpenGLCanvas::BackingStore ) &&
        QOpenGLFramebufferObject::hasOpenGLFramebufferBlit() )
    {
        // does this mode make any sense - QOpenGLWidget has its own internal FBO

        if ( d_data->fbo == NULL || d_data->fbo->size() != size() )
        {
            invalidateBackingStore();

            QOpenGLFramebufferObjectFormat fboFormat;
            fboFormat.setSamples( d_data->numSamples );
            fboFormat.setAttachment( QOpenGLFramebufferObject::CombinedDepthStencil );

            d_data->fbo = new QOpenGLFramebufferObject( size(), fboFormat );

            QOpenGLPaintDevice pd( size() );

            QPainter fboPainter( &pd );
            draw( &fboPainter);
            fboPainter.end();
        }

        makeCurrent();

        QOpenGLFramebufferObject::blitFramebuffer( NULL, d_data->fbo );

        if ( hasFocusIndicator )
        {
            QPainter painter( this );
            drawFocusIndicator( &painter );
        }
    }
    else
    {
        QPainter painter( this );
        draw( &painter );

        if ( hasFocusIndicator )
            drawFocusIndicator( &painter );
    }
}

void QwtPlotOpenGLCanvas::resizeGL( int, int )
{
    invalidateBackingStore();
}

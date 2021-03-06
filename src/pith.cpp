#include "inc/pith.h"

#include "inc/coordinate.h"

#include <QPainter>

Pith::Pith(const int size) : QVector<uiCoord2D>(size)
{
}

Pith::Pith( const Pith &pith ) : QVector<uiCoord2D>(pith)
{
}

Pith::Pith( const QVector<uiCoord2D> &coordinates ) : QVector<uiCoord2D>(coordinates)
{
}

/*******************************
 * Public getters
 *******************************/


/*******************************
 * Public setters
 *******************************/
void Pith::draw( QImage &image, const int &sliceIdx ) const
{
	if ( sliceIdx < size() )
	{
		QPainter painter(&image);
		painter.setBrush(Qt::red);
		painter.setPen(Qt::red);
		painter.drawEllipse((*this)[sliceIdx].x-5,(*this)[sliceIdx].y-5,10,10);
	}
}

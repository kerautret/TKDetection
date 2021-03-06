#ifndef MARROW_H
#define MARROW_H

#include "def/def_coordinate.h"

#include <QVector>

class QImage;

class Pith : public QVector<uiCoord2D>
{
public:
	Pith( const int size = 0 );
	Pith( const Pith &pith );
	Pith( const QVector<uiCoord2D> &coordinates );

	void draw( QImage &image, const int &sliceIdx ) const;
};

#endif // MARROW_H

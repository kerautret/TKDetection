#include "inc/contour.h"

#include "inc/billon.h"
#include "inc/slicealgorithm.h"
#include "inc/define.h"

#include <QPainter>
#include <QLinkedList>

Contour::Contour() : QVector<iCoord2D>(0)
{
}

Contour::Contour( const QVector<iCoord2D> &vector ) : QVector<iCoord2D>(vector)
{
}

Contour::Contour( const Contour &contour ) : QVector<iCoord2D>(contour)
{
}

Contour::~Contour()
{
}

void Contour::compute( const Slice &slice, const iCoord2D &sliceCenter, const int &intensityThreshold, iCoord2D startPoint )
{
	this->clear();
	if ( startPoint == iCoord2D(-1,-1) )
	{
		startPoint = SliceAlgorithm::findNearestPointOfThePith( slice, sliceCenter, intensityThreshold );
	}
	if ( startPoint != iCoord2D(-1,-1) )
	{
		iCoord2D currentPos(startPoint);
		QVector<iCoord2D> mask(8);
		qreal startAngle;
		int interdit, j;

		// Using Moore-Neighbor Tracing
		startAngle = fmod(startPoint.angle(sliceCenter)+TWO_PI,TWO_PI);
		interdit = qRound(startAngle*4./PI);
		do
		{
			this->append(currentPos);
			mask[0].x = mask[1].x = mask[7].x = currentPos.x+1;
			mask[2].x = mask[6].x = currentPos.x;
			mask[3].x = mask[4].x = mask[5].x = currentPos.x-1;
			mask[1].y = mask[2].y = mask[3].y = currentPos.y+1;
			mask[0].y = mask[4].y = currentPos.y;
			mask[5].y = mask[6].y = mask[7].y = currentPos.y-1;
			j = (interdit+1)%8;
			while ( j < interdit+8 && slice.at(mask[j%8].y,mask[j%8].x) <= intensityThreshold ) ++j;
			currentPos = mask[j%8];
			interdit = (j+4)%8;
		}
		while ( currentPos != startPoint || this->size() < 10 );
	}
}

void Contour::smooth( int smoothingRadius )
{
	const int nbPoints = this->size();
	if ( smoothingRadius > 0 && nbPoints > qMax(smoothingRadius,2) )
	{
		smoothingRadius = qMin(smoothingRadius,nbPoints);
		const int smoothingDiameter = 2*smoothingRadius+1;
		const qreal qSmoothingDiameter = smoothingDiameter;
		if ( nbPoints > smoothingDiameter )
		{
			QVector<iCoord2D> initialContour;
			initialContour.reserve(nbPoints+2*smoothingRadius);
			initialContour << this->mid(nbPoints-smoothingRadius) <<  *this << this->mid(0,smoothingRadius);

			int i, smoothingValueX, smoothingValueY;
			smoothingValueX = smoothingValueY = 0;

			for ( i=0 ; i<smoothingDiameter ; ++i )
			{
				smoothingValueX += initialContour[i].x;
				smoothingValueY += initialContour[i].y;
			}

			this->clear();
			this->append(iCoord2D( qRound(smoothingValueX/qSmoothingDiameter), qRound(smoothingValueY/qSmoothingDiameter) ));

			iCoord2D currentCoord, oldCoord;
			bool isModified;

			for ( int i=0 ; i<nbPoints-1 ; ++i )
			{
				smoothingValueX = smoothingValueX - initialContour[i].x + initialContour[i+smoothingDiameter].x;
				smoothingValueY = smoothingValueY - initialContour[i].y + initialContour[i+smoothingDiameter].y;
				currentCoord.x = qRound(smoothingValueX / qSmoothingDiameter);
				currentCoord.y = qRound(smoothingValueY / qSmoothingDiameter);
				if ( currentCoord == this->last() ) continue;
				if ( this->size() > 1 )
				{
					oldCoord = (*this)[this->size()-2];
					isModified = true;
					while ( isModified )
					{
						if ( currentCoord == oldCoord )
						{
							this->pop_back();
							this->pop_back();
							if ( this->size() > 1 ) oldCoord = (*this)[this->size()-2];
							else isModified = false;
						}
						else
						{
							if ( qSqrt( qPow(currentCoord.x-oldCoord.x,2)+qPow(currentCoord.y-oldCoord.y,2) ) < 2 )
							{
								this->pop_back();
								if ( this->size() > 1 ) oldCoord = (*this)[this->size()-2];
								else isModified = false;
							}
							else isModified = false;
						}
					}
				}
				this->append(currentCoord);
			}
			isModified = true;
			while ( this->size() > 3 && isModified )
			{
				if ( (*this)[0] == (*this)[this->size()-2] ) this->pop_back();
				else if ((*this)[1] == (*this)[this->size()-1] ) this->pop_front();
				else if ((*this)[1] == (*this)[this->size()-2] )
				{
					this->pop_back();
					this->pop_front();
				}
				else isModified = false;
			}
		}
	}
}

Contour Contour::convexHull()
{
	Contour convexHull;
	QVector<iCoord2D>::ConstIterator iterPoints = this->constBegin();
	convexHull.append(*iterPoints++);
	convexHull.append(*iterPoints++);
	while ( iterPoints != this->end() )
	{
		const iCoord2D &point = *iterPoints++;
		while ( convexHull.size() > 1 &&  convexHull.at(convexHull.size()-2).vectorProduct(convexHull.last(),point) <= 0 )
		{
			convexHull.pop_back();
		}
		convexHull.push_back(point);
	}
	return convexHull;
}

void Contour::draw( QPainter &painter, const int &cursorPosition, const iCoord2D &sliceCenter, const TKD::ProjectionType &viewType ) const
{
	const int nbPoints = this->size();
	if ( nbPoints > 0 )
	{
		const qreal angularFactor = painter.window().width()/TWO_PI;
		int i, j, k, x, y;

		painter.save();
		painter.setPen(Qt::blue);
		if ( viewType == TKD::Z_PROJECTION )
		{
			for ( int i=0 ; i<nbPoints ; ++i )
			{
				painter.drawPoint((*this)[i].x,(*this)[i].y);
			}

		}
		else if ( viewType == TKD::CARTESIAN_PROJECTION )
		{
			for ( k=0 ; k<nbPoints ; ++k )
			{
				i = (*this)[k].x - sliceCenter.x;
				j = (*this)[k].y - sliceCenter.y;
				y = qSqrt(qPow(i,2) + qPow(j,2));
				x = 2. * qAtan( j / (qreal)(i + y) ) * angularFactor;
				painter.drawPoint(x,y);
			}
		}

		// Dessin du curseur
		if ( cursorPosition >= 0 )
		{
			painter.setPen(Qt::cyan);
			if ( viewType == TKD::Z_PROJECTION )
			{
				painter.drawEllipse((*this)[cursorPosition].x-1,(*this)[cursorPosition].y-1,2,2);
			}
			else if ( viewType == TKD::CARTESIAN_PROJECTION )
			{
				i = sliceCenter.x - (*this)[cursorPosition].x;
				j = sliceCenter.y - (*this)[cursorPosition].y;
				y = qSqrt(qPow(i,2) + qPow(j,2));
				x = 2. * qAtan( j / (i + y ) );
				painter.drawEllipse(x-1,y-1,2,2);
			}
		}
		painter.restore();
	}

}

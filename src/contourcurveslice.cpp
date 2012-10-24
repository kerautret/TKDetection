#include "inc/contourcurveslice.h"

#include "inc/billon.h"
#include "inc/billonalgorithms.h"

#include <QProcess>
#include <QTemporaryFile>
#include <QPainter>

ContourCurveSlice::ContourCurveSlice( const Slice *slice ) : _slice(slice)
{
}

ContourCurveSlice::ContourCurveSlice( const ContourCurveSlice &contourCurveSlice ) : _slice(contourCurveSlice._slice),
	_datasContourPoints(contourCurveSlice._datasContourPoints),	_datasOriginalContourPoints(contourCurveSlice._datasOriginalContourPoints), _datasDominantPoints(contourCurveSlice._datasDominantPoints),
	_datasMainDominantPoints(contourCurveSlice._datasMainDominantPoints), _datasIndexMainDominantPoints(contourCurveSlice._datasIndexMainDominantPoints), _datasMainSupportPoints(contourCurveSlice._datasMainSupportPoints),
	_contourPolygonBottom(contourCurveSlice._contourPolygonBottom), _contourPolygonTop(contourCurveSlice._contourPolygonTop)
{
}

ContourCurveSlice::~ContourCurveSlice()
{
}

/**********************************
 * Public getters
 **********************************/

const QVector<iCoord2D> &ContourCurveSlice::contourPoints() const
{
	return _datasContourPoints;
}

const QVector<iCoord2D> &ContourCurveSlice::dominantPoints() const
{
	return _datasDominantPoints;
}

const QVector<iCoord2D> &ContourCurveSlice::mainDominantPoints() const
{
	return _datasMainDominantPoints;
}

int ContourCurveSlice::indexOfMainPoint( const uint &number ) const
{
	return number<2 ? _datasIndexMainDominantPoints[number] : -1;
}

const QVector<iCoord2D> &ContourCurveSlice::mainSupportPoints() const
{
	return _datasMainSupportPoints;
}

/**********************************
 * Public setters
 **********************************/

void ContourCurveSlice::compute( Slice &resultSlice, const iCoord2D &sliceCenter, const int &intensityThreshold, const int &blurredSegmentThickness, const int &smoothingRadius, const iCoord2D &startPoint )
{
	clear();
//	if ( _slice != 0 )
//	{
		extractContourPointsAndDominantPoints( sliceCenter, intensityThreshold, blurredSegmentThickness, smoothingRadius, startPoint );

		if ( _datasDominantPoints.size() > 3 )
		{
			computeMainDominantPoints(sliceCenter);
		}

		computeContourPolygons();
		updateSlice( resultSlice, sliceCenter, intensityThreshold );
//	}
}

void ContourCurveSlice::computeOldMethod( Slice &resultSlice, const iCoord2D &sliceCenter, const int &intensityThreshold, const int &smoothingRadius, const iCoord2D &startPoint )
{
	clear();
	if ( _slice != 0 )
	{
		_datasOriginalContourPoints = BillonAlgorithms::extractContour( *_slice, sliceCenter, intensityThreshold, startPoint );
		_datasContourPoints = _datasOriginalContourPoints;
		smoothCurve( smoothingRadius );

		const int width = _slice->n_cols;
		const int height = _slice->n_rows;
		resultSlice.resize(height,width);
		resultSlice.fill(0);

		const int nbOriginalPointsContour = _datasOriginalContourPoints.size();

		int i,j, minXIndex, maxXIndex, minYIndex, maxYIndex;
		QPolygon contourPolygonBottom;

		// Sinon on ajoute la composante en entier
		minXIndex = maxXIndex = _datasOriginalContourPoints[0].x;
		minYIndex = maxYIndex = _datasOriginalContourPoints[0].y;
		for ( i=0 ; i<nbOriginalPointsContour ; ++i )
		{
			contourPolygonBottom << QPoint(_datasOriginalContourPoints[i].x,_datasOriginalContourPoints[i].y);
			minXIndex = qMin(minXIndex,_datasOriginalContourPoints[i].x);
			maxXIndex = qMax(maxXIndex,_datasOriginalContourPoints[i].x);
			minYIndex = qMin(minYIndex,_datasOriginalContourPoints[i].y);
			maxYIndex = qMax(maxYIndex,_datasOriginalContourPoints[i].y);
		}
		contourPolygonBottom << QPoint(_datasOriginalContourPoints[0].x,_datasOriginalContourPoints[0].y);

		for ( j = minYIndex ; j<maxYIndex ; ++j )
		{
			for ( i = minXIndex ; i<maxXIndex ; ++i )
			{
				if ( _slice->at(j,i) > intensityThreshold && contourPolygonBottom.containsPoint(QPoint(i,j),Qt::OddEvenFill) )
				{
					resultSlice.at(j,i) = 1;
				}
			}
		}
	}
}

void ContourCurveSlice::draw( QImage &image ) const
{
	if ( _slice != 0 )
	{
		QPainter painter(&image);
		int i;

		const int nbContourPoints = _datasContourPoints.size();
		if ( nbContourPoints > 0 )
		{
			// Dessin des points de contours
			painter.setPen(Qt::blue);
			for ( i=0 ; i<nbContourPoints ; ++i )
			{
				painter.drawPoint(_datasContourPoints[i].x,_datasContourPoints[i].y);
			}

			const int nbDominantPoints = _datasDominantPoints.size();
			if ( nbDominantPoints > 0 )
			{
				// Dessin des points dominants
				painter.setPen(Qt::green);
				for ( i=0 ; i<nbDominantPoints ; ++i )
				{
					painter.drawEllipse(_datasDominantPoints[i].x-2,_datasDominantPoints[i].y-2,4,4);
				}


				const iCoord2D &mainPoint1 = _datasMainDominantPoints[0];
				const iCoord2D &mainPoint2 = _datasMainDominantPoints[1];

				// Dessin des points dominants principaux
				painter.setPen(Qt::red);
				if ( _datasIndexMainDominantPoints[0] != -1 ) painter.drawEllipse(mainPoint1.x-3,mainPoint1.y-3,6,6);
				if ( _datasIndexMainDominantPoints[1] != -1 ) painter.drawEllipse(mainPoint2.x-3,mainPoint2.y-3,6,6);

				// Dessins des droites de coupe issues des points de support dominants principaux
				painter.setPen(Qt::gray);
				const iCoord2D &supportMain1 = _datasMainSupportPoints[0];
				qreal a, b;
				if ( supportMain1.x != -1 || supportMain1.y != -1 )
				{
					a = ( mainPoint1.y - supportMain1.y ) / static_cast<qreal>( mainPoint1.x - supportMain1.x );
					b = ( mainPoint1.y * supportMain1.x - mainPoint1.x * supportMain1.y ) / static_cast<qreal>( supportMain1.x - mainPoint1.x );
					//				painter.drawLine(0, b, image.width(), a * image.width() + b );
					if ( supportMain1.x < mainPoint1.x )
					{
						painter.drawLine(mainPoint1.x, mainPoint1.y, image.width(), a * image.width() + b );
					}
					else
					{
						painter.drawLine(mainPoint1.x, mainPoint1.y, 0., b );
					}
				}
				const iCoord2D &supportMain2 = _datasMainSupportPoints[1];
				if ( supportMain2.x != -1 || supportMain2.y != -1 )
				{
					a = ( mainPoint2.y - supportMain2.y ) / static_cast<qreal>( mainPoint2.x - supportMain2.x );
					b = ( mainPoint2.y * supportMain2.x - mainPoint2.x * supportMain2.y ) / static_cast<qreal>( supportMain2.x - mainPoint2.x );
					//				painter.drawLine(0, b, image.width(), a * image.width() + b );
					if ( supportMain2.x < mainPoint2.x )
					{
						painter.drawLine(mainPoint2.x, mainPoint2.y, image.width(), a * image.width() + b );
					}
					else
					{
						painter.drawLine(mainPoint2.x, mainPoint2.y, 0., b );
					}
				}
			}

			// Dessin du point de contour initial (également point dominant initial)
			painter.setPen(Qt::red);
			painter.drawEllipse(_datasContourPoints[0].x-1,_datasContourPoints[0].y-1,2,2);
		}
	}
}

/**********************************
 * Private setters
 **********************************/

void ContourCurveSlice::clear()
{
	_datasOriginalContourPoints.clear();
	_datasContourPoints.clear();
	_datasDominantPoints.clear();
	_datasMainDominantPoints.clear();
	_datasMainDominantPoints.fill(iCoord2D(-1,-1),2);
	_datasIndexMainDominantPoints.clear();
	_datasIndexMainDominantPoints.fill(-1,2);
	_datasMainSupportPoints.fill(iCoord2D(-1,-1),2);

	_contourPolygonBottom.clear();
	_contourPolygonTop.clear();
}

void ContourCurveSlice::smoothCurve( int smoothingRadius )
{
	if ( smoothingRadius > 0 )
	{
		const int nbPoints = _datasContourPoints.size();
		smoothingRadius = qMin(smoothingRadius,nbPoints);
		const int smoothingDiameter = 2*smoothingRadius+1;
		if ( nbPoints > smoothingDiameter )
		{
			QVector<iCoord2D> datasContourForSmoothing;
			datasContourForSmoothing.reserve(nbPoints+2*smoothingRadius);
			datasContourForSmoothing << _datasContourPoints.mid(nbPoints-smoothingRadius) <<  _datasContourPoints << _datasContourPoints.mid(0,smoothingRadius);

			int i, smoothingValueX, smoothingValueY;
			smoothingValueX = smoothingValueY = 0;

			for ( i=0 ; i<smoothingDiameter ; ++i )
			{
				smoothingValueX += datasContourForSmoothing[i].x;
				smoothingValueY += datasContourForSmoothing[i].y;
			}
			_datasContourPoints[0].x = smoothingValueX / static_cast<qreal>(smoothingDiameter);
			_datasContourPoints[0].y = smoothingValueY / static_cast<qreal>(smoothingDiameter);
			for ( int i=1 ; i<nbPoints ; ++i )
			{
				smoothingValueX = smoothingValueX - datasContourForSmoothing[i-1].x + datasContourForSmoothing[i+smoothingDiameter-1].x;
				smoothingValueY = smoothingValueY - datasContourForSmoothing[i-1].y + datasContourForSmoothing[i+smoothingDiameter-1].y;
				_datasContourPoints[i].x = smoothingValueX / static_cast<qreal>(smoothingDiameter);
				_datasContourPoints[i].y = smoothingValueY / static_cast<qreal>(smoothingDiameter);
			}
		}
	}
}

void ContourCurveSlice::extractContourPointsAndDominantPoints( const iCoord2D &sliceCenter, const int &intensityThreshold, const int &blurredSegmentThickness, const int &smoothingRadius, const iCoord2D &startPoint )
{
	_datasOriginalContourPoints = BillonAlgorithms::extractContour( *_slice, sliceCenter, intensityThreshold, startPoint );
	_datasContourPoints = _datasOriginalContourPoints;
	smoothCurve( smoothingRadius );

	int nbPoints = _datasContourPoints.size();

	// Ecriture des données de points de contours
	QTemporaryFile fileContours("TKDetection_XXXXXX.ctr");
	if ( !fileContours.open() )
	{
		qDebug() << QObject::tr("ERREUR : Impossible de créer le ficher de contours %1.").arg(fileContours.fileName());
		return;
	}

	QTextStream streamContours(&fileContours);
	for ( int i=0 ; i<nbPoints ; ++i )
	{
		streamContours << _datasContourPoints.at(i).x << " " << _datasContourPoints.at(i).y << endl;
	}
	fileContours.close();

	// Extraction des points dominants à partir des points de contour
	QTemporaryFile fileDominantPoint("TKDetection_XXXXXX.dp");
	if( !fileDominantPoint.open() )
	{
		qDebug() << QObject::tr("ERREUR : Impossible de créer le ficher de points dominants %1.").arg(fileDominantPoint.fileName());
		return;
	}
	fileDominantPoint.close();

	QProcess dominantPointExtraction;
	dominantPointExtraction.setStandardInputFile(fileContours.fileName());
	dominantPointExtraction.start(QString("cornerdetection -epais %1 -pointFile %2").arg(blurredSegmentThickness).arg(fileDominantPoint.fileName()));

	if ( dominantPointExtraction.waitForFinished(3000) )
	{
		if( !fileDominantPoint.open() )
		{
			qDebug() << QObject::tr("ERREUR : Impossible de lire le ficher de points dominants %1.").arg(fileDominantPoint.fileName());
			return;
		}
		QTextStream streamDominantPoints(&fileDominantPoint);
		nbPoints = streamDominantPoints.readLine().toInt();
		int x, y;
		for ( int i=0 ; i<nbPoints ; ++i )
		{
			streamDominantPoints >> x >> y;
			_datasDominantPoints << iCoord2D(x,y);
		}

		fileDominantPoint.close();
	}
}

void ContourCurveSlice::computeMainDominantPoints( const iCoord2D &sliceCenter )
{
	// On détermine les points dominants principaux en comparant la distance à la moelle d'un point dominant
	// à la distance à la moelle des deux points dominants précdents et du point dominant suivant.
	int nbDominantPoints, index, oldIndex;
	qreal distanceP0Pm2, distanceP0Pm1, distanceP0Pp1, distanceP0Pp2, distanceP0Px;
	bool angleOk;

	nbDominantPoints = _datasDominantPoints.size();
	if ( nbDominantPoints > 0 ) _datasDominantPoints << _datasDominantPoints[0];
	nbDominantPoints++;

	// Point dominant dans le sens du contour
	index = 1;
	distanceP0Pm1 = sliceCenter.euclideanDistance(_datasDominantPoints[index-1]);
	distanceP0Px = sliceCenter.euclideanDistance(_datasDominantPoints[index]);
	distanceP0Pp1 = sliceCenter.euclideanDistance(_datasDominantPoints[index+1]);
	distanceP0Pp2 = sliceCenter.euclideanDistance(_datasDominantPoints[index+2]);
	angleOk = (distanceP0Pp1-distanceP0Px>10) || (distanceP0Pm1<distanceP0Pp1 && (distanceP0Px<distanceP0Pp1 || distanceP0Px<distanceP0Pp2) );
	if ( angleOk ) ++index;
	while ( angleOk && index < nbDominantPoints-1 )
	{
		distanceP0Pm2 = distanceP0Pm1;
		distanceP0Pm1 = distanceP0Px;
		distanceP0Px = distanceP0Pp1;
		distanceP0Pp1 = sliceCenter.euclideanDistance(_datasDominantPoints[index+1]);
		angleOk = (distanceP0Pp1-distanceP0Px>10) || (distanceP0Pm2<distanceP0Pm1 && distanceP0Pm1<distanceP0Pp1 && distanceP0Px<distanceP0Pp1);
		if ( angleOk ) ++index;
	}
	oldIndex = index;
	// Si le point dominant trouvé est correct
	if ( index < nbDominantPoints-1 )
	{
		_datasMainDominantPoints[0] = _datasDominantPoints[index];
		_datasIndexMainDominantPoints[0] = index;

		// Calcul du point dominant support du point dominant principal
		_datasMainSupportPoints[0].x = _datasMainSupportPoints[0].y = 0;
		while ( index >= 0 )
		{
			_datasMainSupportPoints[0].x += _datasDominantPoints[index].x;
			_datasMainSupportPoints[0].y += _datasDominantPoints[index].y;
			index--;
		}
		_datasMainSupportPoints[0].x /= (_datasIndexMainDominantPoints[0] + 1);
		_datasMainSupportPoints[0].y /= (_datasIndexMainDominantPoints[0] + 1);
	}

	// Point dominant dans le sens contraire du contour
	index = nbDominantPoints-2;
	distanceP0Pm1 = sliceCenter.euclideanDistance(_datasDominantPoints[index+1]);
	distanceP0Px = sliceCenter.euclideanDistance(_datasDominantPoints[index]);
	distanceP0Pp1 = sliceCenter.euclideanDistance(_datasDominantPoints[index-1]);
	distanceP0Pp2 = sliceCenter.euclideanDistance(_datasDominantPoints[index-2]);
	angleOk = (distanceP0Pp1-distanceP0Px>10) || (distanceP0Pm1<distanceP0Pp1 && (distanceP0Px<distanceP0Pp1 || distanceP0Px<distanceP0Pp2) );
	if ( angleOk ) index--;
	while ( angleOk && index > oldIndex )
	{
		distanceP0Pm2 = distanceP0Pm1;
		distanceP0Pm1 = distanceP0Px;
		distanceP0Px = distanceP0Pp1;
		distanceP0Pp1 = sliceCenter.euclideanDistance(_datasDominantPoints[index-1]);
		angleOk = (distanceP0Pp1-distanceP0Px>10) || (distanceP0Pm2<distanceP0Pm1 && distanceP0Pm1<distanceP0Pp1 && distanceP0Px<distanceP0Pp1);
		if ( angleOk ) index--;
	}
	// Si le point dominant trouvé est correct
	if ( index > oldIndex )
	{
		_datasMainDominantPoints[1] = _datasDominantPoints[index];
		_datasIndexMainDominantPoints[1] = index;

		// Calcul du point dominant support du point dominant principal
		_datasMainSupportPoints[1].x = _datasMainSupportPoints[1].y = 0;
		while ( index != 1 )
		{
			_datasMainSupportPoints[1].x += _datasDominantPoints[index].x;
			_datasMainSupportPoints[1].y += _datasDominantPoints[index].y;
			index = (index+1)%nbDominantPoints;
		}
		_datasMainSupportPoints[1].x /= (nbDominantPoints - _datasIndexMainDominantPoints[1] + 1);
		_datasMainSupportPoints[1].y /= (nbDominantPoints - _datasIndexMainDominantPoints[1] + 1);
	}

	_datasDominantPoints.remove(nbDominantPoints-1);
}

void ContourCurveSlice::computeContourPolygons()
{
	const int width = _slice->n_cols;
	const int height = _slice->n_rows;
	const int nbOriginalPointsContour = _datasOriginalContourPoints.size();

	const iCoord2D &mainPoint1 = _datasMainDominantPoints[0];
	const iCoord2D &mainPoint2 = _datasMainDominantPoints[1];

	const bool hasMain1 = (mainPoint1.x != -1 || mainPoint1.y != -1);
	const bool hasMain2 = (mainPoint2.x != -1 || mainPoint2.y != -1);

	int i, index, originalMain1Index, originalMain2Index, minXIndex, maxXIndex, minYIndex, maxYIndex;
	qreal currentDistanceMain1, currentDistanceMain2;

	// S'il y a deux points dominants principaux
	if ( hasMain1 && hasMain2 )
	{
		// Calcul des point du contour original les plus proches des points dominants principaux
		// et de la boite englobante du contour original
		currentDistanceMain1 = currentDistanceMain2 = width+height;
		originalMain1Index = originalMain2Index = 0;
		minXIndex = maxXIndex = _datasOriginalContourPoints[0].x;
		minYIndex = maxYIndex = _datasOriginalContourPoints[0].y;
		for ( index=0 ; index<nbOriginalPointsContour ; ++index )
		{
			const iCoord2D &currentContourPoint = _datasOriginalContourPoints[index];
			if ( _datasMainDominantPoints[0].euclideanDistance(currentContourPoint) < currentDistanceMain1 )
			{
				currentDistanceMain1 = _datasMainDominantPoints[0].euclideanDistance(currentContourPoint);
				originalMain1Index = index;
			}
			if ( _datasMainDominantPoints[1].euclideanDistance(currentContourPoint) <= currentDistanceMain2 )
			{
				currentDistanceMain2 = _datasMainDominantPoints[1].euclideanDistance(currentContourPoint);
				originalMain2Index = index;
			}
			minXIndex = qMin(minXIndex,currentContourPoint.x);
			maxXIndex = qMax(maxXIndex,currentContourPoint.x);
			minYIndex = qMin(minYIndex,currentContourPoint.y);
			maxYIndex = qMax(maxYIndex,currentContourPoint.y);
		}

		// Création du polygone qui servira à tester l'appartenance d'un pixel en dessous de la droite des deux points dominants principaux au noeud.
		// Ce polygone est constitué :
		//     - des points compris entre le premier point du contour initial (non lissé) et le plus proche du premier point dominant
		//     - du point le plus proche du premier point dominant
		//     - du point le plus proche du second point dominant
		//     - des points compris entre le point du contour initial le plus proche du second point dominant et le dernier points du contour initial.
		for ( i=0 ; i<=originalMain1Index ; ++i ) _contourPolygonBottom << QPoint(_datasOriginalContourPoints[i].x,_datasOriginalContourPoints[i].y);
		_contourPolygonBottom << QPoint(mainPoint1.x,mainPoint1.y)
							 << QPoint(mainPoint2.x,mainPoint2.y);
		for ( i=originalMain2Index ; i<nbOriginalPointsContour ; ++i ) _contourPolygonBottom << QPoint(_datasOriginalContourPoints[i].x,_datasOriginalContourPoints[i].y);
		_contourPolygonBottom << QPoint(_datasOriginalContourPoints[0].x,_datasOriginalContourPoints[0].y);

		// Création du polygone qui servira à tester l'appartenance d'un pixel au dessus de la droite des deux points dominants principaux au noeud.
		// Ce polygone est constitué :
		//     - du point le plus proche du premier point dominant
		//     - des points du contour initial (non lissé) situés entre le point le plus proche du premier point dominant et le point le plus proche du second point dominant
		//     - du point le plus proche du second point dominant
		_contourPolygonTop << QPoint(mainPoint1.x,mainPoint1.y);
		for ( i=originalMain1Index ; i<=originalMain2Index ; ++i ) _contourPolygonTop << QPoint(_datasOriginalContourPoints[i].x,_datasOriginalContourPoints[i].y);
		_contourPolygonTop << QPoint(mainPoint2.x,mainPoint2.y)
						  << QPoint(mainPoint1.x,mainPoint1.y);
	}
	else
	{
		// Création du polygone qui servira à tester l'appartenance d'un pixel au noeud.
		// Ce polygone est constitué des pixels du contour initial (non lissé)
		minXIndex = maxXIndex = _datasOriginalContourPoints[0].x;
		minYIndex = maxYIndex = _datasOriginalContourPoints[0].y;
		for ( i=0 ; i<nbOriginalPointsContour ; ++i )
		{
			_contourPolygonBottom << QPoint(_datasOriginalContourPoints[i].x,_datasOriginalContourPoints[i].y);
			minXIndex = qMin(minXIndex,_datasOriginalContourPoints[i].x);
			maxXIndex = qMax(maxXIndex,_datasOriginalContourPoints[i].x);
			minYIndex = qMin(minYIndex,_datasOriginalContourPoints[i].y);
			maxYIndex = qMax(maxYIndex,_datasOriginalContourPoints[i].y);
		}
		_contourPolygonBottom << QPoint(_datasOriginalContourPoints[0].x,_datasOriginalContourPoints[0].y);
	}
}

void ContourCurveSlice::updateSlice( Slice &resultSlice, const iCoord2D &sliceCenter, const int &intensityThreshold )
{
	const int width = _slice->n_cols;
	const int height = _slice->n_rows;
	const int nbOriginalPointsContour = _datasOriginalContourPoints.size();

	resultSlice = *_slice;
	resultSlice.fill(0);

	const iCoord2D &mainPoint1 = _datasMainDominantPoints[0];
	const iCoord2D &mainPoint2 = _datasMainDominantPoints[1];
	const iCoord2D &support1 = _datasMainSupportPoints[0];
	const iCoord2D &support2 = _datasMainSupportPoints[1];

	const qreal daMain1Main2 = mainPoint1.y - mainPoint2.y;
	const qreal dbMain1Main2 = mainPoint2.x - mainPoint1.x;
	const qreal dcMain1Main2 = daMain1Main2*mainPoint1.x + dbMain1Main2*mainPoint1.y;
	const bool supToMain1Main2 = ( daMain1Main2*sliceCenter.x + dbMain1Main2*sliceCenter.y ) > dcMain1Main2;

	const qreal daMain1Support1 = mainPoint1.y - support1.y;
	const qreal dbMain1Support1 = support1.x - mainPoint1.x;
	const qreal dcMain1Support1 = daMain1Support1*mainPoint1.x + dbMain1Support1*mainPoint1.y;
	const bool supToMain1Support1 = ( daMain1Support1*mainPoint2.x + dbMain1Support1*mainPoint2.y ) > dcMain1Support1;

	const qreal daMain2Support2 = mainPoint2.y - support2.y;
	const qreal dbMain2Support2 = support2.x - mainPoint2.x;
	const qreal dcMain2Support2 = daMain2Support2*mainPoint2.x + dbMain2Support2*mainPoint2.y;
	const bool supToMain2Support2 = ( daMain2Support2*mainPoint1.x + dbMain2Support2*mainPoint1.y ) > dcMain2Support2;

	const bool hasMain1 = (mainPoint1.x != -1 || mainPoint1.y != -1);
	const bool hasMain2 = (mainPoint2.x != -1 || mainPoint2.y != -1);

	int i,j;

	// S'il y a deux points dominants principaux
	if ( hasMain1 && hasMain2 )
	{
		// Ajout des pixel
		for ( j=0 ; j<height ; ++j )
		{
			for ( i=0 ; i<width ; ++i )
			{
				if ( _slice->at(j,i) > intensityThreshold &&
					 ( _contourPolygonBottom.containsPoint(QPoint(i,j),Qt::OddEvenFill) ||
					   (
						   ((daMain1Main2*i+dbMain1Main2*j <= dcMain1Main2) == supToMain1Main2) &&
						   ((daMain1Support1*i+dbMain1Support1*j >= dcMain1Support1) == supToMain1Support1) &&
						   ((daMain2Support2*i+dbMain2Support2*j >= dcMain2Support2) == supToMain2Support2) &&
						   _contourPolygonTop.containsPoint(QPoint(i,j),Qt::OddEvenFill)
						   )
					   )
					 )
				{
					resultSlice.at(j,i) = 1;
				}
			}
		}
	}
	else
	{
		if ( hasMain1 )
		{
			// Ajout des pixels du noeud du bon côté de chaque la droite de prolongement
			// et à l'intérieur du contour original
			const iCoord2D nextMain1( mainPoint1.x - daMain1Support1, mainPoint1.y - dbMain1Support1 );
			const bool rightToMain1Support1 = ( daMain1Support1*nextMain1.x + dbMain1Support1*nextMain1.y ) > dcMain1Support1;
			for ( j=0 ; j<height ; ++j )
			{
				for ( i=0 ; i<width ; ++i )
				{
					if ( _slice->at(j,i) > intensityThreshold
						 && ((daMain1Support1*i+dbMain1Support1*j > dcMain1Support1) == rightToMain1Support1)
						 && _contourPolygonBottom.containsPoint(QPoint(i,j),Qt::OddEvenFill) )
					{
						resultSlice.at(j,i) = 1;
					}
				}
			}
		}
		else if ( hasMain2 )
		{
			// Ajout des pixels du noeud du bon côté de chaque la droite de prolongement
			// et à l'intérieur du contour original
			const iCoord2D nextMain2( mainPoint2.x + daMain2Support2, mainPoint2.y + dbMain2Support2 );
			const bool leftToMain2Support2 = ( daMain2Support2*nextMain2.x + dbMain2Support2*nextMain2.y ) > dcMain2Support2;
			for ( j=0 ; j<height ; ++j )
			{
				for ( i=0 ; i<width ; ++i )
				{
					if ( _slice->at(j,i) > intensityThreshold
						 && ((daMain2Support2*i+dbMain2Support2*j > dcMain2Support2) == leftToMain2Support2)
						 && _contourPolygonBottom.containsPoint(QPoint(i,j),Qt::OddEvenFill) )
					{
						resultSlice.at(j,i) = 1;
					}
				}
			}
		}
		else if ( nbOriginalPointsContour > 3 )
		{
			// Sinon on ajoute la composante en entier
			for ( j=0 ; j<height ; ++j )
			{
				for ( i=0 ; i<width ; ++i )
				{
					if ( _slice->at(j,i) > intensityThreshold && _contourPolygonBottom.containsPoint(QPoint(i,j),Qt::OddEvenFill) )
					{
						resultSlice.at(j,i) = 1;
					}
				}
			}
		}
	}
}

#include "inc/billonalgorithms.h"

#include "inc/billon.h"
#include "inc/piechart.h"

namespace BillonAlgorithms
{
	qreal restrictedAreaMeansRadius(const Billon &billon, const uint &nbDirections, const int &intensityThreshold, const uint &minimumRadius, const uint &nbSlicesToIgnore )
	{
		Q_ASSERT_X( nbDirections>0 , "BillonTpl<T>::getRestrictedAreaMeansRadius", "nbPolygonPoints arguments equals to 0 => division by zero" );

		qreal radius = billon.n_cols/2.;
		if ( billon.hasPith() )
		{
			const int &width = billon.n_cols;
			const int &height = billon.n_rows;
			const int &depth = billon.n_slices-nbSlicesToIgnore;
			const qreal angleIncrement = TWO_PI/static_cast<qreal>(nbDirections);

			rCoord2D center, edge;
			rVec2D direction;
			qreal orientation, currentNorm;

			radius = width;
			for ( int k=nbSlicesToIgnore ; k<depth ; ++k )
			{
				const Slice &currentSlice = billon.slice(k);
				center.x = billon.pithCoord(k).x;
				center.y = billon.pithCoord(k).y;
				orientation = angleIncrement;
				while (orientation < TWO_PI)
				{
					orientation += angleIncrement;
					direction = rVec2D(qCos(orientation),qSin(orientation));
					edge = center + direction*minimumRadius;
					while ( edge.x>0 && edge.y>0 && edge.x<width && edge.y<height && currentSlice.at(edge.y,edge.x) > intensityThreshold )
					{
						edge += direction;
					}
					currentNorm = rVec2D(edge-center).norm();
					if ( currentNorm < radius ) radius = currentNorm;
				}
			}
		}
		qDebug() << "Rayon de la boite englobante (en pixels) : " << radius;
		return radius;
	}

	QVector<rCoord2D> restrictedAreaVertex( const Billon &billon, const Interval<uint> & sliceInterval, const uint & nbPolygonVertex, const int & intensityThreshold )
	{
		Q_ASSERT_X( nbPolygonVertex>0 , "BillonTpl<T>::getRestrictedAreaVertex", "nbPolygonVertex arguments equals to 0 => division by zero" );

		QVector<rCoord2D> vectAllVertex;
		if ( billon.hasPith() )
		{
			const int width = billon.n_cols;
			const int height = billon.n_rows;
			const qreal angleIncrement = TWO_PI/static_cast<qreal>(nbPolygonVertex);
			rCoord2D edge, center;
			rVec2D direction;
			qreal orientation;
			for ( uint indexSlice = sliceInterval.min() ; indexSlice<=sliceInterval.max() ; ++indexSlice )
			{
				const Slice & currentSlice = billon.slice(indexSlice);
				center.x = billon.pithCoord(indexSlice).x;
				center.y = billon.pithCoord(indexSlice).y;
				orientation = 0.;
				while (orientation < TWO_PI)
				{
					orientation += angleIncrement;
					direction = rVec2D(qCos(orientation),qSin(orientation));
					edge = center + direction*30;
					while ( edge.x>0. && edge.y>0. && edge.x<width && edge.y<height && currentSlice.at(edge.y,edge.x) >= intensityThreshold )
					{
						edge += direction;
					}
					vectAllVertex.push_back(edge);
				}
			}
		}
		return vectAllVertex;
	}

	Billon * tangentialTransform( const Billon &billon, const Interval<uint> &sliceInterval, const Interval<uint> &angularInterval, const int &minIntensity )
	{
		const qreal zPithCoord = sliceInterval.mid();
		const rCoord2D &originPith = billon.pithCoord(zPithCoord);
		const uint angularRange = (angularInterval.max() + (angularInterval.isValid() ? 0. : PieChartSingleton::getInstance()->nbSectors())) - angularInterval.min();

		const qreal bisectorOrientation = (angularInterval.min()+angularRange/2.)*PieChartSingleton::getInstance()->angleStep();
		const qreal cosBisector = qCos(bisectorOrientation);
		const qreal sinBisector = qSin(bisectorOrientation);

		// Dimensions des coupes tangentielles
		/* Recherche de l'écorce dans la direction de la bissectrice du secteur angulaire sur la coupe au milieu de l'intervalle de coupes */
		const Slice &midSlice = billon.slice(zPithCoord);
		const rVec2D direction(cosBisector,sinBisector);
		rCoord2D edge = originPith + direction*30;
		while ( edge.x>0 && edge.y>0 && edge.x<billon.n_cols && edge.y<billon.n_rows && midSlice.at(edge.y,edge.x) > minIntensity )
		{
			edge += direction;
		}
		const qreal depth = rVec2D(edge-originPith).norm();
		/* Hauteur et largeur des coueps tangentielles */
		const uint width = sliceInterval.size()+1;
		const uint height = 2 * qTan(angularRange*PieChartSingleton::getInstance()->angleStep()/2.) * depth;
		const int widthOnTWo = width/2.;
		const int heightOnTwo = height/2.;
		const int heightOnTwoMinusOne = height/2.-1;

		const uint nbSlices = depth;

		// Inversion width et height pour correspondre à la ratation de 90°
		Billon * tangentialBillon = new Billon(height,width,nbSlices);
		tangentialBillon->setVoxelSize( billon.voxelWidth(), billon.voxelDepth(), billon.voxelWidth() );
		tangentialBillon->fill(minIntensity);

		// Rotation autour de l'axe Y
		const qreal alpha = PI_ON_TWO;
		const qreal cosAlpha = qCos(alpha);
		const qreal sinAlpha = qSin(alpha);
		arma::Mat<qreal>::fixed<3,3> rotationMatY;
                rotationMatY << cosAlpha <<  0 << -sinAlpha << arma::endr
                             << 0  << 1  << 0 << arma::endr
                             << sinAlpha  << 0<< cosAlpha << arma::endr;

		// Rotation selon l'angle de la zone de nœuds
		 arma::Mat<qreal>::fixed<3,3> rotationMatX;
                rotationMatX <<  1 <<   0 <<   0 << arma::endr
                             << 0 << cosBisector <<   -sinBisector<< arma::endr
                             << 0 << sinBisector <<  cosBisector << arma::endr;

		const arma::Mat<qreal>::fixed<3,3> rotationMat = rotationMatY*rotationMatX;

		// Vecteur de déplacement entre deux coupes tangentielles successives
		arma::Col<qreal>::fixed<3> shiftStep;
                shiftStep <<  0. <<   0. <<  depth/(1.*nbSlices) << arma::endr;
		const arma::Col<qreal>::fixed<3> originShift = rotationMat * shiftStep;

		arma::Col<qreal>::fixed<3> origin;
                origin << originPith.x <<   originPith.y <<   zPithCoord << arma::endr;
		arma::Col<qreal>::fixed<3> initial, destination;
		initial(2) = 0.;

		const qreal semiKnotAreaHeightCoeff = heightOnTwo / static_cast<qreal>( nbSlices );
		int i, j, jStart, jEnd;
		jStart = jEnd = 0;
		int x0,y0,z0;
		qreal x0Dist, y0Dist, z0Dist;
		qreal xFrontTop, xFrontBottom, xBackTop, xBackBottom, yFront, yBack;
		for ( uint k=0 ; k<nbSlices ; ++k )
		{
			Slice &slice = tangentialBillon->slice(k);
			jStart = -qMin(k*semiKnotAreaHeightCoeff,heightOnTwo*1.);
			jEnd = qMin(k*semiKnotAreaHeightCoeff,heightOnTwo*1.);
			for ( j=jStart ; j<jEnd ; ++j )
			{
				initial.at(1) = j;
				for ( i=-widthOnTWo ; i<widthOnTWo ; ++i )
				{
					initial.at(0) = i;
					destination = (rotationMat * initial) + origin;
					x0 = qFloor(destination.at(1));
					y0 = qFloor(destination.at(0));
					z0 = qFloor(destination.at(2));
					x0Dist = destination.at(1)-x0;
					y0Dist = destination.at(0)-y0;
					z0Dist = destination.at(2)-z0;
					xFrontTop = (1.-x0Dist)*billon.at(x0-1,y0+1,z0-1) + x0Dist*billon.at(x0+1,y0+1,z0-1);
					xFrontBottom = (1.-x0Dist)*billon.at(x0-1,y0-1,z0-1) + x0Dist*billon.at(x0+1,y0-1,z0-1);
					xBackTop = (1.-x0Dist)*billon.at(x0-1,y0+1,z0+1) + x0Dist*billon.at(x0+1,y0+1,z0+1);
					xBackBottom = (1.-x0Dist)*billon.at(x0-1,y0-1,z0+1) + x0Dist*billon.at(x0+1,y0-1,z0+1);
					yFront = (1.-y0Dist)*xFrontBottom + y0Dist*xFrontTop;
					yBack = (1.-y0Dist)*xBackBottom + y0Dist*xBackTop;
					// Rotation de 90° dans le sens horaire pour correspondre à l'orientation de l'article
					slice(i+widthOnTWo,heightOnTwoMinusOne-j) = (billon.at(x0,y0,z0)+2.*((1.-z0Dist)*yFront + z0Dist*yBack))/3.;
							//destination.at(0)>=0 && destination.at(0)<billon.n_cols && destination.at(1)>=0 &&
							//destination.at(1)<billon.n_rows && destination.at(2)>=0 && destination.at(2)<billon.n_slices ?
							//	(1.-z0Dist)*yFront + z0Dist*yBack : minIntensity;
								//(billon.at(x0,y0,z0) + billon.at(x0+(x0Dist>0.5?1:-1),y0,z0) + billon.at(x0,y0+(y0Dist>0.5?1:-1),z0) + billon.at(x0,y0,z0+(z0Dist>0.5?1:-1)))/4.:minIntensity;
								//billon.at(qRound(destination.at(1)),qRound(destination.at(0)),qRound(destination.at(2))) : minIntensity;
				}
			}
			origin += originShift;
		}

		return tangentialBillon;
	}

}

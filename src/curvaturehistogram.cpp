#include "inc/curvaturehistogram.h"

#include "inc/contour.h"

#include <QTemporaryFile>
#include <QProcess>
#include <QDebug>

CurvatureHistogram::CurvatureHistogram() : Histogram<qreal>()
{
}

CurvatureHistogram::~CurvatureHistogram()
{
}

/**********************************
 * Public setters
 **********************************/

void CurvatureHistogram::construct( const Contour &contour, const int &curvatureWidth )
{
	clear();

	const int nbPoints = contour.size();
	int i, streamTrash;

	if ( nbPoints > 0 )
	{
		QTemporaryFile fileContours("TKDetection_XXXXXX.ctr");
		if ( !fileContours.open() )
		{
			qDebug() << QObject::tr("ERREUR : Impossible de créer le ficher de contours %1.").arg(fileContours.fileName());
			return;
		}

		QTextStream streamContours(&fileContours);
		for ( i=0 ; i<nbPoints ; ++i )
		{
			streamContours << contour[i].x << " " << contour[i].y << endl;
		}
		fileContours.close();

		QTemporaryFile fileCurvature("TKDetection_XXXXXX.crv");
		if( !fileCurvature.open() )
		{
			qDebug() << QObject::tr("ERREUR : Impossible de créer le ficher de courbure %1.").arg(fileCurvature.fileName());
			return;
		}
		fileCurvature.close();

		QProcess curvatureExtraction;
		curvatureExtraction.setStandardInputFile(fileContours.fileName());
		curvatureExtraction.setStandardOutputFile(fileCurvature.fileName());
        curvatureExtraction.start(QString("/usr/local/bin/curvature_gmcb -setWidth %1").arg(curvatureWidth));

		if ( curvatureExtraction.waitForFinished(3000) )
		{
			if( !fileCurvature.open() )
			{
				qDebug() << QObject::tr("ERREUR : Impossible de lire le ficher de courbure %1.").arg(fileCurvature.fileName());
				return;
			}

			QTextStream streamCurvature(&fileCurvature);
			streamCurvature.readLine();

			resize(nbPoints);
			for ( i=0 ; i<nbPoints ; ++i )
			{
				streamCurvature >> streamTrash >> streamTrash >> (*this)[i];
			}

			fileCurvature.close();
        }
    }else{
        qDebug() << QObject::tr("contour empty");
    }
}

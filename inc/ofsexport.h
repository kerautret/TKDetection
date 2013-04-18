#ifndef OFSEXPORT_H
#define OFSEXPORT_H

#include <QtGlobal>
#include "def/def_billon.h"

template <typename T> class QVector;
template <typename T, typename U> struct QPair;
template <typename T> class Interval;

class QString;
class Pith;
class PiePart;

namespace OfsExport
{
	void process(const Billon &billon, const Interval<uint> &sliceInterval, const QString &fileName, const int &nbEdgesPerSlice, const int &radiusOfTubes = 4, const bool &normalized = true );
	void processOnSector( const Billon &billon, const Interval<uint> &interval, const QString &fileName, const qreal &rightAngle, const qreal &leftAngle, const int &nbEdgesPerSlice, const bool &normalized = true );
	void processOnAllSectorInAllIntervals( const Billon &billon, const QVector< QPair< Interval<uint>, QPair<qreal,qreal> > > &intervals, const QString &fileName, const int &nbEdgesPerSlice = 4,  const bool &normalized = true );
	void processOnRestrictedMesh( const Billon &billon, const Interval<uint> &sliceInterval, const QString &fileName, const uint &resolutionCercle, const int &seuilContour, const bool &normalized,  const bool &displayBegEndFaces = true );
}


#endif // OFSEXPORT_H

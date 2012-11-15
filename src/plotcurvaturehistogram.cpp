#include "inc/plotcurvaturehistogram.h"

#include "inc/curvaturehistogram.h"

PlotCurvatureHistogram::PlotCurvatureHistogram()
{
	_histogramCursor.setBrush(Qt::red);
	_histogramCursor.setPen(QPen(Qt::red));

	_histogramDominantPoints.setBrush(Qt::green);
	_histogramDominantPoints.setPen(QPen(Qt::green));
}

PlotCurvatureHistogram::~PlotCurvatureHistogram()
{
}

void PlotCurvatureHistogram::attach( QwtPlot * const plot )
{
	if ( plot != 0 )
	{
		_histogramData.attach(plot);
		_histogramDominantPoints.attach(plot);
		_histogramCursor.attach(plot);
	}
}

void PlotCurvatureHistogram::clear()
{
	const QVector<QwtIntervalSample> emptyData(0);
	_histogramData.setSamples(emptyData);
	_histogramDominantPoints.setSamples(emptyData);
	_histogramCursor.setSamples(emptyData);
}

void PlotCurvatureHistogram::moveCursor( const uint &sliceIndex )
{
	QVector<QwtIntervalSample> datasCursor(1);
	datasCursor[0].interval.setInterval(sliceIndex,sliceIndex+1);
	datasCursor[0].value = _histogramData.sample(sliceIndex).value;
	_histogramCursor.setSamples(datasCursor);
}

void PlotCurvatureHistogram::update( const CurvatureHistogram & histogram, const QVector<int> dominantPointsIndex )
{
	updateDatas( histogram );
	updateDominantPoints( histogram, dominantPointsIndex );
}

void PlotCurvatureHistogram::updateDatas( const CurvatureHistogram &histogram )
{
	QVector<QwtIntervalSample> datasHistogram(0);
	if ( histogram.size() > 0 )
	{
		datasHistogram.reserve(histogram.size());
		int i=0;
		QVector<qreal>::ConstIterator begin = histogram.begin();
		const QVector<qreal>::ConstIterator end = histogram.end();
		while ( begin != end )
		{
			datasHistogram.append(QwtIntervalSample(*begin++,i,i+1));
			++i;
		}
	}
	_histogramData.setSamples(datasHistogram);
}


void PlotCurvatureHistogram::updateDominantPoints( const CurvatureHistogram &histogram, const QVector<int> dominantPointsIndex )
{
	QVector<QwtIntervalSample> dominantPointHistogram(0);
	if ( histogram.size() > 0 && dominantPointsIndex.size() > 0 )
	{
		dominantPointHistogram.reserve(dominantPointsIndex.size());
		QVector<int>::ConstIterator begin = dominantPointsIndex.begin();
		const QVector<int>::ConstIterator end = dominantPointsIndex.end();
		while ( begin != end )
		{
			dominantPointHistogram.append(QwtIntervalSample(histogram[*begin],*begin,*begin+1));
			++begin;
		}
	}
	_histogramDominantPoints.setSamples(dominantPointHistogram);
}

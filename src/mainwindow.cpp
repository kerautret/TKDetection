#include "inc/mainwindow.h"

#include "ui_mainwindow.h"

#include "inc/billon.h"
#include "inc/billonalgorithms.h"
#include "inc/define.h"
#include "inc/dicomreader.h"
#include "inc/knotpithprofile.h"
#include "inc/pith.h"
#include "inc/pithextractorboukadida.h"
#include "inc/ofsexport.h"
#include "inc/pgm3dexport.h"
#include "inc/piechart.h"
#include "inc/piepart.h"
#include "inc/plotknotpithprofile.h"
#include "inc/plotsectorhistogram.h"
#include "inc/plotslicehistogram.h"
#include "inc/sectorhistogram.h"
#include "inc/slicealgorithm.h"
#include "inc/slicehistogram.h"
#include "inc/sliceview.h"
#include "inc/v3dexport.h"
#include "inc/v3dreader.h"
#include "inc/tiffreader.h"

#include <QLabel>
#include <QFileDialog>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QMessageBox>
#include <QInputDialog>
#include <QXmlStreamWriter>

#include <qwt_plot_renderer.h>
#include <qwt_polar_renderer.h>
#include <qwt_polar_grid.h>

MainWindow::MainWindow( QWidget *parent ) : QMainWindow(parent), _ui(new Ui::MainWindow), _labelSliceView(new QLabel), _labelTangentialView(new QLabel),
	_billon(0), _tangentialBillon(0), _sliceView(new SliceView()),
	_sliceHistogram(new SliceHistogram()), _plotSliceHistogram(new PlotSliceHistogram()),
	_sectorHistogram(new SectorHistogram()), _plotSectorHistogram(new PlotSectorHistogram()),
	_knotPithProfile(new KnotPithProfile()), _plotKnotPithProfile(new PlotKnotPithProfile()),
	_currentSlice(0), _currentYSlice(0), _currentMaximum(0), _currentSector(0), _treeRadius(0)
{
	_ui->setupUi(this);
	setWindowTitle("TKDetection");

	// Paramétrisation des composant graphiques
	_labelSliceView->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	_labelSliceView->setScaledContents(true);
	_labelSliceView->installEventFilter(&_sliceZoomer);
	_labelSliceView->installEventFilter(this);

	_ui->_scrollSliceView->setBackgroundRole(QPalette::Dark);
	_ui->_scrollSliceView->setWidget(_labelSliceView);

	_labelTangentialView->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	_labelTangentialView->setScaledContents(true);
	_labelTangentialView->installEventFilter(&_tangentialZoomer);

	_ui->_scrollTangentialView->setBackgroundRole(QPalette::Dark);
	_ui->_scrollTangentialView->setWidget(_labelTangentialView);

	_ui->_comboViewType->insertItem(TKD::CLASSIC,tr("Originale"));
	_ui->_comboViewType->insertItem(TKD::Z_MOTION,tr("Z-mouvements"));
	_ui->_comboViewType->setCurrentIndex(TKD::CLASSIC);

	// Histogrammes
	_plotSliceHistogram->attach(_ui->_plotSliceHistogram);

	_ui->_plotSectorHistogram->setAxisScale(QwtPlot::xBottom,0,TWO_PI);
	_ui->_polarSectorHistogram->setScale( QwtPolar::Azimuth, TWO_PI, 0.0 );
	_plotSectorHistogram->attach(_ui->_polarSectorHistogram);
	_plotSectorHistogram->attach(_ui->_plotSectorHistogram);

	_plotKnotPithProfile->attach(_ui->_plotKnotPithProfile);

	QwtPolarGrid *grid = new QwtPolarGrid();
	grid->showAxis(QwtPolar::AxisBottom,false);
	grid->setMajorGridPen(QPen(Qt::lightGray));
	grid->attach(_ui->_polarSectorHistogram);

	/**** Mise en place de la communication MVC ****/

	initComponentsValues();

	/**********************************
	 * Évènements sur le widget central
	 **********************************/
	QObject::connect(_ui->_sliderSelectSlice, SIGNAL(valueChanged(int)), this, SLOT(setSlice(int)));
	QObject::connect(_ui->_sliderSelectYSlice, SIGNAL(valueChanged(int)), this, SLOT(setYSlice(int)));
	QObject::connect(_ui->_buttonZoomInitial, SIGNAL(clicked()), &_sliceZoomer, SLOT(resetZoom()));

	/*********************************************
	* Évènements de l'onglet "Paramètres généraux"
	 *********************************************/
	QObject::connect(_ui->_spanSliderIntensityInterval, SIGNAL(lowerValueChanged(int)), _ui->_spinMinIntensity, SLOT(setValue(int)));
	QObject::connect(_ui->_spinMinIntensity, SIGNAL(valueChanged(int)), _ui->_spanSliderIntensityInterval, SLOT(setLowerValue(int)));
	QObject::connect(_ui->_spanSliderIntensityInterval, SIGNAL(upperValueChanged(int)), _ui->_spinMaxIntensity , SLOT(setValue(int)));
	QObject::connect(_ui->_spinMaxIntensity, SIGNAL(valueChanged(int)), _ui->_spanSliderIntensityInterval, SLOT(setUpperValue(int)));
	QObject::connect(_ui->_spinMinIntensity, SIGNAL(valueChanged(int)), this, SLOT(drawSlice()));
	QObject::connect(_ui->_spinMinIntensity, SIGNAL(valueChanged(int)), this, SLOT(drawTangentialView()));
	QObject::connect(_ui->_spinMaxIntensity, SIGNAL(valueChanged(int)), this, SLOT(drawSlice()));
	QObject::connect(_ui->_spinMaxIntensity, SIGNAL(valueChanged(int)), this, SLOT(drawTangentialView()));
	// Onglet "Affichage"
	QObject::connect(_ui->_comboViewType, SIGNAL(currentIndexChanged(int)), this, SLOT(drawSlice()));
	QObject::connect(_ui->_radioYProjection, SIGNAL(clicked()), this, SLOT(drawSlice()));
	QObject::connect(_ui->_radioZProjection, SIGNAL(clicked()), this, SLOT(drawSlice()));
	QObject::connect(_ui->_radioZProjection, SIGNAL(clicked()), this, SLOT(drawTangentialView()));
	QObject::connect(_ui->_radioPolarProjection, SIGNAL(clicked()), this, SLOT(drawSlice()));
	QObject::connect(_ui->_radioPolarProjection, SIGNAL(clicked()), this, SLOT(drawTangentialView()));
	QObject::connect(_ui->_radioEllipticProjection, SIGNAL(clicked()), this, SLOT(drawSlice()));
	QObject::connect(_ui->_radioEllipticProjection, SIGNAL(clicked()), this, SLOT(drawTangentialView()));
	QObject::connect(_ui->_sliderCartesianAngularResolution, SIGNAL(valueChanged(int)), _ui->_spinCartesianAngularResolution, SLOT(setValue(int)));
	QObject::connect(_ui->_spinCartesianAngularResolution, SIGNAL(valueChanged(int)), _ui->_sliderCartesianAngularResolution, SLOT(setValue(int)));
	QObject::connect(_ui->_spinCartesianAngularResolution, SIGNAL(valueChanged(int)), this, SLOT(drawSlice()));
	QObject::connect(_ui->_spinCartesianAngularResolution, SIGNAL(valueChanged(int)), this, SLOT(drawTangentialView()));
	QObject::connect(_ui->_comboViewRender, SIGNAL(currentIndexChanged(int)), this, SLOT(drawSlice()));
	QObject::connect(_ui->_comboViewRender, SIGNAL(currentIndexChanged(int)), this, SLOT(drawTangentialView()));
	// Onglet "Paramètres de la zone restreinte"
	QObject::connect(_ui->_sliderRestrictedAreaResolution, SIGNAL(valueChanged(int)), _ui->_spinRestrictedAreaResolution, SLOT(setValue(int)));
	QObject::connect(_ui->_spinRestrictedAreaResolution, SIGNAL(valueChanged(int)), _ui->_sliderRestrictedAreaResolution, SLOT(setValue(int)));
	QObject::connect(_ui->_sliderRestrictedAreaThreshold, SIGNAL(valueChanged(int)), _ui->_spinRestrictedAreaThreshold, SLOT(setValue(int)));
	QObject::connect(_ui->_spinRestrictedAreaThreshold, SIGNAL(valueChanged(int)), _ui->_sliderRestrictedAreaThreshold, SLOT(setValue(int)));
	QObject::connect(_ui->_sliderRestrictedAreaPercentage, SIGNAL(valueChanged(int)), _ui->_spinRestrictedAreaPercentage, SLOT(setValue(int)));
	QObject::connect(_ui->_spinRestrictedAreaPercentage, SIGNAL(valueChanged(int)), _ui->_sliderRestrictedAreaPercentage, SLOT(setValue(int)));
	QObject::connect(_ui->_spinRestrictedAreaPercentage, SIGNAL(valueChanged(int)), this, SLOT(drawSlice()));
	QObject::connect(_ui->_sliderRestrictedAreaMinimumRadius, SIGNAL(valueChanged(int)), _ui->_spinRestrictedAreaMinimumRadius, SLOT(setValue(int)));
	QObject::connect(_ui->_spinRestrictedAreaMinimumRadius, SIGNAL(valueChanged(int)), _ui->_sliderRestrictedAreaMinimumRadius, SLOT(setValue(int)));
	QObject::connect(_ui->_checkRadiusAroundPith, SIGNAL(clicked()), this, SLOT(drawSlice()));

	/**************************************
	* Évènements de l'onglet "Histogrammes"
	***************************************/
	QObject::connect(_ui->_sliderZMotionMin, SIGNAL(valueChanged(int)), _ui->_spinZMotionMin, SLOT(setValue(int)));
	QObject::connect(_ui->_spinZMotionMin, SIGNAL(valueChanged(int)), _ui->_sliderZMotionMin, SLOT(setValue(int)));
	QObject::connect(_ui->_spinZMotionMin, SIGNAL(valueChanged(int)), this, SLOT(drawSlice()));
	// Onglet "Z-Mouvements par coupes"
	QObject::connect(_ui->_sliderHistogramSmoothingRadius_zMotion, SIGNAL(valueChanged(int)), _ui->_spinHistogramSmoothingRadius_zMotion, SLOT(setValue(int)));
	QObject::connect(_ui->_spinHistogramSmoothingRadius_zMotion, SIGNAL(valueChanged(int)), _ui->_sliderHistogramSmoothingRadius_zMotion, SLOT(setValue(int)));
	QObject::connect(_ui->_sliderHistogramMinimumHeightOfMaximum_zMotion, SIGNAL(valueChanged(int)), _ui->_spinHistogramMinimumHeightOfMaximum_zMotion, SLOT(setValue(int)));
	QObject::connect(_ui->_spinHistogramMinimumHeightOfMaximum_zMotion, SIGNAL(valueChanged(int)), _ui->_sliderHistogramMinimumHeightOfMaximum_zMotion, SLOT(setValue(int)));
	QObject::connect(_ui->_sliderHistogramMinimumWidthOfInterval_zMotion, SIGNAL(valueChanged(int)), _ui->_spinHistogramMinimumWidthOfInterval_zMotion, SLOT(setValue(int)));
	QObject::connect(_ui->_spinHistogramMinimumWidthOfInterval_zMotion, SIGNAL(valueChanged(int)), _ui->_sliderHistogramMinimumWidthOfInterval_zMotion, SLOT(setValue(int)));
	QObject::connect(_ui->_sliderHistogramDerivativeSearchPercentage_zMotion, SIGNAL(valueChanged(int)), _ui->_spinHistogramDerivativeSearchPercentage_zMotion, SLOT(setValue(int)));
	QObject::connect(_ui->_spinHistogramDerivativeSearchPercentage_zMotion, SIGNAL(valueChanged(int)), _ui->_sliderHistogramDerivativeSearchPercentage_zMotion, SLOT(setValue(int)));
	QObject::connect(_ui->_sliderHistogramPercentageOfSlicesToIgnore_zMotion, SIGNAL(valueChanged(int)), _ui->_spinHistogramPercentageOfSlicesToIgnore_zMotion, SLOT(setValue(int)));
	QObject::connect(_ui->_spinHistogramPercentageOfSlicesToIgnore_zMotion, SIGNAL(valueChanged(int)), _ui->_sliderHistogramPercentageOfSlicesToIgnore_zMotion, SLOT(setValue(int)));
	QObject::connect(_ui->_buttonHistogramResetDefaultValuesZMotion, SIGNAL(clicked()), this, SLOT(resetHistogramDefaultValuesZMotion()));
	// Onglet "Z-Mouvement par secteur angulaire"
	QObject::connect(_ui->_sliderHistogramNumberOfAngularSectors_zMotionAngular, SIGNAL(valueChanged(int)), _ui->_spinHistogramNumberOfAngularSectors_zMotionAngular, SLOT(setValue(int)));
	QObject::connect(_ui->_spinHistogramNumberOfAngularSectors_zMotionAngular, SIGNAL(valueChanged(int)), _ui->_sliderHistogramNumberOfAngularSectors_zMotionAngular, SLOT(setValue(int)));
	QObject::connect(_ui->_spinHistogramNumberOfAngularSectors_zMotionAngular, SIGNAL(valueChanged(int)), this, SLOT(setSectorNumber(int)));
	QObject::connect(_ui->_sliderHistogramSmoothingRadius_zMotionAngular, SIGNAL(valueChanged(int)), _ui->_spinHistogramSmoothingRadius_zMotionAngular, SLOT(setValue(int)));
	QObject::connect(_ui->_spinHistogramSmoothingRadius_zMotionAngular, SIGNAL(valueChanged(int)), _ui->_sliderHistogramSmoothingRadius_zMotionAngular, SLOT(setValue(int)));
	QObject::connect(_ui->_sliderHistogramMinimumHeightOfMaximum_zMotionAngular, SIGNAL(valueChanged(int)), _ui->_spinHistogramMinimumHeightOfMaximum_zMotionAngular, SLOT(setValue(int)));
	QObject::connect(_ui->_spinHistogramMinimumHeightOfMaximum_zMotionAngular, SIGNAL(valueChanged(int)), _ui->_sliderHistogramMinimumHeightOfMaximum_zMotionAngular, SLOT(setValue(int)));
	QObject::connect(_ui->_sliderHistogramMinimumWidthOfInterval_zMotionAngular, SIGNAL(valueChanged(int)), _ui->_spinHistogramMinimumWidthOfInterval_zMotionAngular, SLOT(setValue(int)));
	QObject::connect(_ui->_spinHistogramMinimumWidthOfInterval_zMotionAngular, SIGNAL(valueChanged(int)), _ui->_sliderHistogramMinimumWidthOfInterval_zMotionAngular, SLOT(setValue(int)));
	QObject::connect(_ui->_sliderHistogramDerivativeSearchPercentage_zMotionAngular, SIGNAL(valueChanged(int)), _ui->_spinHistogramDerivativeSearchPercentage_zMotionAngular, SLOT(setValue(int)));
	QObject::connect(_ui->_spinHistogramDerivativeSearchPercentage_zMotionAngular, SIGNAL(valueChanged(int)), _ui->_sliderHistogramDerivativeSearchPercentage_zMotionAngular, SLOT(setValue(int)));
	QObject::connect(_ui->_sliderHistogramIntervalGap_zMotionAngular, SIGNAL(valueChanged(int)), _ui->_spinHistogramIntervalGap_zMotionAngular, SLOT(setValue(int)));
	QObject::connect(_ui->_spinHistogramIntervalGap_zMotionAngular, SIGNAL(valueChanged(int)), _ui->_sliderHistogramIntervalGap_zMotionAngular, SLOT(setValue(int)));
	QObject::connect(_ui->_buttonHistogramResetDefaultValuesZMotionAngular, SIGNAL(clicked()), this, SLOT(resetHistogramDefaultValuesZMotionAngular()));

	/**************************************
	* Évènements de l'onglet "Segmentation"
	***************************************/
	// Onglet "Composantes connexes"
	QObject::connect(_ui->_sliderSectorThresholding, SIGNAL(valueChanged(int)), _ui->_spinSectorThresholding, SLOT(setValue(int)));
	QObject::connect(_ui->_spinSectorThresholding, SIGNAL(valueChanged(int)), _ui->_sliderSectorThresholding, SLOT(setValue(int)));
	QObject::connect(_ui->_sliderMinimalSizeOf3DConnexComponents, SIGNAL(valueChanged(int)), _ui->_spinMinimalSizeOf3DConnexComponents, SLOT(setValue(int)));
	QObject::connect(_ui->_spinMinimalSizeOf3DConnexComponents, SIGNAL(valueChanged(int)), _ui->_sliderMinimalSizeOf3DConnexComponents, SLOT(setValue(int)));
	QObject::connect(_ui->_sliderMinimalSizeOf2DConnexComponents, SIGNAL(valueChanged(int)), _ui->_spinMinimalSizeOf2DConnexComponents, SLOT(setValue(int)));
	QObject::connect(_ui->_spinMinimalSizeOf2DConnexComponents, SIGNAL(valueChanged(int)), _ui->_sliderMinimalSizeOf2DConnexComponents, SLOT(setValue(int)));

	/***********************************
	* Évènements de l'onglet "Processus"
	************************************/
	QObject::connect(_ui->_buttonComputePith, SIGNAL(clicked()), this, SLOT(updateBillonPith()));
	QObject::connect(_ui->_comboSelectSliceInterval, SIGNAL(currentIndexChanged(int)), this, SLOT(selectSliceInterval(int)));
	QObject::connect(_ui->_buttonSelectSliceIntervalUpdate, SIGNAL(clicked()), this, SLOT(selectCurrentSliceInterval()));
	QObject::connect(_ui->_comboSelectSectorInterval, SIGNAL(currentIndexChanged(int)), this, SLOT(selectSectorInterval(int)));
	QObject::connect(_ui->_buttonSelectSectorIntervalUpdate, SIGNAL(clicked()), this, SLOT(selectCurrentSectorInterval()));

	/********************************
	* Évènements de l'onglet "Export"
	*********************************/
	// Onglet "Exporter les histogrammes"
	QObject::connect(_ui->_buttonExportHistograms, SIGNAL(clicked()), this, SLOT(exportHistograms()));
	// Onglet "Exporter en OFS"
	QObject::connect(_ui->_sliderExportNbEdges, SIGNAL(valueChanged(int)), _ui->_spinExportNbEdges, SLOT(setValue(int)));
	QObject::connect(_ui->_spinExportNbEdges, SIGNAL(valueChanged(int)), _ui->_sliderExportNbEdges, SLOT(setValue(int)));
	QObject::connect(_ui->_sliderExportRadius, SIGNAL(valueChanged(int)), _ui->_spinExportRadius, SLOT(setValue(int)));
	QObject::connect(_ui->_spinExportRadius, SIGNAL(valueChanged(int)), _ui->_sliderExportRadius, SLOT(setValue(int)));
	QObject::connect(_ui->_buttonExportToOfs, SIGNAL(clicked()), this, SLOT(exportToOfs()));
	// Onglet "Exporter en PGM3D"
	QObject::connect(_ui->_sliderPgm3dExportContrast, SIGNAL(valueChanged(int)), _ui->_spinPgm3dExportContrast, SLOT(setValue(int)));
	QObject::connect(_ui->_spinPgm3dExportContrast, SIGNAL(valueChanged(int)), _ui->_sliderPgm3dExportContrast, SLOT(setValue(int)));
	QObject::connect(_ui->_sliderPgm3dExportResolution, SIGNAL(valueChanged(int)), _ui->_spinPgm3dExportResolution, SLOT(setValue(int)));
	QObject::connect(_ui->_spinPgm3dExportResolution, SIGNAL(valueChanged(int)), _ui->_sliderPgm3dExportResolution, SLOT(setValue(int)));
	QObject::connect(_ui->_buttonExportToPgm3d, SIGNAL(clicked()), this, SLOT(exportToPgm3D()));
	// Onglet "Exporter en V3D"
	QObject::connect(_ui->_buttonExportToV3D, SIGNAL(clicked()), this, SLOT(exportToV3D()));
	// Onglet "Exporter en SDP"
	QObject::connect(_ui->_buttonExportToSDP, SIGNAL(clicked()), this, SLOT(exportToSdp()));

	/*************************************
	* Évènements du panneau "Histogrammes"
	**************************************/
	// Onglet "1. Coupes"
	QObject::connect(_ui->_buttonPreviousMaximum, SIGNAL(clicked()), this, SLOT(previousMaximumInSliceHistogram()));
	QObject::connect(_ui->_buttonNextMaximum, SIGNAL(clicked()), this, SLOT(nextMaximumInSliceHistogram()));
	QObject::connect(_ui->_buttonUpdateSliceHistogram, SIGNAL(clicked()), this, SLOT(updateSliceHistogram()));
	// Onglet "3. Vue tangentielle"
	QObject::connect(_ui->_sliderSelectTangentialSlice, SIGNAL(valueChanged(int)), this, SLOT(setTangentialSlice(int)));

	/*******************
	* Évènements du zoom
	********************/
	QObject::connect(&_sliceZoomer, SIGNAL(zoomFactorChanged(qreal,qreal)), this, SLOT(zoomInSliceView(qreal,qreal)));
	QObject::connect(&_sliceZoomer, SIGNAL(isMovedFrom(QPoint)), this, SLOT(dragInSliceView(QPoint)));
	QObject::connect(&_tangentialZoomer, SIGNAL(zoomFactorChanged(qreal,qreal)), this, SLOT(zoomInTangentialView(qreal,qreal)));
	QObject::connect(&_tangentialZoomer, SIGNAL(isMovedFrom(QPoint)), this, SLOT(dragInTangentialView(QPoint)));


	// Raccourcis des actions du menu
	_ui->_actionOpenDicom->setShortcut(Qt::CTRL + Qt::Key_O);
	QObject::connect(_ui->_actionOpenDicom, SIGNAL(triggered()), this, SLOT(openDicom()));
	_ui->_actionOpenTiff->setShortcut(Qt::CTRL + Qt::Key_T);
	QObject::connect(_ui->_actionOpenTiff, SIGNAL(triggered()), this, SLOT(openTiff()));
	_ui->_actionCloseImage->setShortcut(Qt::CTRL + Qt::Key_W);
	QObject::connect(_ui->_actionCloseImage, SIGNAL(triggered()), this, SLOT(closeImage()));
	_ui->_actionQuit->setShortcut(Qt::CTRL + Qt::Key_Q);
	QObject::connect(_ui->_actionQuit, SIGNAL(triggered()), this, SLOT(close()));

	closeImage();
}

MainWindow::~MainWindow()
{
	delete _plotKnotPithProfile;
	delete _knotPithProfile;
	delete _plotSectorHistogram;
	delete _sectorHistogram;
	delete _plotSliceHistogram;
	delete _sliceHistogram;
	delete _sliceView;
	if ( _tangentialBillon ) delete _tangentialBillon;
	if ( _billon ) delete _billon;
	PieChartSingleton::kill();
}


/*******************************
 * Public fonctions
 *******************************/

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
	if ( obj == _labelSliceView && _billon && _billon->hasPith() )
	{
		if ( event->type() == QEvent::MouseButtonRelease )
		{
			const QMouseEvent *mouseEvent = static_cast<const QMouseEvent*>(event);
			if ( (mouseEvent->button() == Qt::LeftButton) )
			{
				iCoord2D pos = iCoord2D(mouseEvent->x(),mouseEvent->y())/_sliceZoomer.factor();
				qDebug() << "Position (i,j) = " << pos.x << " , " << pos.y << " )";
				_currentSector = PieChartSingleton::getInstance()->sectorIndexOfAngle( _billon->pithCoord(_currentSlice).angle(uiCoord2D(mouseEvent->x(),mouseEvent->y())/_sliceZoomer.factor()) );
				_plotSectorHistogram->moveCursor(_currentSector);
				_ui->_plotSectorHistogram->replot();
				_ui->_polarSectorHistogram->replot();
				drawSlice();
			}
		}
	}
	return QMainWindow::eventFilter(obj, event);
}

/*******************************
 * Private slots
 *******************************/

void MainWindow::openDicom()
{
	QString folderName = QFileDialog::getExistingDirectory(this,tr("Sélection du répertoire DICOM"),QDir::homePath(),QFileDialog::ShowDirsOnly);
	if ( folderName.isEmpty() ) return;

	bool ok = false;
	const QString inverse = tr("Inversé");
	QString text = QInputDialog::getItem(this, tr("Ordre de chargement des coupes du billon"), tr("Ordre :"), QStringList() << tr("De lecture du fichier") << inverse, 0, false,&ok);
	if ( !ok || text.isEmpty() ) return;

	Billon *billon = DicomReader::read(folderName,!text.compare(inverse));
	if ( !billon ) return;

	closeImage();
	_billon = billon;
	updateUiComponentsValues();
	enabledComponents();

	setWindowTitle(QString("TKDetection - %1").arg(folderName.section(QDir::separator(),-1)));

	drawSlice();
}

void MainWindow::openTiff()
{
	QString fileName = QFileDialog::getOpenFileName(0,tr("Sélection du fichier TIF"),QDir::homePath(),tr("Images TIFF (*.tiff *.tif)"));
	if ( fileName.isEmpty() ) return;

	Billon *billon = TiffReader::read(fileName);
	if ( !billon ) return;

	closeImage();
	_billon = billon;
	updateUiComponentsValues();
	enabledComponents();

	setWindowTitle(QString("TKDetection - %1").arg(fileName.section(QDir::separator(),-1)));

	drawSlice();
}

void MainWindow::closeImage()
{
	if ( _billon )
	{
		delete _billon;
		_billon = 0;
	}

	if ( _tangentialBillon )
	{
		delete _tangentialBillon;
		_tangentialBillon = 0;
	}

	_mainPix = QImage(0,0,QImage::Format_ARGB32);
	_tangentialPix = QImage(0,0,QImage::Format_ARGB32);
	_treeRadius = 133.33;
	_ui->_checkRadiusAroundPith->setText( QString::number(_treeRadius) );
	updateSliceHistogram();

	_sectorHistogram->clear();
	_plotSectorHistogram->update(*_sectorHistogram);
	_ui->_plotSectorHistogram->replot();
	_ui->_polarSectorHistogram->replot();

	updateKnotPithProfile();

	selectSectorInterval(0);
	updateUiComponentsValues();
	enabledComponents();

	drawSlice();
	setWindowTitle("TKDetection");
}

/********/
/*******************************************/
/*******/

void MainWindow::drawSlice()
{
	if ( _billon )
	{
		const TKD::ViewType viewType = static_cast<const TKD::ViewType>(_ui->_comboViewType->currentIndex());
		const TKD::ProjectionType projectionType = _ui->_radioYProjection->isChecked()?TKD::Y_PROJECTION:_ui->_radioZProjection->isChecked()?TKD::Z_PROJECTION:TKD::POLAR_PROJECTION;
		const uint &currentSlice = _ui->_radioYProjection->isChecked()?_billon->n_rows-_currentYSlice-1:_currentSlice;
		const rCoord2D &pithCoord = _billon->hasPith()?_billon->pithCoord(_currentSlice):rCoord2D(_billon->n_cols/2,_billon->n_rows/2);
		uint width, height;

		switch (projectionType)
		{
			case TKD::POLAR_PROJECTION :
				if (_billon->hasPith() ) {
					height = qMin(qMin(pithCoord.x,_billon->n_cols-pithCoord.x-1),qMin(pithCoord.y,_billon->n_rows-pithCoord.y-1));
				}
				else {
					height = qMin(_billon->n_cols/2,_billon->n_rows/2);
				}
				width = _ui->_spinCartesianAngularResolution->value();
				break;
			case TKD::Y_PROJECTION : width = _billon->n_cols; height = _billon->n_slices; break;
			case TKD::Z_PROJECTION :
			default : width = _billon->n_cols; height = _billon->n_rows; break;
		}

		_mainPix = QImage(width,height,QImage::Format_ARGB32);
		_mainPix.fill(0xff000000);

		_sliceView->drawSlice(_mainPix, *_billon, viewType, currentSlice, Interval<int>(_ui->_spinMinIntensity->value(), _ui->_spinMaxIntensity->value()),
					  _ui->_spinZMotionMin->value(), _ui->_spinCartesianAngularResolution->value(), projectionType, TKD::ImageViewRender(_ui->_comboViewRender->currentIndex()));

		if ( (projectionType == TKD::Z_PROJECTION || projectionType == TKD::POLAR_PROJECTION) && _billon->hasPith() )
		{
			if ( projectionType == TKD::Z_PROJECTION )  _billon->pith().draw(_mainPix,_currentSlice);

			if ( _ui->_checkRadiusAroundPith->isChecked() && _treeRadius > 0 )
			{
				const qreal restrictedRadius = _treeRadius*_ui->_spinRestrictedAreaPercentage->value()/100.;
				QPainter painter(&_mainPix);
				painter.setPen(Qt::yellow);
				if ( projectionType == TKD::Z_PROJECTION )
					painter.drawEllipse(pithCoord.x,pithCoord.y,restrictedRadius,restrictedRadius);
				else
					painter.drawLine(0,restrictedRadius,width,restrictedRadius);
			}

			const bool inDrawingArea = (_ui->_comboSelectSliceInterval->currentIndex() > 0 &&
										_sliceHistogram->interval(_ui->_comboSelectSliceInterval->currentIndex()-1).containsClosed(_currentSlice));
			if ( inDrawingArea )
			{
				if ( !_sectorHistogram->isEmpty() )
				{
					PieChartSingleton::getInstance()->draw(_mainPix, pithCoord, _sectorHistogram->intervals(), projectionType);
					PieChartSingleton::getInstance()->draw(_mainPix, pithCoord, _currentSector, projectionType);
				}
			}
		}
	}
	else
	{
		_ui->_labelSliceNumber->setText(tr("Aucune"));
		_mainPix = QImage(1,1,QImage::Format_ARGB32);
	}

	_labelSliceView->setPixmap( QPixmap::fromImage(_mainPix) );
	_labelSliceView->resize(_sliceZoomer.factor() * _mainPix.size());
}

void MainWindow::drawTangentialView()
{
	if ( _tangentialBillon )
	{
		const uint &currentSlice = _ui->_sliderSelectTangentialSlice->value();
		const uint &depth = _tangentialBillon->n_slices;

		uint width = _tangentialBillon->n_cols;
		uint height = _tangentialBillon->n_rows;

		const rCoord2D &pithCoord = _tangentialBillon->hasPith()?_tangentialBillon->pithCoord(currentSlice):rCoord2D(width/2,height/2);
		const TKD::ProjectionType projectionType = _ui->_radioPolarProjection->isChecked()?TKD::POLAR_PROJECTION:
																						   _ui->_radioEllipticProjection->isChecked()?TKD::ELLIPTIC_PROJECTION:
																																	  TKD::Z_PROJECTION;
		switch (projectionType)
		{
			case TKD::POLAR_PROJECTION :
				if (_tangentialBillon->hasPith() ) {
					height = qMin(qMin(pithCoord.x,width-pithCoord.x),qMin(pithCoord.y,height-pithCoord.y));
				}
				else {
					height = qMin(width/2,height/2);
				}
				width = _ui->_spinCartesianAngularResolution->value();
				break;
			case TKD::Z_PROJECTION :
			default :
				break;
		}

		_tangentialPix = QImage(width,height,QImage::Format_ARGB32);
		_tangentialPix.fill(0xff000000);

		_sliceView->drawSlice( _tangentialPix, *_tangentialBillon, TKD::CLASSIC, currentSlice, Interval<int>(_ui->_spinMinIntensity->value(), _ui->_spinMaxIntensity->value()),
							   _ui->_spinZMotionMin->value(), _ui->_spinCartesianAngularResolution->value(), projectionType, TKD::ImageViewRender(_ui->_comboViewRender->currentIndex()),
							   _knotPithProfile->size()?qCos((*_knotPithProfile)[currentSlice]):1.);

		if ( projectionType == TKD::Z_PROJECTION )
		{
                  QVector<QColor> colors(0) ;
                  colors.append( Qt::blue);
                  colors.append( Qt::green);
                  colors.append( Qt::cyan);
                  colors.append( Qt::white);
                  

			const int nbKnotAreas = _sectorHistogram->intervals().size();
			const int nbColorsToUse = qMax( nbKnotAreas>colors.size() ? ((nbKnotAreas+1)/2)%colors.size() : colors.size() , 1 );
			const QColor currentColor = colors[(_ui->_comboSelectSectorInterval->currentIndex()-1)%nbColorsToUse];

			const qreal knotAreaLine = currentSlice * (width/2.) / static_cast<qreal>(depth);

			QPainter painter(&_tangentialPix);
			painter.setPen(currentColor);
			painter.drawLine(width/2.-knotAreaLine,0,width/2.-knotAreaLine,height);
			painter.drawLine(width/2.+knotAreaLine,0,width/2.+knotAreaLine,height);
			painter.end();

			if ( _tangentialBillon->hasPith() )
			{
				_tangentialBillon->pith().draw(_tangentialPix,currentSlice, 2);
			}
		}
	}
	else
	{
		_tangentialPix = QImage(1,1,QImage::Format_ARGB32);
	}

	_labelTangentialView->setPixmap( QPixmap::fromImage(_tangentialPix) );
	_labelTangentialView->resize(_tangentialZoomer.factor() * _tangentialPix.size());
}


void MainWindow::zoomInSliceView( const qreal &zoomFactor, const qreal &zoomCoefficient )
{
	_labelSliceView->resize(zoomFactor * _mainPix.size());
	QScrollBar *hBar = _ui->_scrollSliceView->horizontalScrollBar();
	hBar->setValue(int(zoomCoefficient * hBar->value() + ((zoomCoefficient - 1) * hBar->pageStep()/2)));
	QScrollBar *vBar = _ui->_scrollSliceView->verticalScrollBar();
	vBar->setValue(int(zoomCoefficient * vBar->value() + ((zoomCoefficient - 1) * vBar->pageStep()/2)));
}

void MainWindow::dragInSliceView( const QPoint &movementVector )
{
	QScrollArea &scrollArea = *(_ui->_scrollSliceView);
	if ( movementVector.x() ) scrollArea.horizontalScrollBar()->setValue(scrollArea.horizontalScrollBar()->value()-movementVector.x());
	if ( movementVector.y() ) scrollArea.verticalScrollBar()->setValue(scrollArea.verticalScrollBar()->value()-movementVector.y());
}

void MainWindow::zoomInTangentialView( const qreal &zoomFactor, const qreal &zoomCoefficient )
{
	_labelTangentialView->resize(zoomFactor * _tangentialPix.size());
	QScrollBar *hBar = _ui->_scrollTangentialView->horizontalScrollBar();
	hBar->setValue(int(zoomCoefficient * hBar->value() + ((zoomCoefficient - 1) * hBar->pageStep()/2)));
	QScrollBar *vBar = _ui->_scrollTangentialView->verticalScrollBar();
	vBar->setValue(int(zoomCoefficient * vBar->value() + ((zoomCoefficient - 1) * vBar->pageStep()/2)));
}

void MainWindow::dragInTangentialView( const QPoint &movementVector )
{
	QScrollArea &scrollArea = *(_ui->_scrollTangentialView);
	if ( movementVector.x() ) scrollArea.horizontalScrollBar()->setValue(scrollArea.horizontalScrollBar()->value()-movementVector.x());
	if ( movementVector.y() ) scrollArea.verticalScrollBar()->setValue(scrollArea.verticalScrollBar()->value()-movementVector.y());
}

void MainWindow::previousMaximumInSliceHistogram()
{
	const uint nbMaximums = _sliceHistogram->nbMaximums();
	if ( nbMaximums )
	{
		_currentMaximum = _currentMaximum <= 0 ? nbMaximums-1 : ( _currentMaximum - 1 ) % nbMaximums;
		_ui->_sliderSelectSlice->setValue( _sliceHistogram->maximumIndex(_currentMaximum) );
	}
	else
	{
		_currentMaximum = -1;
	}
}

void MainWindow::nextMaximumInSliceHistogram()
{
	const uint nbMaximums = _sliceHistogram->nbMaximums();
	if ( nbMaximums )
	{
		_currentMaximum = ( _currentMaximum + 1 ) % nbMaximums;
		_ui->_sliderSelectSlice->setValue( _sliceHistogram->maximumIndex(_currentMaximum) );
	}
	else
	{
		_currentMaximum = -1;
	}
}

/********/
/*******************************************/
/*******/

void MainWindow::setSlice( const int &sliceIndex )
{
	_currentSlice = sliceIndex;
	_ui->_labelSliceNumber->setNum(sliceIndex);

	_plotSliceHistogram->moveCursor(sliceIndex);
	_ui->_plotSliceHistogram->replot();

	drawSlice();
}

void MainWindow::setYSlice( const int &yPosition )
{
	_currentYSlice = yPosition;
	drawSlice();
}

void MainWindow::setTangentialSlice( const int &sliceIndex )
{
	_plotKnotPithProfile->moveCursor(sliceIndex);
	_ui->_plotKnotPithProfile->replot();

	drawTangentialView();
}

void MainWindow::setSectorNumber( const int &value )
{
	PieChartSingleton::getInstance()->setNumberOfAngularSectors(value);
}


/********/
/*******************************************/
/*******/

void MainWindow::selectSliceInterval( const int &index )
{
	if ( _tangentialBillon )
	{
		delete _tangentialBillon;
		_tangentialBillon = 0;
		updateKnotPithProfile();
	}
	_ui->_sliderSelectTangentialSlice->setValue(0);

	_ui->_comboSelectSectorInterval->blockSignals(true);
	_ui->_comboSelectSectorInterval->clear();
	_ui->_comboSelectSectorInterval->addItem(tr("Aucun"));
	_ui->_comboSelectSectorInterval->blockSignals(false);

	if ( index > 0 && index <= static_cast<int>(_sliceHistogram->nbIntervals()) )
	{
		const Interval<uint> &sliceInterval = _sliceHistogram->interval(index-1);
		updateSectorHistogram(sliceInterval);
		_ui->_sliderSelectSlice->setValue(_sliceHistogram->interval(index-1).mid());

		const QVector< Interval<uint> > &angularIntervals = _sectorHistogram->intervals();
		if ( !angularIntervals.isEmpty() )
		{
			qreal minAngle, maxAngle;
			for ( int i=0 ; i<angularIntervals.size() ; ++i )
			{
				const Interval<uint> &currentAngularInterval = angularIntervals[i];
				minAngle = PieChartSingleton::getInstance()->sector(currentAngularInterval.min()).minAngle()*RAD_TO_DEG_FACT;
				maxAngle = PieChartSingleton::getInstance()->sector(currentAngularInterval.max()).maxAngle()*RAD_TO_DEG_FACT;
				_ui->_comboSelectSectorInterval->addItem(tr("Secteur %1 : [ %2, %3 ] (%4 degres)").arg(i).arg(minAngle).arg(maxAngle)
														 .arg(currentAngularInterval.isValid()?maxAngle-minAngle:maxAngle-minAngle+360.));
			}
		}
	}
}

void MainWindow::selectCurrentSliceInterval()
{
	selectSliceInterval(_ui->_comboSelectSliceInterval->currentIndex());
}

void MainWindow::selectSectorInterval(const int &index, const bool &draw )
{
	if ( _tangentialBillon )
	{
		delete _tangentialBillon;
		_tangentialBillon = 0;
	}

	if ( index > 0 && index <= static_cast<int>(_sectorHistogram->nbIntervals()) && _billon->hasPith() )
	{
		const uint nbAngularSectors = PieChartSingleton::getInstance()->nbSectors();
		const Interval<uint> &sectorInterval = _sectorHistogram->interval(index-1);
		const uint semiAngularRange = ((sectorInterval.max() + (sectorInterval.isValid() ? 0. : nbAngularSectors)) - sectorInterval.min())/2;
		const uint &maximumIndex = _sectorHistogram->maximumIndex(index-1);
		Interval<uint> centeredSectorInterval( maximumIndex-semiAngularRange, maximumIndex+semiAngularRange );
		if ( maximumIndex<semiAngularRange ) centeredSectorInterval.setMin( nbAngularSectors + centeredSectorInterval.min() - 1 );
		if ( centeredSectorInterval.max() > nbAngularSectors-1 ) centeredSectorInterval.setMax( centeredSectorInterval.max() - nbAngularSectors + 1 );
		const Interval<uint> &sliceInterval = _sliceHistogram->interval(_ui->_comboSelectSliceInterval->currentIndex()-1);
		const Interval<int> intensityInterval(_ui->_spinMinIntensity->value(), _ui->_spinMaxIntensity->value());

		_tangentialBillon = BillonAlgorithms::tangentialTransform( *_billon, sliceInterval, centeredSectorInterval, intensityInterval.min() );
		_ui->_sliderSelectTangentialSlice->blockSignals(true);
		_ui->_sliderSelectTangentialSlice->setMaximum(_tangentialBillon->n_slices>0?_tangentialBillon->n_slices-1:0);
		_ui->_sliderSelectTangentialSlice->setValue(0);
		_ui->_sliderSelectTangentialSlice->blockSignals(false);

		PithExtractorBoukadida pithExtractor( _ui->_spinPithSubWindowWidth_knot->value()/_tangentialBillon->voxelWidth(),
											  _ui->_spinPithSubWindowHeight_knot->value()/_tangentialBillon->voxelHeight(),
											  _ui->_spinPithMaximumShift_knot->value(), _ui->_spinPithSmoothingRadius_knot->value(),
											  _ui->_spinPithMinimumWoodPercentage_knot->value(), Interval<int>( _ui->_spinPithMinIntensity_knot->value(), _ui->_spinPithMaxIntensity_knot->value() ),
											  _ui->_chechPithAscendingOrder_knot->isChecked());
		pithExtractor.process(*_tangentialBillon);
	}

	if (draw)
	{
		updateKnotPithProfile();
		drawSlice();
		drawTangentialView();
	}
}

void MainWindow::selectCurrentSectorInterval()
{
	selectSectorInterval(_ui->_comboSelectSectorInterval->currentIndex());
}

void MainWindow::updateBillonPith()
{
	if ( _billon )
	{
		PithExtractorBoukadida pithExtractor( _ui->_spinPithSubWindowWidth_billon->value(), _ui->_spinPithSubWindowHeight_billon->value(),
											  _ui->_spinPithMaximumShift_billon->value(), _ui->_spinPithSmoothingRadius_billon->value(),
											  _ui->_spinPithMinimumWoodPercentage_billon->value(), Interval<int>( _ui->_spinPithMinIntensity_billon->value(), _ui->_spinPithMaxnIntensity_billon->value() ),
											  _ui->_chechPithAscendingOrder_billon->isChecked());
		pithExtractor.process(*_billon);
		_treeRadius = BillonAlgorithms::restrictedAreaMeansRadius(*_billon,_ui->_spinRestrictedAreaResolution->value(),_ui->_spinRestrictedAreaThreshold->value(),_ui->_spinRestrictedAreaMinimumRadius->value()*_billon->n_cols/100., _ui->_spinHistogramPercentageOfSlicesToIgnore_zMotion->value()*_billon->n_slices/100.);
		_ui->_checkRadiusAroundPith->setText( QString::number(_treeRadius) );
		_ui->_spinHistogramNumberOfAngularSectors_zMotionAngular->setValue(TWO_PI*_treeRadius);
	}
	drawSlice();
	updateSliceHistogram();
}

void MainWindow::updateSliceHistogram()
{
	_sliceHistogram->clear();

	if ( _billon && _billon->hasPith() )
	{
		_sliceHistogram->construct(*_billon, Interval<int>(_ui->_spinMinIntensity->value(),_ui->_spinMaxIntensity->value()),
								   _ui->_spinZMotionMin->value(), _ui->_spinHistogramPercentageOfSlicesToIgnore_zMotion->value()*_billon->n_slices/100., _treeRadius*_ui->_spinRestrictedAreaPercentage->value()/100.);
		_sliceHistogram->computeMaximumsAndIntervals( _ui->_spinHistogramSmoothingRadius_zMotion->value(), _ui->_spinHistogramMinimumHeightOfMaximum_zMotion->value(),
													  _ui->_spinHistogramDerivativeSearchPercentage_zMotion->value(), _ui->_spinHistogramMinimumWidthOfInterval_zMotion->value(), false);
	}
	_plotSliceHistogram->update( *_sliceHistogram );
	_plotSliceHistogram->moveCursor( _currentSlice );
	_plotSliceHistogram->updatePercentageCurve( _sliceHistogram->thresholdOfMaximums( _ui->_spinHistogramMinimumHeightOfMaximum_zMotion->value() ) );
	_ui->_plotSliceHistogram->setAxisScale(QwtPlot::xBottom,0,_sliceHistogram->size());
	_ui->_plotSliceHistogram->replot();

	const int oldIntervalIndex = _ui->_comboSelectSliceInterval->currentIndex();
	_ui->_comboSelectSliceInterval->clear();
	_ui->_comboSelectSliceInterval->addItem(tr("Aucun"));
	const QVector< Interval<uint> > &intervals = _sliceHistogram->intervals();
	if ( !intervals.isEmpty() )
	{
		for ( int i=0 ; i<intervals.size() ; ++i )
		{
			const Interval<uint> &interval = intervals[i];
			_ui->_comboSelectSliceInterval->addItem(tr("Interval %1 : [ %2, %3 ] (%4 coupes)").arg(i).arg(interval.min()).arg(interval.max()).arg(interval.width()+1));
		}
	}
	_ui->_comboSelectSliceInterval->setCurrentIndex(oldIntervalIndex<=intervals.size()?oldIntervalIndex:0);
}

void MainWindow::updateSectorHistogram( const Interval<uint> &interval )
{
	_sectorHistogram->clear();

	if ( _billon )
	{
		_sectorHistogram->construct( *_billon, interval, Interval<int>(_ui->_spinMinIntensity->value(),_ui->_spinMaxIntensity->value()),
									 _ui->_spinZMotionMin->value(), _treeRadius*_ui->_spinRestrictedAreaPercentage->value()/100.);
		qreal coeffDegToSize = _ui->_spinHistogramNumberOfAngularSectors_zMotionAngular->value()/360.;
		_sectorHistogram->computeMaximumsAndIntervals( _ui->_spinHistogramSmoothingRadius_zMotionAngular->value()*coeffDegToSize,
													   _ui->_spinHistogramMinimumHeightOfMaximum_zMotionAngular->value(),
													   _ui->_spinHistogramDerivativeSearchPercentage_zMotionAngular->value(),
													   _ui->_spinHistogramMinimumWidthOfInterval_zMotionAngular->value()*coeffDegToSize,
													   _ui->_spinHistogramIntervalGap_zMotionAngular->value(), true );
	}

	_plotSectorHistogram->update(*_sectorHistogram);
	_ui->_plotSectorHistogram->replot();
	_ui->_polarSectorHistogram->replot();
	drawSlice();
}

void MainWindow::updateKnotPithProfile()
{
	_knotPithProfile->clear();

	if ( _tangentialBillon && _tangentialBillon->hasPith() )
	{
		_knotPithProfile->construct( _tangentialBillon->pith(), _tangentialBillon->voxelDims() );
	}

	_plotKnotPithProfile->update( *_knotPithProfile );
	_plotKnotPithProfile->moveCursor( _ui->_sliderSelectTangentialSlice->value() );
	_ui->_plotKnotPithProfile->setAxisScale(QwtPlot::xBottom,0,_knotPithProfile->size());
	_ui->_plotKnotPithProfile->replot();
}

/********/
/*******************************************/
/*******/

void MainWindow::resetHistogramDefaultValuesZMotion()
{
	_ui->_spinHistogramSmoothingRadius_zMotion->setValue(HISTOGRAM_SMOOTHING_RADIUS);
	_ui->_spinHistogramMinimumHeightOfMaximum_zMotion->setValue(HISTOGRAM_PERCENTAGE_OF_MINIMUM_HEIGHT_OF_MAXIMUM);
	_ui->_spinHistogramMinimumWidthOfInterval_zMotion->setValue(HISTOGRAM_MINIMUM_WIDTH_OF_INTERVALS);
	_ui->_spinHistogramDerivativeSearchPercentage_zMotion->setValue(HISTOGRAM_DERIVATIVE_SEARCH_PERCENTAGE);
	_ui->_spinHistogramPercentageOfSlicesToIgnore_zMotion->setValue(HISTOGRAM_PERCENTAGE_OF_SLICES_TO_IGNORE);
}

void MainWindow::resetHistogramDefaultValuesZMotionAngular()
{
	_ui->_spinHistogramNumberOfAngularSectors_zMotionAngular->setValue(TWO_PI*_treeRadius);
	_ui->_spinHistogramSmoothingRadius_zMotionAngular->setValue(HISTOGRAM_ANGULAR_SMOOTHING_RADIUS);
	_ui->_spinHistogramMinimumHeightOfMaximum_zMotionAngular->setValue(HISTOGRAM_ANGULAR_PERCENTAGE_OF_MINIMUM_HEIGHT_OF_MAXIMUM);
	_ui->_spinHistogramMinimumWidthOfInterval_zMotionAngular->setValue(HISTOGRAM_ANGULAR_MINIMUM_WIDTH_OF_INTERVALS);
	_ui->_spinHistogramDerivativeSearchPercentage_zMotionAngular->setValue(HISTOGRAM_ANGULAR_DERIVATIVE_SEARCH_PERCENTAGE);
	_ui->_spinHistogramIntervalGap_zMotionAngular->setValue(HISTOGRAM_ANGULAR_INTERVAL_GAP);
}


/********/
/*******************************************/
/*******/

void MainWindow::exportToOfs()
{
	if ( _billon )
	{
		switch (_ui->_comboOfsExportType->currentIndex())
		{
			case TKD::COMPLETE_PITH:
				exportPithToOfs(false);
				break;
			case TKD::PITH_ON_CURRENT_SLICE_INTERVAL:
				exportPithToOfs(true);
				break;
			case TKD::COMPLETE_BILLON:
				exportCompleteBillonToOfs();
				break;
			case TKD::ALL_KNOT_AREAS:
//				exportAllKnotAreasToOfs();
				break;
			case TKD::ALL_KNOT_AREAS_OF_CURRENT_SLICE_INTERVAL:
				break;
			case TKD::CURRENT_KNOT_AREA:
//				exportCurrentKnotAreaToOfs();
				break;
			default:
				QMessageBox::warning(this,tr("Export en .ofs"), tr("Contenu à exporter inconnu."));
				break;
		}
	}
	else QMessageBox::warning(this,tr("Export en .ofs"), tr("Aucun fichier de billon ouvert."));
}

void MainWindow::exportHistograms()
{
	int type = _ui->_comboExportHistoType->currentIndex();
	int format = _ui->_comboExportHistoFormat->currentIndex();
	switch (type)
	{
		case TKD::SLICE_HISTOGRAM:
			switch ( format )
			{
				case 0: exportSliceHistogramToSep(); break;
				case 1: exportSliceHistogramToImage(); break;
				default: QMessageBox::warning(this,tr("Exporter l'histogramme de coupes"),tr("L'export a échoué : format inconnu.")); break;
			}
			break;
		case TKD::SECTOR_HISTOGRAM:
			switch ( format )
			{
				case 0: exportSectorHistogramToSep(); break;
				case 1: exportSectorHistogramToImage(); break;
				default: QMessageBox::warning(this,tr("Exporter l'histogramme de secteurs"),tr("L'export a échoué : format inconnu.")); break;
			}
			break;
		default: QMessageBox::warning(this,tr("Exporter les histogramme"),tr("L'histogramme demandé n'est pas prévu pour l'export.")); break;
	}

}

void MainWindow::exportToPgm3D()
{
	int type = _ui->_comboExportPgm3dType->currentIndex();
	switch (type)
	{
//		case 0: exportCurrentSegmentedKnotToPgm3d();	break;
//		case 1: exportSegmentedKnotsOfCurrentSliceIntervalToPgm3d();	break;
		case 2: exportImageSliceIntervalToPgm3d();	break;
		case 3: exportImageCartesianSliceIntervalToPgm3d();	break;

	default: break;
	}
}

void MainWindow::exportToV3D()
{
	int type = _ui->_comboExportToV3DType->currentIndex();
	switch (type)
	{
//		case 0: exportCurrentSegmentedKnotToV3D();	break;
//		case 1: exportSegmentedKnotsOfCurrentSliceIntervalToV3D();	break;
//		case 2: exportAllSegmentedKnotsOfBillonToV3D(); break;
		default: break;
	}
}

void MainWindow::exportToSdp()
{
	switch ( _ui->_comboExportToSdpType->currentIndex() )
	{
//		case 0 : exportContourToSdp();	break;
//		case 1 : exportCurrentSegmentedKnotToSdp();	break;
//		case 2 : exportSegmentedKnotsOfCurrentSliceIntervalToSdp(_ui->_checkBoxKeepBillonSliceNumber->isChecked() );	break;
//		case 3 : exportAllSegmentedKnotsOfBillonToSdp(); break;
//		case 4 : exportSegmentedKnotsOfCurrentSliceIntervalToSdpOldAlgo(_ui->_checkBoxKeepBillonSliceNumber->isChecked() );	break;
		default : break;
	}
}

/*******************************
 * Private functions
 *******************************/

void MainWindow::openNewBillon( const QString &fileName )
{
	if ( _billon )
	{
		delete _billon;
		_billon = 0;
	}
	if ( !fileName.isEmpty() )
	{
		_billon = DicomReader::read(fileName);
	}
	if ( _billon )
	{
		_mainPix = QImage(_billon->n_cols, _billon->n_rows,QImage::Format_ARGB32);
	}
	else
	{
		_mainPix = QImage(0,0,QImage::Format_ARGB32);
	}
	_tangentialPix = QImage(0,0,QImage::Format_ARGB32);
}

void MainWindow::initComponentsValues() {
	_ui->_spanSliderIntensityInterval->setMinimum(MINIMUM_INTENSITY);
	_ui->_spanSliderIntensityInterval->setLowerValue(MINIMUM_INTENSITY);
	_ui->_spanSliderIntensityInterval->setMaximum(MAXIMUM_INTENSITY);
	_ui->_spanSliderIntensityInterval->setUpperValue(MAXIMUM_INTENSITY);

	_ui->_spinMinIntensity->setMinimum(MINIMUM_INTENSITY);
	_ui->_spinMinIntensity->setMaximum(MAXIMUM_INTENSITY);
	_ui->_spinMinIntensity->setValue(MINIMUM_INTENSITY);

	_ui->_spinMaxIntensity->setMinimum(MINIMUM_INTENSITY);
	_ui->_spinMaxIntensity->setMaximum(MAXIMUM_INTENSITY);
	_ui->_spinMaxIntensity->setValue(MAXIMUM_INTENSITY);

	_ui->_sliderSelectSlice->setValue(0);
	_ui->_sliderSelectSlice->setRange(0,0);

	_ui->_sliderSelectYSlice->setValue(0);
	_ui->_sliderSelectYSlice->setRange(0,0);

	_ui->_spinZMotionMin->setMinimum(0);
	_ui->_spinZMotionMin->setMaximum(2000);
	_ui->_spinZMotionMin->setValue(200);

	_ui->_spinHistogramMinimumWidthOfInterval_zMotion->setMinimum(0);
	_ui->_spinHistogramMinimumWidthOfInterval_zMotion->setMaximum(50);
	_ui->_spinHistogramMinimumWidthOfInterval_zMotion->setValue(HISTOGRAM_MINIMUM_WIDTH_OF_INTERVALS);
}

void MainWindow::updateUiComponentsValues()
{
	int minValue, maxValue, nbSlices, height, angularResolution;

	if ( _billon )
	{
		minValue = _billon->minValue();
		maxValue = _billon->maxValue();
		nbSlices = _billon->n_slices-1;
		height = _billon->n_rows;
		angularResolution = (height+_billon->n_cols)*2;
		_ui->_labelSliceNumber->setNum(0);
		_ui->_statusBar->showMessage( tr("Dimensions de voxels (largeur, hauteur, profondeur) : ( %1, %2, %3 )")
									  .arg(_billon->voxelWidth()).arg(_billon->voxelHeight()).arg(_billon->voxelDepth()) );
	}
	else
	{
		minValue = maxValue = 0;
		nbSlices = height = 0;
		angularResolution = 360;
		_ui->_labelSliceNumber->setText(tr("Aucune coupe présente."));
		_ui->_statusBar->clearMessage();
	}

	_ui->_sliderSelectSlice->setValue(0);
	_ui->_sliderSelectSlice->setRange(0,nbSlices);

	_ui->_sliderSelectYSlice->setValue(0);
	_ui->_sliderSelectYSlice->setRange(0,height-1);

	_ui->_spinMinIntensity->setMinimum(minValue);
	_ui->_spinMinIntensity->setMaximum(maxValue);
	_ui->_spinMinIntensity->setValue(MINIMUM_INTENSITY);

	_ui->_spinMaxIntensity->setMinimum(minValue);
	_ui->_spinMaxIntensity->setMaximum(maxValue);
	_ui->_spinMaxIntensity->setValue(MAXIMUM_INTENSITY);

	_ui->_spanSliderIntensityInterval->setMinimum(minValue);
	_ui->_spanSliderIntensityInterval->setLowerValue(MINIMUM_INTENSITY);
	_ui->_spanSliderIntensityInterval->setMaximum(maxValue);
	_ui->_spanSliderIntensityInterval->setUpperValue(MAXIMUM_INTENSITY);

	_ui->_checkRadiusAroundPith->setText( QString::number(_treeRadius) );

	_ui->_spinCartesianAngularResolution->setMaximum(angularResolution);
	_ui->_sliderCartesianAngularResolution->setMaximum(angularResolution);
}

void MainWindow::enabledComponents()
{
	const bool enable = _billon;
	_ui->_sliderSelectSlice->setEnabled(enable);
	_ui->_sliderSelectYSlice->setEnabled(enable);
	_ui->_spanSliderIntensityInterval->setEnabled(enable);
	_ui->_buttonComputePith->setEnabled(enable);
	_ui->_buttonUpdateSliceHistogram->setEnabled(enable);
	_ui->_buttonExportHistograms->setEnabled(enable);
	_ui->_buttonExportToOfs->setEnabled(enable);
	_ui->_buttonNextMaximum->setEnabled(enable);
	_ui->_buttonPreviousMaximum->setEnabled(enable);
}

void MainWindow::exportPithToOfs( const bool &onCurrentSliceInterval )
{
	if ( !_billon )
	{
		QMessageBox::warning( this, tr("Export en .ofs"), tr("Aucun billon ouvert."));
		return;
	}
	if ( !_billon->hasPith() )
	{
		QMessageBox::warning( this, tr("Export en .ofs"), tr("La moelle n'est pas calculée."));
		return;
	}
	if ( onCurrentSliceInterval && !_ui->_comboSelectSliceInterval->currentIndex() )
	{
		QMessageBox::warning( this, tr("Export en .ofs"), tr("Aucun intervalle de coupes selectionné."));
		return;
	}
	QString fileName = QFileDialog::getSaveFileName( this, tr("Exporter en .ofs"), "output.ofs", tr("Fichiers de données (*.ofs);;Tous les fichiers (*.*)"));
	if ( !fileName.isEmpty() )
	{
		QFile file(fileName);
		if ( file.open(QIODevice::WriteOnly) )
		{
			QTextStream stream(&file);
			OfsExport::writeHeader( stream );
			const Interval<uint> &sliceInterval = onCurrentSliceInterval ? _sliceHistogram->interval(_ui->_comboSelectSliceInterval->currentIndex()) : Interval<uint>(0,_billon->n_slices-1);
			OfsExport::processOnPith( stream, *_billon, sliceInterval, _ui->_spinExportNbEdges->value(), _ui->_spinExportRadius->value(), false );
			file.close();
			QMessageBox::information( this, tr("Export en .ofs"), tr("Terminé avec succés !"));
		}
		else QMessageBox::warning( this, tr("Export en .ofs"), tr("Impossible d'écrire le fichier."));
	}
}

void MainWindow::exportCompleteBillonToOfs()
{
	if ( !_billon )
	{
		QMessageBox::warning( this, tr("Export en .ofs"), tr("Aucun billon ouvert."));
		return;
	}
	if ( !_billon->hasPith() )
	{
		QMessageBox::warning( this, tr("Export en .ofs"), tr("La moelle n'est pas calculée."));
		return;
	}
	QString fileName = QFileDialog::getSaveFileName(this, tr("Exporter en .ofs"), "output.ofs", tr("Fichiers de données (*.ofs);;Tous les fichiers (*.*)"));
	if ( !fileName.isEmpty() )
	{
		QFile file(fileName);
		if ( file.open(QIODevice::WriteOnly) )
		{
			QTextStream stream(&file);
			const Interval<uint> sliceInterval(0,_billon->n_slices-1);
			QVector<rCoord2D> vectVertex = BillonAlgorithms::restrictedAreaVertex( *_billon, sliceInterval, _ui->_spinRestrictedAreaResolution->value(),
																				   _ui->_spinRestrictedAreaThreshold->value() );
			if ( !vectVertex.isEmpty() )
			{
				OfsExport::writeHeader( stream );
				OfsExport::processOnRestrictedMesh( stream, *_billon, sliceInterval, vectVertex, _ui->_spinRestrictedAreaResolution->value(), false, _ui->_checkCloseBillon->isChecked() );
				QMessageBox::information( this, tr("Export en .ofs"), tr("Terminé avec succés !"));
			}
			else QMessageBox::warning( this, tr("Export en .ofs"), tr("Erreur lors de la création du tube."));
			file.close();
		}
		else QMessageBox::warning( this, tr("Export en .ofs"), tr("Impossible d'écrire le fichier."));
	}
}

//void MainWindow::exportAllKnotAreasToOfs()
//{
//	if ( !_billon )
//	{
//		QMessageBox::warning( this, tr("Export en .ofs"), tr("Aucun billon ouvert."));
//		return;
//	}
//	if ( !_billon->hasPith() )
//	{
//		QMessageBox::warning( this, tr("Export en .ofs"), tr("La moelle n'est pas calculée."));
//		return;
//	}
//	QString fileName = QFileDialog::getSaveFileName(this, tr("Exporter en .ofs"), "output.ofs", tr("Fichiers de données (*.ofs);;Tous les fichiers (*.*)"));
//	if ( !fileName.isEmpty() )
//	{
//		const QVector< Interval<uint> > &sliceIntervals = _sliceHistogram->intervals();
//		QVector< QVector< Interval<qreal> > > angleIntervals(sliceIntervals.size());
//		for ( int i=0 ; i<sliceIntervals.size() ; ++i )
//		{
//			_ui->_comboSelectSliceInterval->setCurrentIndex(i+1);
//			for ( int j=0 ; j<_sectorHistogram->intervals().size() ; ++j )
//			{
//				const Interval<uint> &sectorInterval = _sectorHistogram->interval(j);
//				angleIntervals[i].append( Interval<qreal>( PieChartSingleton::getInstance()->sector(sectorInterval.min()).minAngle(), PieChartSingleton::getInstance()->sector(sectorInterval.max()).maxAngle() ) );
//			}
//		}

//		QFile file(fileName);
//		if ( file.open(QIODevice::WriteOnly) )
//		{
//			QTextStream stream(&file);
//			OfsExport::writeHeader( stream );
//			OfsExport::processOnAllSectorInAllIntervals( stream, *_billon, _ui->_spinExportNbEdges->value(), _treeRadius, sliceIntervals, angleIntervals, false );
//			QMessageBox::information(this,tr("Export en .ofs"), tr("Terminé avec succés !"));
//			file.close();
//		}
//		else QMessageBox::warning( this, tr("Export en .ofs"), tr("Impossible d'écrire le fichier."));
//	}
//}

//void MainWindow::exportCurrentKnotAreaToOfs()
//{
//	if ( !_billon )
//	{
//		QMessageBox::warning( this, tr("Export en .ofs"), tr("Aucun billon ouvert."));
//		return;
//	}
//	if ( !_knotBillon )
//	{
//		QMessageBox::warning( this, tr("Export en .ofs"), tr("Aucune zone de nœud selectionnée."));
//		return;
//	}
//	QString fileName = QFileDialog::getSaveFileName(this, tr("Exporter en .ofs"), "output.ofs", tr("Fichiers de données (*.ofs);;Tous les fichiers (*.*)"));
//	if ( !fileName.isEmpty() )
//	{
//		const Interval<uint> &sliceInterval = _sliceHistogram->interval(_ui->_comboSelectSliceInterval->currentIndex()-1);
//		const Interval<uint> &sectorInterval = _sectorHistogram->interval(_ui->_comboSelectSectorInterval->currentIndex()-1);
//		QFile file(fileName);
//		if ( file.open(QIODevice::WriteOnly) )
//		{
//			QTextStream stream(&file);
//			OfsExport::writeHeader( stream );
//			OfsExport::processOnSector( stream, *_knotBillon, _ui->_spinExportNbEdges->value(), _treeRadius, sliceInterval, Interval<qreal>( PieChartSingleton::getInstance()->sector(sectorInterval.min()).minAngle(),
//										PieChartSingleton::getInstance()->sector(sectorInterval.max()).maxAngle() ), false );
//			QMessageBox::information(this,tr("Export en .ofs"), tr("Terminé avec succés !"));
//			file.close();
//		}
//		else QMessageBox::warning( this, tr("Export en .ofs"), tr("Impossible d'écrire le fichier."));
//	}
//}

void MainWindow::exportSliceHistogramToSep()
{
	if ( _sliceHistogram->size() )
	{
		QString fileName = QFileDialog::getSaveFileName(this, tr("Exporter l'histogramme de coupes en .sep"), "output.sep",
														tr("Fichiers séquences de point euclidiens (*.sep);;Tous les fichiers (*.*)"));
		if ( !fileName.isEmpty() )
		{
			QFile file(fileName);
			if ( file.open(QIODevice::WriteOnly) )
			{
				QTextStream stream(&file);
				stream << *_sliceHistogram;
				file.close();
				QMessageBox::information(this,tr("Exporter l'histogramme de coupes en .sep"), tr("Terminé avec succés !"));
			}
			else QMessageBox::warning(this,tr("Exporter l'histogramme de coupes en .sep"),tr("L'export a échoué : impossible de créer le fichier."));
		}
	}
	else QMessageBox::warning(this,tr("Exporter l'histogramme de coupes en .sep"),tr("L'export a échoué : l'histogramme de coupes n'est pas calculé."));
}

void MainWindow::exportSectorHistogramToSep()
{
	if ( _sectorHistogram->size() )
	{
		QString fileName = QFileDialog::getSaveFileName(this, tr("Exporter l'histogramme de secteurs en .sep"), "output.sep",
														tr("Fichiers séquences de point euclidiens (*.sep);;Tous les fichiers (*.*)"));
		if ( !fileName.isEmpty() )
		{
			QFile file(fileName);
			if ( file.open(QIODevice::WriteOnly) )
			{
				QTextStream stream(&file);
				stream << *_sectorHistogram;
				file.close();
				QMessageBox::information(this,tr("Exporter l'histogramme de secteurs en .sep"), tr("Terminé avec succés !"));
			}
			else QMessageBox::warning(this,tr("Exporter l'histogramme de secteurs en .sep"),tr("L'export a échoué : impossible de créer le fichier."));
		}
	}
	else QMessageBox::warning(this,tr("Exporter l'histogramme de secteurs en .sep"),tr("L'export a échoué : l'histogramme de secteurs n'est pas calculé."));
}

void MainWindow::exportSliceHistogramToImage()
{
	QString fileName;
	QwtPlotRenderer histoRenderer;
	QMessageBox::StandardButton button;
	QLabel label1;
	QPixmap image1;
	int sizeFact;
	bool sizeOk, abort;
	sizeOk = abort = false;

	if ( _sliceHistogram->size() )
	{
		while (!sizeOk && !abort)
		{
			sizeFact = QInputDialog::getInt(this,tr("Taille de l'image"), tr("Pourcentage"), 100, 10, 100, 1, &sizeOk);
			if ( sizeOk )
			{
				image1 = QPixmap( 1240*sizeFact/100 , 874*sizeFact/100 );
				image1.fill();

				_ui->_plotSliceHistogram->setAxisTitle(QwtPlot::xBottom,tr("Indice de la coupe"));
				_ui->_plotSliceHistogram->enableAxis(QwtPlot::yLeft);
				_ui->_plotSliceHistogram->setAxisTitle(QwtPlot::yLeft,tr("Cumul du z-mouvement"));

				histoRenderer.renderTo(_ui->_plotSliceHistogram,image1);

				image1 = image1.scaledToHeight(600,Qt::SmoothTransformation);
				label1.setPixmap(image1);
				label1.show();

				button = QMessageBox::question(&label1,tr("Taille correcte"),tr("La taille de l'image est-elle correcte ?"),QMessageBox::Abort|QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes);
				switch ( button )
				{
					case QMessageBox::Yes:
						fileName = QFileDialog::getSaveFileName(&label1, tr("Export de l'histogramme de coupes en image"), "output.pdf",
																tr("Fichiers PDF (*.pdf);;Fichiers PS (*.ps);;Fichiers PNG (*.png);;Fichiers SVG (*.svg);;Tous les fichiers (*.*)"));
						if ( !fileName.isEmpty() )
						{
							histoRenderer.renderDocument(_ui->_plotSliceHistogram,fileName,QSize(297*sizeFact/100,210*sizeFact/100),100);
							QMessageBox::information(this,tr("Export de l'histogramme de coupes en image"), tr("Terminé avec succés !"));
						}
						else QMessageBox::warning(this,tr("Export de l'histogramme de coupes en image"), tr("Impossible de créer le fichier."));
						sizeOk = true;
						break;
					case QMessageBox::No:
						sizeOk = false;
						break;
					case QMessageBox::Abort:
					default :
						abort = true;
						break;
				}

				_ui->_plotSliceHistogram->setAxisTitle(QwtPlot::xBottom,"");
				_ui->_plotSliceHistogram->setAxisTitle(QwtPlot::yLeft,"");
				_ui->_plotSliceHistogram->enableAxis(QwtPlot::yLeft,false);
			}
			else
			{
				abort = true;
				QMessageBox::warning(this,tr("Export de l'histogramme de coupes en image"), tr("Erreur lors de la saisie de la taille de l'image."));
			}
		}
	}
	else QMessageBox::warning(this,tr("Export de l'histogramme de coupes en image"),tr("L'export a échoué : l'histogramme de coupes n'est pas calculé."));
}

void MainWindow::exportSectorHistogramToImage()
{
	QString fileName, chemin, name;
	QwtPlotRenderer histoRenderer;
	QwtPolarRenderer diagramRenderer;
	QMessageBox::StandardButton button;
	QLabel label1, label2;
	QPixmap image1, image2;
	QSize imageSize;
	int sizeFact;
	bool sizeOk, abort;
	sizeOk = abort = false;

	if ( _sectorHistogram->size() )
	{
		while (!sizeOk && !abort)
		{
			sizeFact = QInputDialog::getInt(this,tr("Taille de l'image"), tr("Pourcentage"), 100, 10, 100, 1, &sizeOk);
			if ( sizeOk )
			{
				imageSize = QSize(297*sizeFact/100,210*sizeFact/100);
				image1 = QPixmap( 1240*sizeFact/100 , 874*sizeFact/100 );
				image1.fill();
				image2 = QPixmap( 1240*sizeFact/100 , 874*sizeFact/100 );
				image2.fill();

				_ui->_plotSectorHistogram->setAxisTitle(QwtPlot::xBottom,tr("Secteur angulaire en radians"));
				_ui->_plotSectorHistogram->enableAxis(QwtPlot::yLeft);
				_ui->_plotSectorHistogram->setAxisTitle(QwtPlot::yLeft,tr("Cumul du z-mouvement"));

				histoRenderer.renderTo(_ui->_plotSectorHistogram,image1);
				diagramRenderer.renderTo(_ui->_polarSectorHistogram,image2);

				image1 = image1.scaledToHeight(600,Qt::SmoothTransformation);
				label1.setPixmap(image1);
				label1.show();
				image2 = image2.scaledToHeight(600,Qt::SmoothTransformation);
				label2.setPixmap(image2);
				label2.show();

				button = QMessageBox::question(&label1,tr("Taille correcte"),tr("La taille de l'image est-elle correcte ?"),QMessageBox::Abort|QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes);
				switch ( button )
				{
					case QMessageBox::Yes:
						fileName = QFileDialog::getSaveFileName(&label1, tr("Export de l'histogramme de secteurs en image"), "output.pdf",
																tr("Fichiers PDF (*.pdf);;Fichiers PS (*.ps);;Fichiers PNG (*.png);;Fichiers SVG (*.svg);;Tous les fichiers (*.*)"));
						if ( !fileName.isEmpty() )
						{
							chemin = fileName.section(QDir::separator(),0,-2)+QDir::separator();
							name = fileName.section(QDir::separator(),-1);
							histoRenderer.renderDocument(_ui->_plotSectorHistogram, chemin+"histo_"+name, imageSize, 100);
							diagramRenderer.renderDocument(_ui->_polarSectorHistogram, chemin+"diag_"+name, imageSize, 100);
							QMessageBox::information(this,tr("Export de l'histogramme de secteurs en image"), tr("Terminé avec succés !"));
						}
						else QMessageBox::warning(this,tr("Export de l'histogramme de secteurs en image"), tr("Impossible de créer les fichiers."));
						sizeOk = true;
						break;
					case QMessageBox::No:
						sizeOk = false;
						break;
					case QMessageBox::Abort:
					default :
						abort = true;
						break;
				}

				_ui->_plotSectorHistogram->setAxisTitle(QwtPlot::xBottom,"");
				_ui->_plotSectorHistogram->setAxisTitle(QwtPlot::yLeft,"");
				_ui->_plotSectorHistogram->enableAxis(QwtPlot::yLeft,false);
			}
			else
			{
				abort = true;
				QMessageBox::warning(this,tr("Export de l'histogramme de secteurs en image"), tr("Erreur lors de la saisie de la taille de l'image."));
			}
		}
	}
	else QMessageBox::warning(this,tr("Export de l'histogramme de secteurs en image"),tr("L'export a échoué : l'histogramme de secteurs n'est pas calculé."));
}


void MainWindow::exportImageSliceIntervalToPgm3d()
{
	if ( !_billon )
	{
		QMessageBox::warning(this,tr("Export en .pgm3d"), tr("Aucun billon ouvert."));
		return;
	}
	QString fileName = QFileDialog::getSaveFileName(this, tr("Exporter l'image en .pgm3d"), "output.pgm3d", tr("Fichiers de données (*.pgm3d);;Tous les fichiers (*.*)"));
	if ( !fileName.isEmpty() )
	{
		QFile file(fileName);
		if ( file.open(QIODevice::WriteOnly) )
		{
			QTextStream stream(&file);
			Pgm3dExport::processImage( stream, *_billon, Interval<int>(0,_billon->n_slices-1),
									   Interval<int>(_ui->_spinMinIntensity->value(),_ui->_spinMaxIntensity->value()), _ui-> _spinPgm3dExportResolution->value(),
									   (_ui->_spinPgm3dExportContrast->value()+100.)/100. );
			file.close();
			QMessageBox::information(this,tr("Exporter en PGM3D"), tr("Export réussi !"));
		}
		else QMessageBox::warning(this,tr("Export en .pgm3d"), tr("Impossible d'écrire le fichier."));
	}
}



void MainWindow::exportImageCartesianSliceIntervalToPgm3d()
{
	if ( !_billon )
	{
		QMessageBox::warning(this,tr("Export en .pgm3d"), tr("Aucun billon ouvert."));
		return;
	}
	QString fileName = QFileDialog::getSaveFileName(this, tr("Exporter l'image en .pgm3d"), "output.pgm3d", tr("Fichiers de données (*.pgm3d);;Tous les fichiers (*.*)"));
	if ( !fileName.isEmpty() )
	{
		QFile file(fileName);
		if ( file.open(QIODevice::WriteOnly) )
		{
			QTextStream stream(&file);
			Pgm3dExport::processImageCartesian( stream, *_billon, Interval<int>(0,_billon->n_slices-1),
												Interval<int>(_ui->_spinMinIntensity->value(),_ui->_spinMaxIntensity->value()),  _ui-> _spinPgm3dExportResolution->value(),
												_ui-> _spinCartesianAngularResolution->value(), (_ui->_spinPgm3dExportContrast->value()+100.)/100. );
			file.close();
			QMessageBox::information(this,tr("Exporter en PGM3D"), tr("Export réussi !"));
		}
		else QMessageBox::warning(this,tr("Export en .pgm3d"), tr("Impossible d'écrire le fichier."));
	}
}



//void MainWindow::exportCurrentSegmentedKnotToPgm3d()
//{
//	if ( _knotBillon )
//	{
//		QString fileName = QFileDialog::getSaveFileName(this, tr("Exporter le nœud courant segmenté en PGM3D"), "output.pgm3d", tr("Fichiers PGM3D (*.pgm3d);;Tous les fichiers (*.*)"));
//		if ( !fileName.isEmpty() )
//		{
//			QFile file(fileName);
//			if( file.open(QIODevice::WriteOnly) )
//			{
//				QTextStream stream(&file);
//				stream << "P3D" << endl;
//				stream << "#!VoxelDim " << _knotBillon->voxelWidth() << ' ' << _knotBillon->voxelHeight() << ' ' << _knotBillon->voxelDepth() << endl;
//				stream << _knotBillon->n_cols << " " << _knotBillon->n_rows << " " << _knotBillon->n_slices << endl;
//				stream << 1 << endl;

//				QDataStream dstream(&file);
//				for ( uint k=0 ; k<_knotBillon->n_slices ; ++k )
//				{
//					SliceAlgorithm::writeInPgm3D( _knotBillon->slice(k) , dstream );
//				}

//				file.close();
//				QMessageBox::information(this,tr("Exporter le nœud courant segmenté en PGM3D"), tr("Export réussi !"));
//			}
//			else QMessageBox::warning(this,tr("Exporter le nœud courant segmenté en PGM3D"),tr("L'export a échoué : impossible de créer le ficher %1.").arg(fileName));
//		}
//	}
//	else QMessageBox::warning(this,tr("Exporter le nœud courant segmenté en PGM3D"),tr("L'export a échoué : le contour n'est pas calculé."));
//}

//void MainWindow::exportSegmentedKnotsOfCurrentSliceIntervalToPgm3d()
//{
//	if ( _billon && _billon->hasPith() && _ui->_comboSelectSliceInterval->currentIndex() )
//	{
//		QString fileName = QFileDialog::getSaveFileName(this, tr("Exporter la zone de nœuds courante en PGM3D"), "output.pgm3d", tr("Fichiers de données (*.pgm3d);;Tous les fichiers (*.*)"));
//		if ( !fileName.isEmpty() )
//		{
//			QFile file(fileName);
//			if( file.open(QIODevice::WriteOnly) )
//			{
//				Billon billonToWrite( _billon->n_cols, _billon->n_rows, _sliceHistogram->interval(_ui->_comboSelectSliceInterval->currentIndex()-1).size()+1 );
//				billonToWrite.fill(0);

//				QTextStream stream(&file);
//				stream << "P3D" << endl;
//				stream << "#!VoxelDim " << _billon->voxelWidth() << ' ' << _billon->voxelHeight() << ' ' << _billon->voxelDepth() << endl;
//				stream << billonToWrite.n_cols << " " << billonToWrite.n_rows << " " << billonToWrite.n_slices << endl;
//				stream << _sectorHistogram->size() << endl;

//				uint i, j, k;
//				for ( int sectorIndex=1 ; sectorIndex< _ui->_comboSelectSectorInterval->count() ; ++sectorIndex )
//				{
//					selectSectorInterval(sectorIndex,false);
//					if ( _knotBillon )
//					{
//						for ( k=0 ; k<_knotBillon->n_slices ; ++k )
//						{
//							const Slice &knotSlice = _knotBillon->slice(k);
//							Slice &slice = billonToWrite.slice(k);
//							for ( j=0 ; j<slice.n_rows ; ++j )
//							{
//								for ( i=0 ; i<slice.n_cols ; ++i )
//								{
//									if (knotSlice.at(j,i)) slice.at(j,i) = sectorIndex;
//								}
//							}
//						}

//					}
//				}

//				QDataStream dstream(&file);
//				for ( k=0 ; k<billonToWrite.n_slices ; ++k )
//				{
//					Slice &slice = billonToWrite.slice(k);
//					for ( j=0 ; j<slice.n_rows ; ++j )
//					{
//						for ( i=0 ; i<slice.n_cols ; ++i )
//						{
//							dstream << (qint16)(slice.at(j,i));
//						}
//					}
//				}

//				file.close();
//				QMessageBox::information(this,tr("Exporter la zone de nœuds courante en PGM3D"), tr("Export réussi !"));
//			}
//			else QMessageBox::warning(this,tr("Exporter le nœud courant segmenté en PGM3D"),tr("L'export a échoué : impossible de créer le ficher %1.").arg(fileName));
//		}
//	}
//	else QMessageBox::warning(this,tr("Exporter de la zone de nœuds courante en PGM3D"),tr("L'export a échoué : aucun intervalle angulaire sélectionné."));
//}


//void MainWindow::exportCurrentSegmentedKnotToV3D()
//{
//	if ( _knotBillon )
//	{
//		QString fileName = QFileDialog::getSaveFileName(this, tr("Exporter le nœud courant segmenté en V3D"), "output.v3d", tr("Fichiers V3D (*.v3d);;Tous les fichiers (*.*)"));
//		if ( !fileName.isEmpty() )
//		{
//			QFile file(fileName);
//			if( file.open(QIODevice::WriteOnly) )
//			{
//				V3DExport::process( file, *_knotBillon );
//				file.close();

//				QMessageBox::information(this,tr("Exporter le nœud courant segmenté en V3D"), tr("Export réussi !"));
//			}
//			else QMessageBox::warning(this,tr("Exporter le nœud courant segmenté en V3D"),tr("L'export a échoué : impossible de créer le ficher %1.").arg(fileName));
//		}
//	}
//	else QMessageBox::warning(this,tr("Exporter le nœud courant segmenté en V3D"),tr("L'export a échoué : le nœud n'est pas segmenté."));
//}

//void MainWindow::exportSegmentedKnotsOfCurrentSliceIntervalToV3D()
//{
//	if ( _billon && _billon->hasPith() && _ui->_comboSelectSliceInterval->currentIndex() )
//	{
//		QString fileName = QFileDialog::getSaveFileName(this, tr("Exporter les nœuds segmentés de l'intervalle de coupe courant en V3D"), "output.v3d",
//														tr("Fichiers V3D (*.v3d);;Tous les fichiers (*.*)"));
//		if ( !fileName.isEmpty() )
//		{
//			QFile file(fileName);
//			if ( file.open(QIODevice::WriteOnly) )
//			{
//				QXmlStreamWriter stream(&file);
//				V3DExport::init(file,stream);

//				V3DExport::appendTags(stream,*_billon );

//				int sectorIndex;

//				V3DExport::startComponents(stream);
//				for ( sectorIndex=1 ; sectorIndex< _ui->_comboSelectSectorInterval->count() ; ++sectorIndex )
//				{
//					selectSectorInterval(sectorIndex,false);
//					if ( _componentBillon && _knotBillon )
//					{
//						V3DExport::appendComponent( stream, *_knotBillon, _knotBillon->zPos()-_componentBillon->zPos(), sectorIndex );
//					}
//				}
//				V3DExport::endComponents(stream);

//				if (_componentBillon) V3DExport::appendPith(stream,*_componentBillon, 0 );
//				else V3DExport::appendPith(stream,*_billon, -_sliceHistogram->interval(_ui->_comboSelectSliceInterval->currentIndex()-1).min() );

//				V3DExport::close(stream);

//				file.close();

//				QMessageBox::information(this,tr("Exporter les nœuds segmentés de l'intervalle de coupe courant en V3D"), tr("Export réussi !"));
//			}
//			else QMessageBox::warning(this,tr("Exporter les nœuds segmentés de l'intervalle de coupe courant en V3D"),tr("L'export a échoué : impossible de créer le ficher %1.").arg(fileName));
//		}
//	}
//	else QMessageBox::warning(this,tr("Exporter les nœuds segmentés de l'intervalle de coupe courant en V3D"),tr("L'export a échoué : aucun intervalle de coupes sélectionné."));
//}

//void MainWindow::exportAllSegmentedKnotsOfBillonToV3D()
//{
//	if ( _billon && _billon->hasPith() )
//	{
//		QString fileName = QFileDialog::getSaveFileName(this, tr("Exporter tous les nœuds segmentés du billon en V3D"), "output.v3d", tr("Fichiers V3D (*.v3d);;Tous les fichiers (*.*)"));
//		if ( !fileName.isEmpty() )
//		{
//			QFile file(fileName);
//			if ( file.open(QIODevice::WriteOnly) )
//			{
//				QXmlStreamWriter stream(&file);
//				V3DExport::init(file,stream);

//				V3DExport::appendTags(stream,*_billon );
//				V3DExport::appendPith(stream,*_billon, 0 );

//				int intervalIndex, sectorIndex, counter;

//				V3DExport::startComponents(stream);
//				counter = 1;
//				for ( intervalIndex=1 ; intervalIndex< _ui->_comboSelectSliceInterval->count() ; ++intervalIndex )
//				{
//					_ui->_comboSelectSliceInterval->setCurrentIndex(intervalIndex);
//					for ( sectorIndex=1 ; sectorIndex< _ui->_comboSelectSectorInterval->count() ; ++sectorIndex )
//					{
//						selectSectorInterval(sectorIndex,false);
//						if ( _knotBillon )
//						{
//							V3DExport::appendComponent( stream, *_knotBillon, _knotBillon->zPos(), counter++ );
//						}
//					}
//				}
//				V3DExport::endComponents(stream);

//				V3DExport::close(stream);

//				file.close();

//				QMessageBox::information(this,tr("Exporter tous les nœuds segmentés du billon en V3D"), tr("Export réussi !"));
//			}
//			else QMessageBox::warning(this,tr("Exporter tous les nœuds segmentés du billon en V3D"),tr("L'export a échoué : impossible de créer le ficher %1.").arg(fileName));
//		}
//	}
//	else QMessageBox::warning(this,tr("Exporter tous les nœuds segmentés du billon en V3D"),tr("L'export a échoué : la moelle n'est pas calculée."));
//}


//void MainWindow::exportCurrentSegmentedKnotToSdp()
//{
//	if ( _knotBillon )
//	{
//		QString fileName = QFileDialog::getSaveFileName(this, tr("Exporter le nœud courant segmenté en SDP"), "output.sdp", tr("Fichiers PGM3D (*.sdp);;Tous les fichiers (*.*)"));
//		if ( !fileName.isEmpty() )
//		{
//			QFile file(fileName);
//			if ( file.open(QIODevice::WriteOnly) )
//			{
//				QTextStream stream(&file);
//				stream << "#SDP (Sequence of Discrete Points)" << endl;

//				for ( uint k=0 ; k<_knotBillon->n_slices ; ++k )
//				{
//					SliceAlgorithm::writeInSDP( _knotBillon->slice(k) , stream, k, 0 );
//				}

//				file.close();

//				QMessageBox::information(this,tr("Exporter le nœud segmenté en SDP"), tr("Export réussi !"));
//				QMessageBox::information(this,"Exporter le nœud courant segmenté en SDP", "Export réussi !");
//			}
//			else QMessageBox::warning(this,tr("Exporter le nœud courant segmenté en SDP"),tr("L'export a échoué : impossible de créer le ficher %1.").arg(fileName));
//		}
//	}
//	else QMessageBox::warning(this,tr("Exporter le nœud courant segmenté en SDP"),tr("L'export a échoué : le nœud n'est pas segmenté."));
//}

//void MainWindow::exportSegmentedKnotsOfCurrentSliceIntervalToSdp( bool keepBillonSliceNumber )
//{
//	if ( _billon && _billon->hasPith() )
//	{
//		QString fileName = QFileDialog::getSaveFileName(this, tr("Exporter les nœuds segmentés de l'intervalle de coupe courant en SDP"), "output.sdp",
//														tr("Fichiers SDP (*.sdp);;Tous les fichiers (*.*)"));
//		if ( !fileName.isEmpty() )
//		{
//			QFile file(fileName);
//			if ( file.open(QIODevice::WriteOnly) )
//			{
//				QTextStream stream(&file);
//				stream << "#SDP (Sequence of Discrete Points)" << endl;

//				const bool coeffSliceNumber = !keepBillonSliceNumber;
//				int sectorIndex;
//				uint k;

//				for ( sectorIndex=1 ; sectorIndex< _ui->_comboSelectSectorInterval->count() ; ++sectorIndex )
//				{
//					selectSectorInterval(sectorIndex,false);
//					if ( _knotBillon )
//					{
//						for ( k=0 ; k<_knotBillon->n_slices ; ++k )
//						{
//							SliceAlgorithm::writeInSDP( _knotBillon->slice(k) , stream, _knotBillon->zPos()+k-_componentBillon->zPos()*coeffSliceNumber, 0 );
//						}
//					}
//				}

//				file.close();

//				QMessageBox::information(this,tr("Exporter les nœuds segmentés de l'intervalle de coupe courant en SDP"), tr("Export réussi !"));
//			}
//			else QMessageBox::warning(this,tr("Exporter les nœuds segmentés de l'intervalle de coupe courant en SDP"),tr("L'export a échoué : impossible de créer le ficher %1.").arg(fileName));
//		}
//	}
//	else QMessageBox::warning(this,tr("Exporter les nœuds segmentés de l'intervalle de coupe courant en SDP"),tr("L'export a échoué : le nœud n'est pas segmenté."));
//}



//void MainWindow::exportSegmentedKnotsOfCurrentSliceIntervalToSdpOldAlgo( bool keepBillonSliceNumber )
//{
//	if ( _billon && _billon->hasPith() )
//	  {
//		QString fileName = QFileDialog::getSaveFileName(this, tr("Exporter les nœuds segmentés de l'intervalle de coupe courant en SDP (old version)"), "output.sdp",
//														tr("Fichiers SDP (*.sdp);;Tous les fichiers (*.*)"));
//		if ( !fileName.isEmpty() )
//		{
//			QFile file(fileName);
//			if ( file.open(QIODevice::WriteOnly) )
//			{
//				QTextStream stream(&file);
//				stream << "#SDP (Sequence of Discrete Points)" << endl;

//				const uint width = _billon->n_cols;
//				const uint height = _billon->n_rows;
//				const bool coeffSliceNumber = !keepBillonSliceNumber;

//				int sectorIndex;
//				uint i,j,k;

//				for ( sectorIndex=1 ; sectorIndex< _ui->_comboSelectSectorInterval->count() ; ++sectorIndex )
//				{
//					selectSectorInterval(sectorIndex,false);
//					if ( _knotBillon )
//					{
//						for ( k=0 ; k<_knotBillon->n_slices ; ++k )
//						{
//							const Slice &componentSlice = _componentBillon->slice(_knotBillon->zPos()-_componentBillon->zPos()+k);
//							for ( j=0 ; j<height ; ++j )
//							{
//								for ( i=0 ; i<width ; ++i )
//								{
//									if ( componentSlice.at(j,i) )
//									{
//										stream << i << " "<< j << " " <<  _knotBillon->zPos()+k-_componentBillon->zPos()*coeffSliceNumber << endl ;
//									}
//								}
//							}
//						}
//					}
//				}

//				file.close();

//				QMessageBox::information(this,tr("Exporter les nœuds segmentés de l'intervalle de coupe courant en SDP"), tr("Export réussi !"));
//			}
//			else QMessageBox::warning(this,tr("Exporter les nœuds segmentés de l'intervalle de coupe courant en SDP"),tr("L'export a échoué : impossible de créer le ficher %1.").arg(fileName));
//		}
//	}
//	else QMessageBox::warning(this,tr("Exporter les nœuds segmentés de l'intervalle de coupe courant en SDP"),tr("L'export a échoué : le nœud n'est pas segmenté."));
//}




//void MainWindow::exportAllSegmentedKnotsOfBillonToSdp()
//{
//	if ( _billon && _billon->hasPith() )
//	{
//		QString fileName = QFileDialog::getSaveFileName(this, tr("Exporter tous les nœuds segmentés du billon en SDP"), "output.sdp", tr("Fichiers SDP (*.sdp);;Tous les fichiers (*.*)"));
//		if ( !fileName.isEmpty() )
//		{
//			QFile file(fileName);
//			if ( file.open(QIODevice::WriteOnly) )
//			{
//				QTextStream stream(&file);
//				stream << "#SDP (Sequence of Discrete Points)" << endl;

//				int intervalIndex, sectorIndex;
//				uint k;

//				for ( intervalIndex=1 ; intervalIndex< _ui->_comboSelectSliceInterval->count() ; ++intervalIndex )
//				{
//					_ui->_comboSelectSliceInterval->setCurrentIndex(intervalIndex);
//					for ( sectorIndex=1 ; sectorIndex< _ui->_comboSelectSectorInterval->count() ; ++sectorIndex )
//					{
//						selectSectorInterval(sectorIndex,false);
//						if ( _knotBillon )
//						{
//							for ( k=0 ; k<_knotBillon->n_slices ; ++k )
//							{
//								SliceAlgorithm::writeInSDP( _knotBillon->slice(k) , stream, _knotBillon->zPos()+k, 0 );
//							}
//						}
//					}
//				}

//				file.close();

//				QMessageBox::information(this,tr("Exporter tous les nœuds segmentés du billon en SDP"), tr("Export réussi !"));
//			}
//			else QMessageBox::warning(this,tr("Exporter tous les nœuds segmentés du billon en SDP"),tr("L'export a échoué : impossible de créer le ficher %1.").arg(fileName));
//		}
//	}
//	else QMessageBox::warning(this,tr("Exporter tous les nœuds segmentés du billon en SDP"),tr("L'export a échoué : la moelle n'est pas calculée."));
//}


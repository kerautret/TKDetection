#include "inc/mainwindow.h"

#include "ui_mainwindow.h"
#include "inc/billon.h"
#include "inc/sliceview.h"
#include "inc/dicomreader.h"
#include "inc/slicehistogram.h"
#include "inc/marrowextractor.h"
#include "inc/pie_def.h"
#include "inc/piepart.h"
#include "inc/piechart.h"
#include "inc/piecharthistograms.h"

#include <iostream>

#include <QFileDialog>
#include <QMouseEvent>
#include <QPainter>

MainWindow::MainWindow( QWidget *parent ) : QMainWindow(parent), _ui(new Ui::MainWindow), _billon(0), _sliceView(new SliceView()), _sliceHistogram(0), _marrow(0), _pieChart(new PieChart(0,1)), _pieChartHistograms(new PieChartHistograms()) {
	_ui->setupUi(this);

	// Initialisation des vues
	_sliceHistogram = new SliceHistogram(_ui->_plotSliceHistogram);

	// Paramétrisation des composant graphiques
	_ui->_labelSliceView->installEventFilter(this);

	_ui->_sliderSelectSlice->setSingleStep(1);
	_ui->_sliderSelectSlice->setPageStep(10);
	_ui->_sliderSelectSlice->setTickInterval(1);
	_ui->_sliderSelectSlice->setTickPosition(QSlider::TicksAbove);

	_ui->_plotSliceHistogram->enableAxis(QwtPlot::yLeft,false);

	_histogramCursor.attach(_ui->_plotSliceHistogram);
	_histogramCursor.setBrush(Qt::red);
	_histogramCursor.setPen(QPen(Qt::red));

	_groupSliceView.addButton(_ui->_radioOriginalSlice,SliceType::CURRENT);
	_groupSliceView.addButton(_ui->_radioAverageSlice,SliceType::AVERAGE);
	_groupSliceView.addButton(_ui->_radioMedianSlice,SliceType::MEDIAN);
	_groupSliceView.setExclusive(true);

	// Raccourcis des actions du menu
	_ui->_actionOpenDicom->setShortcut(Qt::CTRL + Qt::Key_O);
	_ui->_actionCloseImage->setShortcut(Qt::CTRL + Qt::Key_W);
	_ui->_actionQuit->setShortcut(Qt::CTRL + Qt::Key_Q);

	/**** Mise en place de la communication MVC ****/

	// Évènements déclenchés par le slider de n° de coupe
	QObject::connect(_ui->_sliderSelectSlice, SIGNAL(valueChanged(int)), _sliceView, SLOT(drawSlice(int)));
	QObject::connect(_ui->_sliderSelectSlice, SIGNAL(valueChanged(int)), _ui->_labelSliceNumber, SLOT(setNum(int)));
	QObject::connect(_ui->_sliderSelectSlice, SIGNAL(valueChanged(int)), this, SLOT(highlightSliceHistogram(int)));

	// Évènements déclenchés par les boutons de sélection de la vue
	QObject::connect(&_groupSliceView, SIGNAL(buttonClicked(int)), _sliceView, SLOT(setTypeOfView(int)));

	// Évènements déclenchés par le slider de seuillage
	QObject::connect(_ui->_spansliderSliceThreshold, SIGNAL(lowerValueChanged(int)), _sliceView, SLOT(setLowThreshold(int)));
	QObject::connect(_ui->_spansliderSliceThreshold, SIGNAL(lowerValueChanged(int)), _sliceHistogram, SLOT(setLowThreshold(int)));
	QObject::connect(_ui->_spansliderSliceThreshold, SIGNAL(lowerValueChanged(int)), _pieChartHistograms, SLOT(setLowThreshold(int)));
	QObject::connect(_ui->_spansliderSliceThreshold, SIGNAL(lowerValueChanged(int)), _ui->_labelMinThreshold, SLOT(setNum(int)));
	QObject::connect(_ui->_spansliderSliceThreshold, SIGNAL(upperValueChanged(int)), _sliceView, SLOT(setHighThreshold(int)));
	QObject::connect(_ui->_spansliderSliceThreshold, SIGNAL(upperValueChanged(int)), _sliceHistogram, SLOT(setHighThreshold(int)));
	QObject::connect(_ui->_spansliderSliceThreshold, SIGNAL(upperValueChanged(int)), _pieChartHistograms, SLOT(setHighThreshold(int)));
	QObject::connect(_ui->_spansliderSliceThreshold, SIGNAL(upperValueChanged(int)), _ui->_labelMaxThreshold, SLOT(setNum(int)));

	// Évènements déclenchés par le bouton de mise à jour de l'histogramme
	QObject::connect(_ui->_buttonUpdateHistogram, SIGNAL(clicked()), _sliceHistogram, SLOT(constructHistogram()));

	// Évènements déclenchés par les bouton associès à la moelle
	QObject::connect(_ui->_buttonComputeMarrow, SIGNAL(clicked()), this, SLOT(computeNewMarrow()));
	QObject::connect(_ui->_checkDrawMarrow, SIGNAL(toggled(bool)), _sliceView, SLOT(drawMarrow(bool)));

	// Évènements reçus de la vue en coupe
	QObject::connect(_sliceView, SIGNAL(updated(QPixmap)), _ui->_labelSliceView, SLOT(setPixmap(QPixmap)));
	QObject::connect(_sliceView, SIGNAL(typeOfViewChanged(SliceType::SliceType)), this, SLOT(adaptGraphicsComponentsToSliceType(SliceType::SliceType)));

	// Évènements reçus de la vue histogramme
	QObject::connect(_sliceHistogram, SIGNAL(histogramUpdated()), this, SLOT(redrawSliceHistogram()));

	// Évènements déclenchés par les bouton associès aux histogrammes de secteurs
	QObject::connect(_ui->_buttonUpdateSectors, SIGNAL(clicked()), this, SLOT(updateSectorsHistograms()));
	QObject::connect(_ui->_comboSelectSector, SIGNAL(currentIndexChanged(int)), _ui->_stackedSectorsHistograms, SLOT(setCurrentIndex(int)));
	QObject::connect(_ui->_comboSelectSector, SIGNAL(currentIndexChanged(int)), this, SLOT(drawCurrentSector()));

	// Évènements déclenchés par les classes relatives aux secteirs angulaires
	QObject::connect(_pieChartHistograms, SIGNAL(histogramsUpdated()), this, SLOT(updateSectorsHistogramsView()));

	// Évènements déclenchés par les actions du menu
	QObject::connect(_ui->_actionOpenDicom, SIGNAL(triggered()), this, SLOT(openDicom()));
	QObject::connect(_ui->_actionCloseImage, SIGNAL(triggered()), this, SLOT(closeImage()));
	QObject::connect(_ui->_actionQuit, SIGNAL(triggered()), this, SLOT(close()));

	_ui->_radioOriginalSlice->click();
}

MainWindow::~MainWindow() {
	if ( _pieChart != 0 ) delete _pieChart;
	if ( _pieChartHistograms != 0 ) delete _pieChartHistograms;
	while ( !_pieChartPlots.isEmpty() ) {
		_pieChartPlots.removeLast();
	}
	if ( _marrow != 0 ) delete _marrow;
	if ( _sliceHistogram != 0 ) delete _sliceHistogram;
	if ( _sliceView != 0 ) delete _sliceView;
	if ( _billon != 0 ) delete _billon;
	if ( _ui != 0 ) delete _ui;
}

/*******************************
 * Public fonctions
 *******************************/

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
	bool res = true;
	if (obj == _ui->_labelSliceView) {
		if ( _billon != 0 && event->type() == QEvent::MouseButtonPress ) {
			QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
			const int x = mouseEvent->x();
			const int y = mouseEvent->y();
			const int sector = _pieChart->partOfAngle( TWO_PI-ANGLE(256,256,x,y) );

			if ( _pieChartHistograms != 0 && _ui->_comboSelectSector->count() > sector) {
				_ui->_comboSelectSector->setCurrentIndex(sector);
			}
		}
		else {
			res = false;
		}
	 }
	else {
		 res = QMainWindow::eventFilter(obj, event);
	 }
	return res;
}

/*******************************
 * Private slots
 *******************************/

void MainWindow::openDicom() {
	QString folderName = QFileDialog::getExistingDirectory(0,tr("Sélection du répertoire DICOM"),QDir::homePath(),QFileDialog::ShowDirsOnly);
	if ( !folderName.isEmpty() ) {
		// Lecture des fichiers DICOM
		openNewBillon(folderName);
		updateGraphicsComponentsValues();
	}
}

void MainWindow::closeImage() {
	openNewBillon();
	computeNewMarrow();
	updateSectorsHistogramsView();
	updateGraphicsComponentsValues();
}

void MainWindow::adaptGraphicsComponentsToSliceType(const SliceType::SliceType &type) {
	switch (type) {
		case SliceType::AVERAGE :
		case SliceType::MEDIAN :
			_ui->_sliderSelectSlice->setEnabled(false);
			_ui->_spansliderSliceThreshold->setEnabled(false);
			_ui->_buttonComputeMarrow->setEnabled(false);
			_ui->_buttonUpdateHistogram->setEnabled(false);
			_ui->_checkDrawMarrow->setEnabled(false);
			break;
		case SliceType::CURRENT :
		default :
			const bool existBillon = _billon != 0;
			_ui->_sliderSelectSlice->setEnabled(existBillon);
			_ui->_spansliderSliceThreshold->setEnabled(existBillon);
			_ui->_buttonComputeMarrow->setEnabled(existBillon);
			_ui->_buttonUpdateHistogram->setEnabled(existBillon);
			_ui->_checkDrawMarrow->setEnabled(existBillon);
	}
}

void MainWindow::computeNewMarrow() {
	if ( _marrow != 0 ) {
		delete _marrow;
		_marrow = 0;
	}
	if ( _billon != 0 ) {
		MarrowExtractor extractor;
		_marrow = extractor.process(*_billon,0,_billon->n_slices-1);
	}
	_sliceView->setModel(_marrow);
}

void MainWindow::redrawSliceHistogram() {
	_ui->_plotSliceHistogram->setAxisScale(QwtPlot::xBottom,0,(_billon != 0)?_billon->n_slices:0);
	_ui->_plotSliceHistogram->replot();
	highlightSliceHistogram(_ui->_sliderSelectSlice->value());
}


void MainWindow::highlightSliceHistogram( const int &slicePosition ) {
	double height = _sliceHistogram->sample(slicePosition).value;
	double x[4] = {slicePosition,slicePosition, slicePosition+1,slicePosition+1};
	double y[4] = {0,height,height,0};
	_histogramCursor.setSamples(x,y,4);
	_ui->_plotSliceHistogram->replot();
}

void MainWindow::updateSectorsHistograms() {
	_pieChart->setOrientation(_ui->_spinSectorsOrientation->value());
	_pieChart->setSectorsNumber(_ui->_spinSectorsNumber->value());

	_pieChartHistograms->setModel(_pieChart);
	_pieChartHistograms->setModel(_billon);
	_pieChartHistograms->setBillonInterval(_ui->_sliderSelectSlice->value(),_ui->_sliderSelectSlice->value());
	_pieChartHistograms->computeHistograms();
}

void MainWindow::updateSectorsHistogramsView() {
	while ( !_pieChartPlots.isEmpty() ) {
		_ui->_stackedSectorsHistograms->removeWidget(_pieChartPlots.last());
		_pieChartPlots.removeLast();
	}

	_ui->_comboSelectSector->clear();

	const int nbHistograms = _pieChartHistograms->count();
	if ( nbHistograms != 0 ) {

		_pieChartPlots.reserve(nbHistograms);
		for ( int i=0 ; i<nbHistograms ; ++i ) {
			QwtPlot * plot = new QwtPlot();
			_pieChartPlots.append(plot);
			_ui->_stackedSectorsHistograms->addWidget(plot);
			_ui->_comboSelectSector->addItem(tr("Secteur %1").arg(i));
		}

		_pieChartHistograms->attach(_pieChartPlots);

		for ( int i=0 ; i<nbHistograms ; ++i ) {
			_pieChartPlots[i]->replot();
		}

		_ui->_labelSectorsOrientation->setNum(_ui->_spinSectorsOrientation->value());
		_ui->_labelSectorsNumber->setNum(_ui->_spinSectorsNumber->value());
		_ui->_stackedSectorsHistograms->setCurrentIndex(0);
	}
	else {
		_ui->_labelSectorsOrientation->setText(tr("Aucune"));
		_ui->_labelSectorsNumber->setText(tr("Aucun"));
	}

}

void MainWindow::drawCurrentSector() {

	QList<PiePart> sectors = _pieChart->sectors();
	QPixmap pix(*(_sliceView->pixmap()));
	QPainter painter(&pix);

	const int sectorIdx = qMax(0,_ui->_comboSelectSector->currentIndex());
	const int width = pix.width();
	const int height = pix.height();

	painter.setPen(QColor(0,255,0));
	const double rightAngle = TWO_PI-sectors.at(sectorIdx).rightAngle();
	if ( rightAngle != PI/2. && rightAngle != 3.*PI/2. ) {
		const double a_r = tan(rightAngle);
		const double b_r = 256 - (a_r*256);
		if ( rightAngle < PI/2. || rightAngle > 3.*PI/2. ) {
			painter.drawLine(256,256,width,a_r*width+b_r);
		}
		else {
			painter.drawLine(0,b_r,256,256);
		}
	}
	else {
		if ( rightAngle == PI/2. ) {
			painter.drawLine(256,256,256,height);
		}
		else if ( rightAngle == 3.*PI/2. ) {
			painter.drawLine(256,0,256,256);
		}
	}

	painter.setPen(QColor(0,255,0));
	const double leftAngle = TWO_PI-sectors.at(sectorIdx).leftAngle();
	if ( leftAngle != PI/2. && leftAngle != 3.*PI/2. ) {
		const double a_l = tan(leftAngle);
		const double b_l = 256 - (a_l*256);
		if ( leftAngle < PI/2. || leftAngle > 3.*PI/2. ) {
			painter.drawLine(256,256,width,a_l*width+b_l);
		}
		else {
			painter.drawLine(0,b_l,256,256);
		}
	}
	else {
		if ( leftAngle == PI/2. ) {
			painter.drawLine(256,256,256,height);
		}
		else if ( leftAngle == 3.*PI/2. ) {
			painter.drawLine(256,0,256,256);
		}
	}

	_ui->_labelSliceView->setPixmap(pix);
}

/*******************************
 * Private functions
 *******************************/

void MainWindow::updateGraphicsComponentsValues() {
	int minValue, maxValue, nbSlices;
	bool enable = true;
	if ( _billon != 0 ) {
		minValue = _billon->minValue();
		maxValue = _billon->maxValue();
		nbSlices = _billon->n_slices;
	}
	else {
		minValue = maxValue = 0;
		nbSlices = 1;
		enable = false;
	}

	_ui->_buttonComputeMarrow->setEnabled(enable);

	_ui->_spansliderSliceThreshold->setMinimum(minValue);
	_ui->_spansliderSliceThreshold->setLowerValue(minValue);
	_ui->_spansliderSliceThreshold->setMaximum(maxValue);
	_ui->_spansliderSliceThreshold->setUpperValue(maxValue);

	_ui->_sliderSelectSlice->setValue(0);
	_ui->_sliderSelectSlice->setRange(0,nbSlices-1);

	if ( _billon != 0 )	_ui->_labelSliceNumber->setNum(0);
	else _ui->_labelSliceNumber->setText(tr("Aucune coupe présente."));

	adaptGraphicsComponentsToSliceType(_sliceView->sliceType());
}

void MainWindow::openNewBillon( const QString &folderName ) {
	if ( _billon != 0 ) {
		delete _billon;
		_billon = 0;
	}
	if ( !folderName.isEmpty() ) {
		_billon = DicomReader::read(folderName);
	}
	_sliceView->setModel(_billon);
	_sliceHistogram->setModel(_billon);
	_pieChartHistograms->setModel(_billon);
}

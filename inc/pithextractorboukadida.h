#ifndef PITHEXTRACTORBOUKADIDA_H
#define PITHEXTRACTORBOUKADIDA_H

#include "def/def_billon.h"
#include "def/def_coordinate.h"
#include "inc/define.h"

#include <armadillo>

class Pith;

class PithExtractorBoukadida {

public:

	PithExtractorBoukadida( const int &subWindowWidth = NEIGHBORHOOD_WINDOW_WIDTH, const int &subWindowHeight = NEIGHBORHOOD_WINDOW_HEIGHT,
							const qreal &pithShift = PITH_LAG, const uint smoothingRadius = PITH_SMOOTHING );

	~PithExtractorBoukadida();

	/**
	 * \fn		Pith* process( const icube &image, const int &sliceMin, const int &sliceMax )
	 * \brief	Extrait la moelle d'une matrice sous forme d'une liste des coordonnées des voxels qui la compose.
	 *          Cette classe implémente l'algorithme proposé par Boukadida et al., 2012.
	 * \param	image Image 2D
	 * \param	sliceMin Première coupe à traiter
	 * \param	sliceMax Dernière coupe à traiter
	 */
	void process( Billon &billon ) const;

	/********************************************************
	 * Get functions
	 ********************************************************/
	/**
	 * \fn		int subWindowWidth()
	 * \return	la largeur de la fenêtre du voisinage de la moelle
	 */
	int subWindowWidth() const;

	/**
	 * \fn		int subWindowHeight()
	 * \return	la hauteur de la fenêtre du voisinage de la moelle
	 */
	int subWindowHeight() const;

	/**
	 * \fn		int pithShift()
	 * \return	Décalage autorisé de la position de la moelle entre deux coupes consécutives
	 */
	qreal pithShift() const;

	/**
	 * \fn		uint smoothingRadius()
	 * \return	Rayon de lissage de la moelle (en nombre de coupes)
	 */
	uint smoothingRadius() const;

	/********************************************************
	 * Set functions
	 ********************************************************/
	/**
	 * \fn		void setSubWindowWidth( int width )
	 * \param	width Largeur de la fenêtre du voisinage de la moelle
	 */
	void setSubWindowWidth( const int &width );

	/**
	 * \fn		void setSubWindowHeight( int height )
	 * \param	height Hauteur de la fenêtre du voisinage de la moelle
	 */
	void setSubWindowHeight( const int &height );

	/**
	 * \fn		void setPithShift( const uint &shift )
	 * \param	shift Décalage autorisé de la position de la moelle entre deux coupes consécutives
	 */
	void setPithShift( const qreal &shift );

	/**
	 * \fn		void setSmoothingRadius( const uint &radius )
	 * \param	radius Rayon de lissage de la moelle (en nombre de coupes)
	 */
	void setSmoothingRadius( const uint &radius );

private:
	/**
	 * \fn		Coord transHough(const Slice &slice, uint &nbContourPoints );
	 * \brief	Transformée de Hough sur une coupe
	 * \param	slice Coupe à traiter
	 * \param   nbContourPoints nombre de points utilisés pour tracer les droites
	 * \return	les coordonnées de la moelle pour la coupe traitée
	 */
	uiCoord2D transHough( const Slice &slice, qreal & lineOnMaxRatio ) const;

	/**
	 * \fn		uint contour( const Slice &slice, arma::Mat<qreal> &orientations )
	 * \brief	Calcul de l'orientation en chaque pixel de slice avec les filtres de Sobel
	 * \param	slice Coupe dont on veut les orientations
	 * \param	orientations Coupe contenant l'orientation en chaque pixel (pour les pixels d'orientation supérieure à l'orientation médiane)
	 * \param   hasContour Matrice de booleen qui précise pour chaque pixel de Slice s'il est un point de contour
	 * \return	le nombre d'orientations prises en compte
	 */
	uint contour(const Slice &slice, arma::Mat<qreal> &orientations, arma::Mat<char> & hasContour ) const;

	void drawLine( arma::Mat<int> &slice, const uiCoord2D &origin, const qreal &orientationOrig ) const;



private:
	int _subWindowWidth;	/*!< Largeur de la fenêtre de voisinage */
	int _subWindowHeight;	/*!< Hauteur de la fenêtre de voisinage */
	qreal _pithShift;		/*!< Décalage autorisé de la moelle entre deux coupes consécutives */
	uint _smoothingRadius;	/*!< Rayon de lissage de la moelle (en nombre de coupes) */
};

#endif // PITHEXTRACTORBOUKADIDA_H
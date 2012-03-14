#ifndef SLICEVIEW_DEF_H
#define SLICEVIEW_DEF_H

// Types de coupe possibles
namespace SliceType {
	enum SliceType {
		_SLICE_TYPE_MIN_ = -1,
		CURRENT,
		AVERAGE,
		MEDIAN,
		MOVEMENT,
		FLOW,
		_SLICE_TYPE_MAX_
	};
}

#endif // SLICEVIEW_DEF_H

#include "inc/marrowextractor_def.h"

namespace MarrowExtractorDefaultsParameters {
	int FALSE_CUT_PERCENT = 20;
	int NEIGHBORHOOD_WINDOW_WIDTH = 59;
	int NEIGHBORHOOD_WINDOW_HEIGHT = NEIGHBORHOOD_WINDOW_WIDTH;
	int BINARIZATION_THRESHOLD = 90;
	int MARROW_LAG = NEIGHBORHOOD_WINDOW_WIDTH/2;
}
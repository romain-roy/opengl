#include <CImg.h>

#include "texture.h"

Image LoadImage(const char *filename)
{
	cimg_library::CImg<unsigned char> im(filename);

	std::vector<unsigned char> picture;
	cimg_forXY(im, x, y) {
		picture.push_back(im(x, im.height() - y - 1, 0, 0));
		picture.push_back(im(x, im.height() - y - 1, 0, 1));
		picture.push_back(im(x, im.height() - y - 1, 0, 2));
	}

	return {picture, im.width(), im.height()};
}

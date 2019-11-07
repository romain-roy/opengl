#pragma once
#include <vector>
#include <tuple>

struct Image
{
	std::vector<unsigned char> data;
	int width, height;
};

Image LoadImage(const char *filename);

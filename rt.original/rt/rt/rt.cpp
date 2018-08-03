/**
	The program below generates 512 spheres of different radiuses and renders
	and image of this set of spheres using ray tracing for orthographic camera:
	It generates a single ray for each image plane pixel and finds closest 
	intersection of this ray with a set of spheres. If the intersection is found 
	the program sets pixel color to the color of the sphere. If there are no 
	intersections the color is set to RGB=(0.1 0.1 0.1)
	
	On 2.4GHz processor the program takes ~15-16s to generate and image.
	The task is to optimize the program as much as possible. Make sure it generates 
	the same output.
*/

#include <iostream>
#include <chrono>
#include <cstdlib>
#include <cstdint>

#include "oiio/include/OpenImageIO/imageio.h"

struct ray
{
	// Origin
	float ox, oy, oz;
	// Direction
	float dx, dy, dz;
	// Intersection distance
	float maxt;
};


struct sphere
{
	// Center
	float cx, cy, cz;
	// Radius
	float radius;
	// Shpere color
	float r, g, b;
};

// Output image dimensions
std::uint32_t const kImageWidth = 2048;
std::uint32_t const kImageHeight = 2048;
// Number of spheres to render
std::uint32_t const kNumSpheres = 512;

#define RAND_FLOAT (((float)std::rand()) / RAND_MAX)
#define LEFT -10.f
#define BOTTOM -10.f
#define WIDTH 20.f
#define HEIGHT 20.f
#define NEAR -10.f
#define FAR 10.f

// Randomly generate sphere array of num_spheres items
void generate_spheres(sphere* spheres, std::uint32_t num_spheres)
{
	std::srand(0x88e8fff4);

	for (auto i = 0U; i < num_spheres; ++i)
	{
		spheres[i].cx = RAND_FLOAT * 20.f - 10.f;
		spheres[i].cy = RAND_FLOAT * 20.f - 10.f;
		spheres[i].cz = RAND_FLOAT * 20.f - 5.f;
		spheres[i].radius = (RAND_FLOAT + 0.1f) * 1.5f;
		spheres[i].r = RAND_FLOAT;
		spheres[i].g = RAND_FLOAT;
		spheres[i].b = RAND_FLOAT;
	}
}

// Solve quadratic equations and return roots if exist
// Returns true if roots exist and are returned in x1 and x2
// Returns false if no roots exist and x1 and x2 are undefined
bool solve_quadratic(float a, float b, float c, float& x1, float& x2)
{
	float d = b*b - 4 * a*c;

	if (d < 0)
		return false;
	else
	{
		float den = 1 / (2 * a);
		x1 = (-b - std::sqrt(d))*den;
		x2 = (-b + std::sqrt(d))*den;
		return true;
	}
}

// Intersect the ray against the sphere
// If there is an intersection:
// * return true
// * update r.maxt to intersection distance
bool intersect_sphere(sphere const& sphere, ray& r)
{
	ray rtemp = r;
	rtemp.ox -= sphere.cx;
	rtemp.oy -= sphere.cy;
	rtemp.oz -= sphere.cz;

	float a = rtemp.dx * rtemp.dx + rtemp.dy * rtemp.dy + rtemp.dz * rtemp.dz;
	float b = 2 * (rtemp.ox * rtemp.dx + rtemp.oy * rtemp.dy + rtemp.oz * rtemp.dz);
	float c = rtemp.ox * rtemp.ox + rtemp.oy * rtemp.oy + rtemp.oz * rtemp.oz - sphere.radius * sphere.radius;

	float t0, t1;

	if (solve_quadratic(a, b, c, t0, t1))
	{
		if (t0 > r.maxt || t1 < 0.f)
			return false;

		r.maxt = t0 > 0.f ? t0 : t1;
		return true;
	}

	return false;
}


// Render the image img usign ray tracing for ortho projection camera.
// Each pixel of img contains color of closest sphere after the function has finished.
void trace(sphere const* spheres, std::uint32_t num_spheres, float* img)
{
	for (auto i = 0U; i < kImageWidth; ++i)
	{
		for (auto j = 0U; j < kImageHeight; ++j)
		{
			ray r;
			r.oz = NEAR;
			r.ox = LEFT + (WIDTH / kImageWidth) * (i + 0.5f);
			r.oy = BOTTOM + (HEIGHT / kImageHeight) * (j + 0.5f);
			r.dx = r.dy = 0.f;
			r.dz = 1.f;
			r.maxt = FAR - NEAR;

			int idx = -1;

			for (auto k = 0U; k < num_spheres; ++k)
			{
				if (intersect_sphere(spheres[k], r))
				{
					idx = k;
				}
			}

			if (idx > 0)
			{
				img[(j * kImageWidth + i) * 3] = spheres[idx].r;
				img[(j * kImageWidth + i) * 3 + 1] = spheres[idx].g;
				img[(j * kImageWidth + i) * 3 + 2] = spheres[idx].b;
			}
			else
			{
				img[(j * kImageWidth + i) * 3] = 0.1f;
				img[(j * kImageWidth + i) * 3 + 1] = 0.1f;
				img[(j * kImageWidth + i) * 3 + 2] = 0.1f;
			}
		}
	}
}

int main()
{
	std::vector<sphere> spheres(kNumSpheres);

	generate_spheres(&spheres[0], kNumSpheres);

	std::vector<float> img(kImageWidth * kImageHeight * 3);

	auto start = std::chrono::high_resolution_clock::now();

	trace(&spheres[0], kNumSpheres, &img[0]);

	auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count();

	std::cout << "Execution time " << delta << " ms\n";

	OIIO_NAMESPACE_USING;

	ImageOutput* out = ImageOutput::create("result.png");

	if (!out)
	{
		std::cout << "Can't create image file on disk";
		return -1;
	}

	ImageSpec spec(kImageWidth, kImageHeight, 3, TypeDesc::FLOAT);

	out->open("result.png", spec);
	out->write_image(TypeDesc::FLOAT, &img[0], sizeof(float) * 3);
	out->close();

	return 0;
}

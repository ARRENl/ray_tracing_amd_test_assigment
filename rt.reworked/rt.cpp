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

#include "oiio/include/OpenImageIO/imageio.h"

#include <iostream>
#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <fstream>
#include <string>

#include <CL/cl.hpp>

// Output image dimensions
std::uint32_t const kImageWidth = 2048;
std::uint32_t const kImageHeight = 2048;
// Number of spheres to render
std::uint32_t const kNumSpheres = 512;


#define RT_LEFT -10.f
#define RT_BOTTOM -10.f
#define RT_WIDTH 20.f
#define RT_HEIGHT 20.f
#define RT_NEAR -10.f
#define RT_FAR 10.f

struct ray
{
	// Origin
	float ox, oy, oz;
	// Direction
	float dx, dy, dz;
	// Intersection distance
	float maxt;
};

class sphere
{
private:
	float rand_float()
	{
		return static_cast <float> ( std::rand() ) / static_cast <float> (RAND_MAX);
	}

public:
	// Center
	float cx, cy, cz;
	// Radius
	float radius;
	// Shpere color
	float r, g, b;

	sphere()
	{
		cx = rand_float() * 20.f - 10.f;
		cy = rand_float() * 20.f - 10.f;
		cz = rand_float() * 20.f - 5.f;
		radius = (rand_float() + 0.1f) * 1.5f;
		r = rand_float();
		g = rand_float();
		b = rand_float();
	}
};

// Randomly generate sphere array of num_spheres items
void generate_spheres( std::vector<sphere>& spheres )
{
	for (size_t i = 0; i < spheres.capacity(); ++i)
	{
		spheres.emplace_back();
	}
}

int main()
{
	cl_int err = 0;

	//init platform
	std::vector<cl::Platform> all_platforms;
	cl::Platform::get(&all_platforms);
	if (all_platforms.size() == 0) 
	{
		std::cout << " No platforms found. Check OpenCL installation!\n";
		exit(1);
	}
	cl::Platform default_platform = all_platforms.front();
	std::cout << "Using platform: " << default_platform.getInfo<CL_PLATFORM_NAME>() << "\n";

	//init device
	std::vector<cl::Device> devices;
	default_platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
	cl::Device device = devices.front();

	//create programm
	std::ifstream trace_file("trace.cl");
	std::string src( std::istreambuf_iterator<char>(trace_file), (std::istreambuf_iterator<char>()));

	cl::Program::Sources sources(1, std::make_pair(src.c_str(), src.length() + 1));

	cl::Context context(device);
	cl::Program program(context, sources);

	err =  program.build("-cl-std=CL1.2");

	//
	cl::Kernel kernel(program, "trace", &err);

	//init data
	std::srand(0x88e8fff4);

	std::vector<sphere> spheres;
	spheres.reserve(kNumSpheres);

	generate_spheres( spheres );

	std::vector<float> img(kImageWidth * kImageHeight * 3);

	//init buffers
	cl::Buffer in_buf(context, CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS | CL_MEM_COPY_HOST_PTR, sizeof(sphere) * spheres.size(), spheres.data(), &err);
	cl::Buffer out_buf(context, CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY, sizeof(float) * img.size(), img.data(), &err);

	err = kernel.setArg(0, in_buf);
	err = kernel.setArg(1, out_buf);

	//
	cl::CommandQueue queue(context, device);

	//
	auto start = std::chrono::high_resolution_clock::now();

	//
	err = queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(kImageWidth, kImageHeight));
	err = queue.enqueueReadBuffer(out_buf, CL_FALSE, 0, sizeof(float) * img.size(), img.data());
	
	err = cl::finish();

	auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count();

	std::cout << "Execution time " << delta << " ms\n";

	OIIO_NAMESPACE::ImageOutput* out = OIIO_NAMESPACE::ImageOutput::create("result.png");

	if (!out)
	{
		std::cout << "Can't create image file on disk";
		return -1;
	}

	OIIO_NAMESPACE::ImageSpec spec(kImageWidth, kImageHeight, 3, OIIO_NAMESPACE::TypeDesc::FLOAT);

	out->open("result.png", spec);
	out->write_image(OIIO_NAMESPACE::TypeDesc::FLOAT, &img[0], sizeof(float) * 3);
	out->close();

	//system("pause");

	return 0;
}

#define RT_LEFT -10.f
#define RT_BOTTOM -10.f
#define RT_WIDTH 20.f
#define RT_HEIGHT 20.f
#define RT_NEAR -10.f
#define RT_FAR 10.f

// Output image dimensions
#define kImageWidth 2048
#define kImageHeight 2048
// Number of spheres to render
#define kNumSpheres 512

typedef struct tag_sphere
{
	// Center
	float cx, cy, cz;
	// Radius
	float radius;
	// Shpere color
	float r, g, b;
} sphere;

typedef struct tag_ray
{
	// Origin
	float ox, oy, oz;
	// Direction
	float dx, dy, dz;
	// Intersection distance
	float maxt;
} ray;

// Render the image img usign ray tracing for ortho projection camera.
// Each pixel of img contains color of closest sphere after the function has finished.
__kernel
void trace(__global sphere* spheres, __global float* img)
{
	size_t gid0 = get_global_id(0);
	size_t gid1 = get_global_id(1);
	size_t gsz = get_global_size(0);
	size_t id = (gid1 * gsz) + gid0;
	
	ray r;
	r.oz = RT_NEAR;
	r.ox = RT_LEFT + (RT_WIDTH / kImageWidth) * (gid0 + 0.5f);
	r.oy = RT_BOTTOM + (RT_HEIGHT / kImageHeight) * (gid1 + 0.5f);
	r.dx = r.dy = 0.f;
	r.dz = 1.f;
	r.maxt = RT_FAR - RT_NEAR;

	int idx = -1;
	
	for (size_t k = 0U; k < kNumSpheres; ++k)
	{	
		bool is_intersect = false;
		bool is_found_roots = false;

		ray rtemp = r;
		rtemp.ox -= spheres[k].cx;
		rtemp.oy -= spheres[k].cy;
		rtemp.oz -= spheres[k].cz;

		float a = rtemp.dx * rtemp.dx + rtemp.dy * rtemp.dy + rtemp.dz * rtemp.dz;
		float b = 2 * (rtemp.ox * rtemp.dx + rtemp.oy * rtemp.dy + rtemp.oz * rtemp.dz);
		float c = (rtemp.ox * rtemp.ox) + (rtemp.oy * rtemp.oy) + (rtemp.oz * rtemp.oz) - (spheres[k].radius * spheres[k].radius);

		float t0, t1;

		float d = (b*b) - (4 * a*c);

		if (d >= 0)
		{
			float den = 1 / (2 * a);
			t0 = (-b - sqrt(d))*den;
			t1 = (-b + sqrt(d))*den;
			is_found_roots = true;
		}

		if (is_found_roots && t0 <= r.maxt && t1 >= 0.f)
		{
			r.maxt = t0 > 0.f ? t0 : t1;
			is_intersect = true;
		}

		if (is_intersect)
		{
			idx = k;
		}
	}

	if (idx >= 0)
	{
		img[id * 3] = spheres[idx].r;
		img[id * 3 + 1] = spheres[idx].g;
		img[id * 3 + 2] = spheres[idx].b;
	}
	else
	{
		img[id * 3] = 0.1f;
		img[id * 3 + 1] = 0.1f;
		img[id * 3 + 2] = 0.1f;
	}
}
#ifdef _WIN32
#define STBR_ASSERT(x) \
	if (!(x)) \
		__debugbreak();
#endif

#define STB_RESAMPLE_IMPLEMENTATION
#define STB_RESAMPLE_STATIC
#include "stb_resample.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifdef _WIN32
#include <sys/timeb.h>
#endif

void test_suite();

int main(int argc, char** argv)
{
	unsigned char* input_data;
	unsigned char* output_data;
	int w, h;
	int n;
	int out_w, out_h, out_stride;

#if 1
	test_suite();
	return 0;
#endif

	if (argc <= 1)
	{
		printf("No input image\n");
		return 1;
	}

	input_data = stbi_load(argv[1], &w, &h, &n, 0);
	if (!input_data)
	{
		printf("Input image could not be loaded");
		return 1;
	}

	out_w = 256;
	out_h = 256;
	out_stride = (out_w + 10) * n;

	output_data = (unsigned char*)malloc(out_stride * out_h);

	int in_w = 512;
	int in_h = 512;

	size_t memory_required = stbr_calculate_memory(in_w, in_h, out_w, out_h, n, STBR_FILTER_CATMULLROM);
	void* extra_memory = malloc(memory_required);

	// Cut out the outside 64 pixels all around to test the stride.
	int border = 64;
	STBR_ASSERT(in_w + border <= w);
	STBR_ASSERT(in_h + border <= h);

#ifdef PERF_TEST
	struct timeb initial_time_millis, final_time_millis;

	long average = 0;
	for (int j = 0; j < 10; j++)
	{
		ftime(&initial_time_millis);
		for (int i = 0; i < 100; i++)
			stbr_resize_arbitrary(input_data + w * border * n + border * n, in_w, in_h, w*n, output_data, out_w, out_h, out_stride, n, STBR_TYPE_UINT8, STBR_FILTER_CATMULLROM, STBR_EDGE_CLAMP, STBR_COLORSPACE_SRGB, extra_memory, memory_required);
		ftime(&final_time_millis);
		long lapsed_ms = (long)(final_time_millis.time - initial_time_millis.time) * 1000 + (final_time_millis.millitm - initial_time_millis.millitm);
		printf("Resample: %dms\n", lapsed_ms);

		average += lapsed_ms;
	}

	average /= 10;

	printf("Average: %dms\n", average);
#else
	stbr_resize_arbitrary(input_data + w * border * n + border * n, in_w, in_h, w*n, output_data, out_w, out_h, out_stride, n, STBR_TYPE_UINT8, STBR_FILTER_CATMULLROM, STBR_EDGE_CLAMP, STBR_COLORSPACE_SRGB, extra_memory, memory_required);
#endif

	free(extra_memory);

	stbi_write_png("output.png", out_w, out_h, n, output_data, out_stride);

	free(output_data);

	return 0;
}

void resize_image(const char* filename, float width_percent, float height_percent, stbr_filter filter, stbr_edge edge, const char* output_filename)
{
	int w, h, n;

	unsigned char* input_data = stbi_load(filename, &w, &h, &n, 0);
	if (!input_data)
	{
		printf("Input image could not be loaded");
		return;
	}

	int out_w = (int)(w * width_percent);
	int out_h = (int)(h * height_percent);

	unsigned char* output_data = (unsigned char*)malloc(out_w * out_h * n);

	size_t memory_required = stbr_calculate_memory(w, h, out_w, out_h, n, filter);
	void* extra_memory = malloc(memory_required);

	stbr_resize_arbitrary(input_data, w, h, 0, output_data, out_w, out_h, 0, n, STBR_TYPE_UINT8, filter, edge, STBR_COLORSPACE_SRGB, extra_memory, memory_required);

	free(extra_memory);

	stbi_write_png(output_filename, out_w, out_h, n, output_data, 0);

	free(output_data);
}

void test_suite()
{
	// sRGB tests
	resize_image("gamma_colors.jpg", .5f, .5f, STBR_FILTER_CATMULLROM, STBR_EDGE_REFLECT, "test-output/gamma_colors.jpg");
	resize_image("gamma_2.2.jpg", .5f, .5f, STBR_FILTER_CATMULLROM, STBR_EDGE_REFLECT, "test-output/gamma_2.2.jpg");
	resize_image("gamma_dalai_lama_gray.jpg", .5f, .5f, STBR_FILTER_CATMULLROM, STBR_EDGE_REFLECT, "test-output/gamma_dalai_lama_gray.jpg");

	for (int i = 10; i < 100; i++)
	{
		char outname[200];
		sprintf(outname, "test-output/barbara-width-%d.jpg", i);
		resize_image("barbara.png", (float)i / 100, 1, STBR_FILTER_CATMULLROM, STBR_EDGE_CLAMP, outname);
	}

	for (int i = 110; i < 500; i += 10)
	{
		char outname[200];
		sprintf(outname, "test-output/barbara-width-%d.jpg", i);
		resize_image("barbara.png", (float)i / 100, 1, STBR_FILTER_CATMULLROM, STBR_EDGE_CLAMP, outname);
	}

	for (int i = 10; i < 100; i++)
	{
		char outname[200];
		sprintf(outname, "test-output/barbara-height-%d.jpg", i);
		resize_image("barbara.png", 1, (float)i / 100, STBR_FILTER_CATMULLROM, STBR_EDGE_CLAMP, outname);
	}

	for (int i = 110; i < 500; i += 10)
	{
		char outname[200];
		sprintf(outname, "test-output/barbara-height-%d.jpg", i);
		resize_image("barbara.png", 1, (float)i / 100, STBR_FILTER_CATMULLROM, STBR_EDGE_CLAMP, outname);
	}

	for (int i = 50; i < 200; i += 10)
	{
		char outname[200];
		sprintf(outname, "test-output/barbara-width-height-%d.jpg", i);
		resize_image("barbara.png", 100 / (float)i, (float)i / 100, STBR_FILTER_CATMULLROM, STBR_EDGE_CLAMP, outname);
	}
}



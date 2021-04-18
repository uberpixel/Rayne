#include <stdlib.h>
#include <stdio.h>
#include <png.h>
#include <string.h>

int width, height;
png_byte color_type;
png_byte bit_depth;
double gAMA;
unsigned short **row_pointers = NULL;
bool isSRGB = false;
int premultiplyAlphaMode = 0;

void read_png_file(char *filename)
{
	FILE *fp = fopen(filename, "rb");

	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!png) abort();

	png_infop info = png_create_info_struct(png);
	if(!info) abort();

	if(setjmp(png_jmpbuf(png))) abort();

	png_init_io(png, fp);
	png_read_info(png, info);

	width      = png_get_image_width(png, info);
	height     = png_get_image_height(png, info);
	color_type = png_get_color_type(png, info);
	bit_depth  = png_get_bit_depth(png, info);
	png_get_gAMA(png, info, &gAMA);

	if(gAMA == 0.0)
	{
		gAMA = 0.454550;
	}

	if(color_type == PNG_COLOR_TYPE_PALETTE)
	{
		color_type = PNG_COLOR_TYPE_RGB;
	}

	png_set_expand_16(png);
	png_read_update_info(png, info);

	if(row_pointers) abort();

	row_pointers = (unsigned short **)malloc(sizeof(unsigned short*) * height);
	for(int y = 0; y < height; y++)
	{
		row_pointers[y] = (unsigned short *)malloc(png_get_rowbytes(png, info));
	}

	png_read_image(png, (png_bytep*)row_pointers);

	fclose(fp);

	png_destroy_read_struct(&png, &info, NULL);
}

void write_png_file(char *filename)
{
	int y;

	FILE *fp = fopen(filename, "wb");
	if(!fp) abort();

	png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!png) abort();

	png_infop info = png_create_info_struct(png);
	if(!info) abort();

	if(setjmp(png_jmpbuf(png))) abort();

	png_init_io(png, fp);

	if(isSRGB)
	{
		png_set_gAMA(png, info, gAMA);
	}

	// Output is 8bit depth, RGBA format.
	png_set_IHDR(
		png,
		info,
		width, height,
		16,
		color_type,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT
	);
	png_write_info(png, info);

	if(!row_pointers) abort();

	png_set_swap(png);

	png_write_image(png, (png_bytep*)row_pointers);
	png_write_end(png, NULL);

	fclose(fp);

	png_destroy_write_struct(&png, &info);
}

float gamma_to_linear(float value)
{
	/*if(value <= 0.04045)
	{
		return value / 12.92;
	}
	else
	{
		return pow((value + 0.055) / 1.055, 2.4);
	}*/

	return pow(value, gAMA);
}
		
float linear_to_gamma(float value)
{
	/*if(value <= 0.0031308)
	{
		return value * 12.92;
	}
	else
	{
		return 1.055 * pow(value, 1.0/2.4) - 0.055;
	}*/

	return pow(value, 1.0/gAMA);
}

void blend_pixels(unsigned short *px00, unsigned short *px01, unsigned short *px10, unsigned short *px11, unsigned short *out)
{
	float a0 = 1.0;
	float a1 = 1.0;
	float a2 = 1.0;
	float a3 = 1.0;
	float alpha = 1.0;
	if(color_type == PNG_COLOR_TYPE_GRAY_ALPHA || color_type == PNG_COLOR_TYPE_RGB_ALPHA)
	{
		int alphaIndex = color_type == PNG_COLOR_TYPE_RGB_ALPHA? 3:1;

		a0 = (float)px00[alphaIndex]/65535.0;
		a1 = (float)px01[alphaIndex]/65535.0;
		a2 = (float)px10[alphaIndex]/65535.0;
		a3 = (float)px11[alphaIndex]/65535.0;

		alpha = a0 + a1 + a2 + a3;
		alpha *= 0.25;

		out[alphaIndex] = alpha * 65535;
	}

	int numberOfColorChannels = 1;
	if(color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_RGB_ALPHA)
	{
		numberOfColorChannels = 3;
	}

	for(int i = 0; i < numberOfColorChannels; i++)
	{
		float c0 = (float)px00[i]/65535.0;
		float c1 = (float)px01[i]/65535.0;
		float c2 = (float)px10[i]/65535.0;
		float c3 = (float)px11[i]/65535.0;

		if(isSRGB)
		{
			c0 = linear_to_gamma(c0);
			c1 = linear_to_gamma(c1);
			c2 = linear_to_gamma(c2);
			c3 = linear_to_gamma(c3);
		}

		if(premultiplyAlphaMode > 0)
		{
			c0 *= a0;
			c1 *= a1;
			c2 *= a2;
			c3 *= a3;
		}

		float value = c0 + c1 + c2 + c3;
		value *= 0.25;

		if(premultiplyAlphaMode > 1)
		{
			value /= alpha;
		}

		if(isSRGB)
		{
			value = gamma_to_linear(value);
		}

		out[i] = value * 65535;
	}
}

void process_png_file()
{
	width /= 2;
	height /= 2;
	if(width < 1) width = 1;
	if(height < 1) height = 1;

	int numberOfChannels = 1;
	if(color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
	{
		numberOfChannels = 2;
	}
	else if(color_type == PNG_COLOR_TYPE_RGB)
	{
		numberOfChannels = 3;
	}
	else if(color_type == PNG_COLOR_TYPE_RGB_ALPHA)
	{
		numberOfChannels = 4;
	}

	for(int y = 0; y < height; y++)
	{
		unsigned short *rowOut = row_pointers[y];
		unsigned short *row0 = row_pointers[y * 2 + 0];
		unsigned short *row1 = row_pointers[y * 2 + 1];
		for(int x = 0; x < width; x++)
		{
			unsigned short *pxOut = &(rowOut[x * numberOfChannels]);
			unsigned short *px00 = pxOut;
			unsigned short *px01 = pxOut;
			unsigned short *px10 = pxOut;
			unsigned short *px11 = pxOut;

			if(width == 1)
			{
				px00 = &(row0[(x * 2 + 0) * numberOfChannels]);
				px01 = &(row0[(x * 2 + 0) * numberOfChannels]);
				px10 = &(row1[(x * 2 + 0) * numberOfChannels]);
				px11 = &(row1[(x * 2 + 0) * numberOfChannels]);
			}
			else if(height == 1)
			{
				px00 = &(row0[(x * 2 + 0) * numberOfChannels]);
				px01 = &(row0[(x * 2 + 1) * numberOfChannels]);
				px10 = &(row0[(x * 2 + 0) * numberOfChannels]);
				px11 = &(row0[(x * 2 + 1) * numberOfChannels]);
			}
			else
			{
				px00 = &(row0[(x * 2 + 0) * numberOfChannels]);
				px01 = &(row0[(x * 2 + 1) * numberOfChannels]);
				px10 = &(row1[(x * 2 + 0) * numberOfChannels]);
				px11 = &(row1[(x * 2 + 1) * numberOfChannels]);
			}
			
			blend_pixels(px00, px01, px10, px11, pxOut);
		}
	}
}

int main(int argc, char *argv[])
{
	if(argc < 4) abort();

	if(argc >= 5)
	{
		if(strcmp(argv[4], "-srgb") == 0)
		{
			isSRGB = true;
		}
		else if(strcmp(argv[4], "-premul") == 0)
		{
			premultiplyAlphaMode = 1;
		}
		else if(strcmp(argv[4], "-premulblend") == 0)
		{
			premultiplyAlphaMode = 2;
		}
	}

	if(argc >= 6)
	{
		if(strcmp(argv[5], "-srgb") == 0)
		{
			isSRGB = true;
		}
		else if(strcmp(argv[5], "-premul") == 0)
		{
			premultiplyAlphaMode = 1;
		}
		else if(strcmp(argv[5], "-premulblend") == 0)
		{
			premultiplyAlphaMode = 2;
		}
	}

	int mipMapCount = atoi(argv[3]);
	char *imageFileName = (char*)calloc(strlen(argv[2])-3 + 1, 1);
	strncpy(imageFileName, argv[2], strlen(argv[2])-3);

	char *imageIndexString = (char*)calloc(5, 1);

	char *imageFullFileName = (char*)calloc(strlen(argv[2]) + 10, 1);
	strcpy(imageFullFileName, imageFileName);
	strcat(imageFullFileName, "0.png");

	read_png_file(argv[1]);
	write_png_file(imageFullFileName);

	for(int i = 1; i < mipMapCount; i++)
	{
		strcpy(imageFullFileName, imageFileName);
		memset(imageIndexString, 0, 5);
		sprintf(imageIndexString, "%d", i);
		strcat(imageFullFileName, imageIndexString);
		strcat(imageFullFileName, ".png");

		process_png_file();
		write_png_file(imageFullFileName);
	}

	for(int y = 0; y < height; y++)
	{
		free(row_pointers[y]);
	}
	free(row_pointers);

	return 0;
}

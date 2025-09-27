/*
 * main.c
 *
 *  Created on: 11 сент. 2025 г.
 *      Author: Andrew
 */

#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "libbmp.h"

#include "math.h"

#include "font.h"

#include "cJSON.h"

#define COLOR_SKY 0x000000
#define COLOR_STAR 0xFF0000
#define COLOR_STAR_FILL 0x0000FF
#define COLOR_STAR_CENTER 0xffffff

#define BMP_OUTPUT_SIDE_MARGIN_PIXELS 15

const char* helpString =
"\nSyntax: starbound *file_name* *intensity_threshold* *star_size_min*\n\n"\
"*file_name* - path to BMP picture\n"\
"*intensity_threshold* - minumum brightness for star pixel (0.0...1.0). Can also take a value relative\n"\
"to the average brightness of picture. In this case, symbol 'x' should be added, e.g. 6.0x.\n"\
"*star_size_min* - minimum star size in pixels.\n\n"\
"Examples:\n"\
"starbound sky.bmp 0.1 3 - absolute threshold 0.1, minumum star size is 3px\n"
"starbound sky.bmp 10.0x 12 - relative threshold 10x, minumum star size is 12px\n";

typedef struct
{
	int row;
	int col;

	int intensity;
	float intensity_normalized;
	unsigned int processed;

	unsigned int color;
}Pixel_t;

typedef struct
{
	int centerRow;
	int centerCol;

	int boundColMin;
	int boundColMax;
	int boundRowMin;
	int boundRowMax;

	int sizeCol;
	int sizeRow;

	Pixel_t** pixels;
	int pixelCount;
}Star_t;

Star_t** found_stars;

bmp_img img;
Pixel_t* pixels;

int stars_count = 0;

void inline __attribute__((always_inline)) img_draw_pixel(bmp_img* img, int row, int col, unsigned int rgb)
{
	if(row < 0)
		return;

	if(row >= img->img_header.biHeight)
		return;

	if(col < 0)
		return;

	if(col >= img->img_header.biWidth)
		return;


	bmp_pixel* px = (img->img_pixels[row] + col);

	px->red =   (unsigned char)(rgb >> 16);
	px->green = (unsigned char)(rgb >> 8);
	px->blue =  (unsigned char)(rgb >> 0);
}



void img_draw_digit(bmp_img* img, int digit, int row, int col, unsigned int rgb);
void img_draw_number(bmp_img* img, int number, int row, int col, unsigned int rgb);
void img_draw_line(bmp_img* img, int row1, int col1, int row2, int col2, unsigned int rgb);

Pixel_t* strideToNextPixel(Pixel_t* curPixel);
int getFileNameWithoutExtension(char* in, char* out);

int main(int argc, char** argv)
{
	char filebase[100];
	char report_name[256];
	char json_report_name[256];
	char out_image_name[256];

	float intensity_threshold;

	int star_size_min = 0;


	cJSON* json_report;

	json_report = cJSON_CreateObject();



	printf("\nStarBound v0.2\n");
	fflush(stdout);


	if(argc != 4)
	{
		printf(helpString);
		fflush(stdout);
		return 1;
	}

	if(bmp_img_read(&img, argv[1]) != BMP_OK)
	{
		printf("Cannot open BMP file.\r\n");
		fflush(stdout);
		return 1;
	}

	//Intensity
	int relative_intensity = 1;

	if(!strstr(argv[2], "x") )
	{
		relative_intensity = 0;
	}

	sscanf(argv[2], "%f", &intensity_threshold);
	/////////////////////////////


	//Star size
	sscanf(argv[3], "%d", &star_size_min);

	if (star_size_min < 1)
	{
		printf("Invalid minimum star size.\r\n");
		fflush(stdout);
		return 1;
	}
	/////////////////////////////


	getFileNameWithoutExtension(argv[1], filebase);

	sprintf(report_name, "%s_report.txt", filebase);
	sprintf(json_report_name, "%s_report.json", filebase);
	sprintf(out_image_name, "%s_report.bmp", filebase);

	int pixelCount = img.img_header.biHeight * img.img_header.biWidth;

	pixels = (Pixel_t*)malloc( sizeof(Pixel_t) * pixelCount );

	int starBufferSize = 100;
	found_stars = (Star_t**)malloc(sizeof(Star_t**) * starBufferSize);

	if(!found_stars || !pixels)
	{
		printf("Cannot allocate memory.\r\n");
		fflush(stdout);
		return 1;
	}







	int row = 0;
	int col = 0;

	float average_intensity;

	average_intensity = 0;
	for(int i=0; i < pixelCount; ++i)
	{
		bmp_pixel* px;

		px = (img.img_pixels[row] + col);

		pixels[i].intensity = (px->red + px->green + px->blue);
		pixels[i].intensity_normalized = ((float)pixels[i].intensity / 765.0F);

		average_intensity += pixels[i].intensity_normalized;

		pixels[i].row = row;
		pixels[i].col = col;

		col++;
		if(col >= img.img_header.biWidth)
		{
			row++;
			col = 0;
		}
	}

	average_intensity /= pixelCount;

	if(relative_intensity)
	{
		intensity_threshold =  average_intensity * intensity_threshold;
	}

	for(int i=0; i < pixelCount; ++i)
	{
		if(pixels[i].intensity_normalized >= intensity_threshold)
		{
			pixels[i].color = COLOR_STAR; //Mark pixel as star pixel
		}
		else
		{
			pixels[i].color = COLOR_SKY; //Mark pixel as sky pixel
		}
	}


	stars_count = 0;
	for(int i = 0; i < (pixelCount); ++i)
	{
		if(pixels[i].processed)
		{
			continue;
		}

		if(pixels[i].color == COLOR_STAR)
		{
			Pixel_t* nextpix = &pixels[i];

			Star_t* newStar = (Star_t*)calloc(sizeof(Star_t), 1);

			int starPixelBufferSize = 100;
			newStar->pixels = (Pixel_t**)calloc(sizeof(Pixel_t**), starPixelBufferSize);

			Pixel_t** pixelStrideList;
			int pixelStrideCount = 0;

			int pixelStrideListSize = 100;

			pixelStrideList = (Pixel_t**)malloc(sizeof(Pixel_t**) * pixelStrideListSize);
			pixelStrideCount = 0;

			int starBoundRowMin = nextpix->row;
			int starBoundRowMax = nextpix->row;
			int starBoundColMin = nextpix->col;
			int starBoundColMax = nextpix->col;

			while(1)
			{
				pixelStrideList[pixelStrideCount++] = nextpix;

				if(pixelStrideCount >= pixelStrideListSize)
				{
					pixelStrideListSize += 100;
					pixelStrideList = (Pixel_t**)realloc(pixelStrideList, sizeof(Pixel_t**) * pixelStrideListSize);

					if(!pixelStrideList)
					{
						printf("Cannot allocate memory.\r\n");
						fflush(stdout);
						return 1;
					}
				}

				nextpix->color = COLOR_STAR_FILL;

				nextpix->processed = 0x01;
				newStar->pixels[newStar->pixelCount++] = nextpix;

				if(newStar->pixelCount >= starPixelBufferSize)
				{
					starPixelBufferSize += 100;
					newStar->pixels = (Pixel_t**)realloc(newStar->pixels, sizeof(Pixel_t**) * starPixelBufferSize);

					if(!newStar->pixels)
					{
						printf("Cannot allocate memory.\r\n");
						fflush(stdout);
						return 1;
					}
				}

				if (nextpix->row < starBoundRowMin)
					starBoundRowMin = nextpix->row;

				if (nextpix->row > starBoundRowMax)
					starBoundRowMax = nextpix->row;

				if (nextpix->col < starBoundColMin)
					starBoundColMin = nextpix->col;

				if (nextpix->col > starBoundColMax)
					starBoundColMax = nextpix->col;

				nextpix = strideToNextPixel(nextpix);

				while(!nextpix)  //dead end. Stride back.
				{
					pixelStrideCount--;

					if(pixelStrideCount < 1)
						break;

					nextpix = strideToNextPixel(pixelStrideList[pixelStrideCount - 1]);
				}

				if(!nextpix) //stride list is empty
				{
					free(pixelStrideList);
					pixelStrideList = NULL;
					break;
				}
			}

			if(newStar->pixelCount > 0)
			{
				if(newStar->pixelCount >= star_size_min) //Star size is bigger or equal to the min value
				{
					//Estimate star mass center coordinates
					int centerRow = 0;
					int centerCol = 0;
					for(int i = 0; i < newStar->pixelCount; ++i)
					{
						centerRow += newStar->pixels[i]->row;
						centerCol += newStar->pixels[i]->col;
					}

					newStar->centerCol = (int)roundf((float)centerCol / (float)newStar->pixelCount);
					newStar->centerRow = (int)roundf((float)centerRow / (float)newStar->pixelCount);

					//Estimate star size
					newStar->boundRowMin = starBoundRowMin;
					newStar->boundRowMax = starBoundRowMax;
					newStar->boundColMin = starBoundColMin;
					newStar->boundColMax = starBoundColMax;

					newStar->sizeCol = starBoundColMax - starBoundColMin + 1;
					newStar->sizeRow = starBoundRowMax - starBoundRowMin + 1;

					//Add star to list
					found_stars[stars_count++] = newStar;
				}
				else
				{
					free(newStar->pixels);
					free(newStar);
				}


				if(stars_count >= starBufferSize) //Increase list size
				{
					starBufferSize += 100;
					found_stars = (Star_t**)realloc((void*)found_stars, starBufferSize * sizeof(Star_t**));
				}
			}
		}
	}


	FILE* fReport;
	FILE* fJsonReport;

	fReport = fopen(report_name, "w");
	fJsonReport = fopen(json_report_name, "w");

	fprintf(fReport, "StarBound v0.2\nInput file: %s\nCoordinate system: Row-Column\nOrigin: Top-Left\nIntensity: 0.000...1.000\nAverage intensity: %1.3f\nIntensity threshold: %1.3f\n\nStars: %d\n", argv[1], average_intensity, intensity_threshold, stars_count);

	char tmp[200];

	cJSON_AddStringToObject(json_report, "Version", "0.2");
	cJSON_AddStringToObject(json_report, "InputFile", argv[1]);
	cJSON_AddStringToObject(json_report, "CoordinateSystem", "Row-Column");
	cJSON_AddStringToObject(json_report, "Origin", "Top-Left");

	cJSON* intRange = cJSON_AddArrayToObject(json_report,"IntensityRange");
	cJSON_AddItemToArray(intRange,cJSON_CreateString("0.000"));
	cJSON_AddItemToArray(intRange,cJSON_CreateString("1.000"));

	sprintf(tmp,"%1.3f",average_intensity);
	cJSON_AddStringToObject(json_report, "IntensityAverage", tmp);

	sprintf(tmp,"%1.3f",intensity_threshold);
	cJSON_AddStringToObject(json_report, "IntensityThreshold", tmp);

	//Add borders
	int dcl = stars_count;

	int margin_right = 0;

	while(dcl > 0)
	{
		margin_right += 8;
		dcl /= 10;
	}

	//create output bmp
	bmp_img bmp_output;
	bmp_img_init_df(&bmp_output, img.img_header.biWidth + BMP_OUTPUT_SIDE_MARGIN_PIXELS * 2 + margin_right,
								 img.img_header.biHeight + BMP_OUTPUT_SIDE_MARGIN_PIXELS * 2);



	//Fill borders
	for(int side = 0; side < 4; ++side)
	{
		int row1, col1, row2, col2;

		if(side == 0)
		{
			row1 = 0;
			row2 = img.img_header.biHeight + BMP_OUTPUT_SIDE_MARGIN_PIXELS*2 - 1;
			col1 = 0;
			col2 = BMP_OUTPUT_SIDE_MARGIN_PIXELS - 1;
		}
		else if(side == 1)
		{
			row1 = 0;
			row2 = BMP_OUTPUT_SIDE_MARGIN_PIXELS - 1;
			col1 = 0;
			col2 = img.img_header.biWidth + BMP_OUTPUT_SIDE_MARGIN_PIXELS * 2 + margin_right - 1;
		}
		else if(side == 2)
		{
			row1 = 0;
			row2 = img.img_header.biHeight + BMP_OUTPUT_SIDE_MARGIN_PIXELS*2 - 1;
			col1 = img.img_header.biWidth + BMP_OUTPUT_SIDE_MARGIN_PIXELS;
			col2 = img.img_header.biWidth + BMP_OUTPUT_SIDE_MARGIN_PIXELS * 2 + margin_right - 1;
		}
		else if(side == 3)
		{
			row1 = img.img_header.biHeight + BMP_OUTPUT_SIDE_MARGIN_PIXELS;
			row2 = img.img_header.biHeight + BMP_OUTPUT_SIDE_MARGIN_PIXELS * 2 - 1;
			col1 = 0;
			col2 = img.img_header.biWidth + BMP_OUTPUT_SIDE_MARGIN_PIXELS * 2 + margin_right - 1;
		}

		for(int row = row1; row <= row2; ++row)
		{
			bmp_pixel* px = bmp_output.img_pixels[row] + col1;

			for(int col = col1; col <= col2; ++col)
			{
				px->red  = 0;
				px->green = 0;
				px->blue = 0;

				px++;
			}
		}
	}


	for(int row=0; row<img.img_header.biHeight; ++row)
	{
		bmp_pixel* px = bmp_output.img_pixels[row + BMP_OUTPUT_SIDE_MARGIN_PIXELS] + BMP_OUTPUT_SIDE_MARGIN_PIXELS;

		for(int col=0; col<img.img_header.biWidth; ++col)
		{
			*(px++) = *(img.img_pixels[row] + col);
		}
	}

	for(int star=0; star<stars_count; ++star)
	{
		for(int pixel=0; pixel<found_stars[star]->pixelCount; ++pixel)
		{
			unsigned int r,g,b;

			r = (unsigned int)( (float)((found_stars[star]->pixels[pixel]->color >> 16) & 0xFF) * found_stars[star]->pixels[pixel]->intensity_normalized);
			g = (unsigned int)( (float)((found_stars[star]->pixels[pixel]->color >> 8) & 0xFF) *  found_stars[star]->pixels[pixel]->intensity_normalized);
			b = (unsigned int)( (float)((found_stars[star]->pixels[pixel]->color >> 0) & 0xFF) *  found_stars[star]->pixels[pixel]->intensity_normalized);

			img_draw_pixel(&bmp_output, BMP_OUTPUT_SIDE_MARGIN_PIXELS + found_stars[star]->pixels[pixel]->row,
									   BMP_OUTPUT_SIDE_MARGIN_PIXELS + found_stars[star]->pixels[pixel]->col,
									   (r << 16) | (g << 8) | b);
		}
	}


	sprintf(tmp, "%d", stars_count);
	cJSON_AddStringToObject(json_report, "Stars", tmp);

	cJSON* starArray = cJSON_AddObjectToObject(json_report,"StarArray");

	for(int star=0; star<stars_count; ++star)
	{
		fprintf(fReport, " Star #%d:\n", (star+1));
		fprintf(fReport, "  Mass center: [%d,%d]\n", found_stars[star]->centerRow, found_stars[star]->centerCol);
		fprintf(fReport, "  Pixels: %d\n", found_stars[star]->pixelCount);
		for(int pixel=0; pixel<found_stars[star]->pixelCount; ++pixel)
		{
			fprintf(fReport, "   Pixel %d:\t[%d,%d]\tIntensity: %1.3f\n", (pixel+1),
					found_stars[star]->pixels[pixel]->row,
					found_stars[star]->pixels[pixel]->col,
					found_stars[star]->pixels[pixel]->intensity_normalized);
		}

		fprintf(fReport, "\n");

		cJSON* json_star = cJSON_CreateObject();

		cJSON* tmpArr = cJSON_AddArrayToObject(json_star,"MassCenter");

		sprintf(tmp, "%d", found_stars[star]->centerRow);
		cJSON_AddItemToArray(tmpArr, cJSON_CreateString(tmp));

		sprintf(tmp, "%d", found_stars[star]->centerCol);
		cJSON_AddItemToArray(tmpArr, cJSON_CreateString(tmp));



		sprintf(tmp, "%d",  found_stars[star]->pixelCount);
		cJSON_AddStringToObject(json_star, "Pixels", tmp);


		cJSON* tmpObj = cJSON_AddObjectToObject(json_star,"PixelArray");

		for(int pixel=0; pixel<found_stars[star]->pixelCount; ++pixel)
		{
			sprintf(tmp, "%d", pixel + 1);
			cJSON* tmpArrPx = cJSON_AddArrayToObject(tmpObj, tmp);

			sprintf(tmp, "%d", found_stars[star]->pixels[pixel]->row);
			cJSON_AddItemToArray(tmpArrPx, cJSON_CreateString(tmp));

			sprintf(tmp, "%d", found_stars[star]->pixels[pixel]->col);
			cJSON_AddItemToArray(tmpArrPx, cJSON_CreateString(tmp));

			sprintf(tmp, "%1.3f", found_stars[star]->pixels[pixel]->intensity_normalized);
			cJSON_AddItemToArray(tmpArrPx, cJSON_CreateString(tmp));
		}

		sprintf(tmp, "%d", (star+1));
		cJSON_AddItemToObject(starArray, tmp, json_star);


		//Select minimal star size (row or col) for cross line
		int crossLineLen = (found_stars[star]->sizeCol > found_stars[star]->sizeRow)?found_stars[star]->sizeRow:found_stars[star]->sizeCol;

		//vertical line of cross
		img_draw_line(&bmp_output, BMP_OUTPUT_SIDE_MARGIN_PIXELS + found_stars[star]->centerRow - (crossLineLen / 2 + 2),
				BMP_OUTPUT_SIDE_MARGIN_PIXELS + found_stars[star]->centerCol,
				BMP_OUTPUT_SIDE_MARGIN_PIXELS + found_stars[star]->centerRow + (crossLineLen / 2 + 2)  + 1,
				BMP_OUTPUT_SIDE_MARGIN_PIXELS + found_stars[star]->centerCol,
					COLOR_STAR_CENTER);

		//horizontal line of cross
		img_draw_line(&bmp_output, BMP_OUTPUT_SIDE_MARGIN_PIXELS + found_stars[star]->centerRow,
				BMP_OUTPUT_SIDE_MARGIN_PIXELS + found_stars[star]->centerCol - (crossLineLen / 2 + 2),
				BMP_OUTPUT_SIDE_MARGIN_PIXELS + found_stars[star]->centerRow,
				BMP_OUTPUT_SIDE_MARGIN_PIXELS + found_stars[star]->centerCol + (crossLineLen / 2 + 2) + 1,
					COLOR_STAR_CENTER);

		img_draw_number(&bmp_output, (star+1),
				BMP_OUTPUT_SIDE_MARGIN_PIXELS + found_stars[star]->centerRow - 9,
				BMP_OUTPUT_SIDE_MARGIN_PIXELS + (found_stars[star]->centerCol + 2),
						COLOR_STAR_CENTER);
	}


	fprintf(fJsonReport, cJSON_Print(json_report));

	bmp_img_write(&bmp_output, out_image_name);


	printf("\nFinished parsing %s.\n", argv[1]);
	fflush(stdout);


	return 0;
}







void img_draw_digit(bmp_img* img, int digit, int row, int col, unsigned int rgb)
{
	int digitIdx = digit * 8;
	for(int c=0; c<8; ++c)
	{
		for(int r=0; r<8; ++r)
		{
			if( Standard8x8[digitIdx] & (0x01 << r)  )
			{
				img_draw_pixel(img, row + r, col + c, rgb);
			}
		}
		digitIdx++;
	}
}


void img_draw_number(bmp_img* img, int number, int row, int col, unsigned int rgb)
{
	char temp[40];

	snprintf(temp, (sizeof(temp)-1), "%d", number);

	for(int i=0; i<strlen(temp); ++i)
	{
		img_draw_digit(img,(temp[i]-48),row, col, rgb);
		col += 8;
	}
}



void img_draw_line(bmp_img* img, int row1, int col1, int row2, int col2, unsigned int rgb)
{
	int deltaRow = row2 - row1;
	int deltaCol = col2 - col1;

	int len = (int)(roundf(sqrtf((float)(deltaCol*deltaCol + deltaRow*deltaRow))));

	float stepRow, stepCol;

	float rowFloat, colFloat;
	int rowInt, colInt;

	stepRow = (float)deltaRow / (float)len;
	stepCol = (float)deltaCol / (float)len;

	rowFloat = (float)row1;
	colFloat = (float)col1;

	while(len--)
	{
		rowInt = (int)roundf(rowFloat);
		colInt = (int)roundf(colFloat);

		img_draw_pixel(img, rowInt, colInt, rgb);

		rowFloat += stepRow;
		colFloat += stepCol;
	}
}


//Select next pixel for outline
Pixel_t* strideToNextPixel(Pixel_t* curPixel)
{
	int nextRow, nextCol;
	Pixel_t* nextPix;

	//pixel stride directions
	static const int deltas[8][2] =
	{
		{0,1},   //Right
		{-1,1},  //Up-right
		{-1,0},  //Up
		{-1,-1}, //Up-left
		{0,-1},  //Left
		{1,-1},  //Down-left
		{1,0},   //Down
		{1,1},   //Down-right
	};

	nextPix = NULL;
	for(int i=0; i<8; ++i)
	{
		nextRow = curPixel->row + deltas[i][0];
		nextCol = curPixel->col + deltas[i][1];

		//Check for image borders
		if (nextRow >= img.img_header.biHeight)
			continue;
		if (nextCol >= img.img_header.biWidth)
			continue;
		if (nextRow < 0)
			continue;
		if (nextCol < 0)
			continue;

		nextPix = pixels + nextRow * img.img_header.biWidth + nextCol;

		if(nextPix->color == COLOR_STAR)
		{
			return nextPix;
		}
	}

	return NULL;
}

int getFileNameWithoutExtension(char* in, char* out)
{
	int len = strlen(in);

	int dotIdx;
	int slashIdx;

	int startIdx;
	int endIdx;

	dotIdx = -1;
	slashIdx = -1;

	for(int i = (len-1); i >= 0; --i)
	{
		if(in[i] == '.')
		{
			if(dotIdx == -1)
			{
				dotIdx = i;
			}
		}

		if((in[i] == '/') || (in[i] == '\\'))
		{
			if(slashIdx == -1)
			{
				slashIdx = i;
			}
		}
	}

	if(slashIdx != -1)
	{
		startIdx = slashIdx + 1;
	}
	else
	{
		startIdx = 0;
	}

	if(dotIdx != -1)
	{
		endIdx = dotIdx - 1;
	}
	else
	{
		endIdx = len-1;
	}


	int p = 0;
	for(int i = startIdx; i <= endIdx; ++i)
	{
		out[p++] = in[i];
	}

	out[p] = 0;

	return 0;
}




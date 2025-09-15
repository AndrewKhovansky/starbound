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

#define COLOR_SKY 0x000000
#define COLOR_STAR 0xFF0000
//#define COLOR_STAR_OUTLINE 0xffdc85
#define COLOR_STAR_OUTLINE 0x0000FF

//#define COLOR_STAR_CENTER 0x73c9ff
#define COLOR_STAR_CENTER 0xFFFFFF

typedef struct
{
	//int coords[2];

	int row;
	int col;

	//unsigned int R;
	//unsigned int G;
	//unsigned int B;

	int intensity;
	float intensity_normalized;
	unsigned int processed;


	unsigned int star;

	unsigned int yellowMark;

	unsigned int color;


	unsigned int sorted;

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
}star_t;


//star_t* found_stars[500];

star_t** found_stars;

int stars_count = 0;


char reportString[4096];


void set_img_pixel(bmp_img* img, int row, int col, unsigned int rgb)
{
	if(row < 0)
		return;

	if(row >= img->img_header.biHeight)
		return;


	if(col < 0)
		return;

	if(col >= img->img_header.biWidth)
		return;



	(img->img_pixels[row] + col)->red = (unsigned char)(rgb >> 16);
	(img->img_pixels[row] + col)->green = (unsigned char)(rgb >> 8);
	(img->img_pixels[row] + col)->blue = (unsigned char)(rgb >> 0);
}



void draw_digit(bmp_img* img, int digit, int row, int col, unsigned int rgb)
{
	int digitIdx = digit * 8;
	for(int c=0; c<8; ++c)
	{
		for(int r=0; r<8; ++r)
		{
			if( Standard8x8[digitIdx] & (0x01 << r)  )
			{
				set_img_pixel(img, row + r, col + c, rgb);
			}
		}
		digitIdx++;
	}
}


void draw_number(bmp_img* img, int number, int row, int col, unsigned int rgb)
{
	char temp[40];

	snprintf(temp, (sizeof(temp)-1), "%d", number);


	for(int i=0; i<strlen(temp); ++i)
	{
		draw_digit(img,(temp[i]-48),row, col, rgb);
		col += 8;
	}



}



void draw_line(bmp_img* img, int row1, int col1, int row2, int col2, unsigned int rgb)
{
	float deltaRow = (float)row2 - (float)row1;
	float deltaCol = (float)col2 - (float)col1;

	int len = (int)(roundf(sqrtf( deltaCol*deltaCol + deltaRow*deltaRow )));

	float stepRow, stepCol;


	stepRow = deltaRow / (float)len;
	stepCol = deltaCol / (float)len;

	float rowFloat;
	float colFloat;


	rowFloat = (float)row1;
	colFloat = (float)col1;



	while(len--)
	{
		set_img_pixel(img, (int)roundf(rowFloat), (int)roundf(colFloat), rgb);

		rowFloat += stepRow;
		colFloat += stepCol;
	}


}





void draw_rect(bmp_img* img, int row, int col, int w, int h, unsigned int rgb)
{
	int dcol, drow;
	int len;
	for(int side=0; side<4; ++side)
	{
		if(side == 0)
		{
			dcol = 1;
			drow = 0;
			len = w;
		}
		else if(side == 1)
		{
			dcol = 0;
			drow = 1;
			len = h;
		}
		if(side == 2)
		{
			dcol = -1;
			drow = 0;
			len = w;
		}
		if(side == 3)
		{
			dcol = 0;
			drow = -1;
			len = h;
		}


		while(len--)
		{
			if((row < 0) || (col < 0))
				break;

			if((row >= img->img_header.biHeight) || (col >= img->img_header.biWidth))
			{
				break;
			}



			(img->img_pixels[row] + col)->red =   rgb >> 16;
			(img->img_pixels[row] + col)->green = rgb >> 8;
			(img->img_pixels[row] + col)->blue =  rgb >> 0;

			col += dcol;
			row += drow;
		}
	}
}


bmp_img img;
Pixel_t* pixels;

Pixel_t* pixels_sorted;

Pixel_t* starPixels[1024 * 1024];
unsigned int starPixelsCount = 0;

const int deltas[8][2] = {
		{0,1},   //Right
		{-1,1},  //Up-right
		{-1,0},  //Up
		{-1,-1}, //Up-left
		{0,-1},  //Left
		{1,-1},  //Down-left
		{1,0},   //Down
		{1,1},   //Down-right
};


Pixel_t* getpix(int row, int col)
{

	if(row < 0)
		return NULL;

	if(row >= img.img_header.biHeight)
		return NULL;

	if(col < 0)
		return NULL;

	if(col >= img.img_header.biWidth)
		return NULL;

	return &pixels[row * img.img_header.biWidth + col];
}

Pixel_t* strideToNextPixel(Pixel_t* curPixel);
Pixel_t* strideToNextRedPixel(Pixel_t* curPixel);


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




int main(int argc, char** argv)
{

	char filebase[100];
	char report_name[256];
	char out_image_name[256];

	if(argc < 2)
	{
		printf("No BMP file selected.\r\n");

		fflush(stdout);
		return 1;
	}

	//memset(found_stars, 0x00, sizeof(found_stars));




	if(bmp_img_read(&img, argv[1]) != BMP_OK)
	{
		printf("Cannot open BMP file.\r\n");
		fflush(stdout);

		return 1;
	}

	getFileNameWithoutExtension(argv[1], filebase);

	sprintf(report_name, "%s_report.txt", filebase);
	sprintf(out_image_name, "%s_report.bmp", filebase);

	int pixelCount = img.img_header.biHeight * img.img_header.biWidth;

	pixels = (Pixel_t*)malloc( sizeof(Pixel_t) * pixelCount );
	int* intesities_sorted = (int*)malloc( sizeof(int) * pixelCount );
	unsigned char* pixels_sorted_marks =  calloc( pixelCount , 1 );


	int starBufferSize = 100;
	found_stars = (star_t**)malloc(sizeof(star_t**) * starBufferSize);

	if(!found_stars || !pixels || !intesities_sorted || !pixels_sorted_marks)
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

	FILE* fReport;

	fReport = fopen(report_name, "w");

	fprintf(fReport, "StarBound v0.2\nInput file: %s\nCoordinate system: Row-Column\nOrigin: Top-Left\nIntensity: 0.000...1.000\n\n", argv[1]);

	for(int i=0; i < pixelCount; ++i)
	{
		if(pixels[i].intensity_normalized >= average_intensity*2) //Pixels with at least 2x of average intensity qualified as star pixels
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

			star_t* newStar = (star_t*)calloc(sizeof(star_t), 1);

			int starPixelBufferSize = 100;
			newStar->pixels = (Pixel_t**)calloc(sizeof(Pixel_t**), starPixelBufferSize);

			int starBoundRowMin = -1;
			int starBoundRowMax = -1;
			int starBoundColMin = -1;
			int starBoundColMax = -1;

			Pixel_t** pixelStrideList;
			int pixelStrideCount = 0;

			int pixelStrideListSize = 100;

			pixelStrideList = (Pixel_t**)malloc(sizeof(Pixel_t**) * pixelStrideListSize);
			pixelStrideCount = 0;

			while(1)
			{
				pixelStrideList[pixelStrideCount++] = nextpix;

				if(pixelStrideCount >= pixelStrideListSize)
				{
					pixelStrideListSize += 100;
					pixelStrideList = (Pixel_t**)realloc(pixelStrideList, sizeof(Pixel_t**) * pixelStrideListSize);
				}

			//	lastRow = nextpix->row;
			//	lastCol = nextpix->col;

				nextpix->color = COLOR_STAR_OUTLINE;
				set_img_pixel(&img, nextpix->row, nextpix->col, nextpix->color);

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

				if(starBoundRowMin == -1)
					starBoundRowMin = nextpix->row;
				else if (nextpix->row < starBoundRowMin)
					starBoundRowMin = nextpix->row;

				if(starBoundRowMax == -1)
					starBoundRowMax = nextpix->row;
				else if (nextpix->row > starBoundRowMax)
					starBoundRowMax = nextpix->row;

				if(starBoundColMin == -1)
					starBoundColMin = nextpix->col;
				else if (nextpix->col < starBoundColMin)
					starBoundColMin = nextpix->col;

				if(starBoundColMax == -1)
					starBoundColMax = nextpix->col;
				else if (nextpix->col > starBoundColMax)
					starBoundColMax = nextpix->col;


				nextpix = strideToNextPixel(nextpix);

				while(!nextpix)
				{
					pixelStrideCount--;

					if(pixelStrideCount < 1)
						break;

					nextpix = strideToNextPixel(pixelStrideList[pixelStrideCount - 1]);
				}

				if((!nextpix))
				{
					free(pixelStrideList);
					pixelStrideList = NULL;
					break;
				}
			}

			if(newStar->pixelCount > 0)
			{
				int centerRow = 0;
				int centerCol = 0;

				float sum_intensity = 0;

				for(int i = 0; i < newStar->pixelCount; ++i)
				{
					centerRow += newStar->pixels[i]->row * newStar->pixels[i]->intensity_normalized;
					centerCol += newStar->pixels[i]->col * newStar->pixels[i]->intensity_normalized;

					sum_intensity += newStar->pixels[i]->intensity_normalized;
				}

				newStar->centerCol = centerCol / sum_intensity;
				newStar->centerRow = centerRow / sum_intensity;


				newStar->boundRowMin = starBoundRowMin;
				newStar->boundRowMax = starBoundRowMax;
				newStar->boundColMin = starBoundColMin;
				newStar->boundColMax = starBoundColMax;

				newStar->sizeCol = starBoundColMax - starBoundColMin;
				newStar->sizeRow = starBoundRowMax - starBoundRowMin;



				fprintf(fReport, "Star #%d:\n", (stars_count+1));
				fprintf(fReport, " Mass center: [%d,%d]\n", newStar->centerRow, newStar->centerCol);
				fprintf(fReport, " Pixels:\n");
				for(int i=0; i<newStar->pixelCount; ++i)
				{
					fprintf(fReport, "  Pixel %d:\t[%d,%d]\tIntensity: %1.3f\n", (i+1),
																		newStar->pixels[i]->row,
																		newStar->pixels[i]->col,
																		newStar->pixels[i]->intensity_normalized);
				}
				fprintf(fReport, "\n");

				found_stars[stars_count++] = newStar;

				if(stars_count >= starBufferSize)
				{
					starBufferSize += 100;

					found_stars = (star_t**)realloc((void*)found_stars, starBufferSize * sizeof(star_t**));
				}





			}
		}
	}

	for(int i=0; i<stars_count; ++i)
	{
		//int lineSize = (found_stars[i]->sizeCol > found_stars[i]->sizeRow)?(found_stars[i]->sizeCol):(found_stars[i]->sizeRow);
		//int lineSize = (found_stars[i]->sizeCol < found_stars[i]->sizeRow)?(found_stars[i]->sizeCol):(found_stars[i]->sizeRow);


		int lineSize = 10;

		//vertical line of cross
		draw_line(&img, found_stars[i]->centerRow - (lineSize / 2 + 2),
					found_stars[i]->centerCol,
					found_stars[i]->centerRow + (lineSize / 2 + 2) + 1,
					found_stars[i]->centerCol,
					COLOR_STAR_CENTER);

		//horizontal line of cross
		draw_line(&img, found_stars[i]->centerRow,
					found_stars[i]->centerCol - (lineSize / 2 + 2),
					found_stars[i]->centerRow,
					found_stars[i]->centerCol + (lineSize / 2 + 2) + 1,
					COLOR_STAR_CENTER);


		draw_rect(&img, found_stars[i]->boundRowMin - 1,
					found_stars[i]->boundColMin - 1,
					found_stars[i]->sizeCol + 2,
					found_stars[i]->sizeRow + 2,
					COLOR_STAR_CENTER);

		draw_number(&img, (i+1),
				    found_stars[i]->boundRowMin - 10,
					(found_stars[i]->boundColMin - 1),
					COLOR_STAR_CENTER);
	}




	bmp_img_write(&img, out_image_name);
	return 0;
}


//Select next pixel for outline
Pixel_t* strideToNextPixel(Pixel_t* curPixel)
{
	int nextRow, nextCol;
	Pixel_t* nextPix;

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

		nextPix = getpix(nextRow, nextCol);

		if(nextPix->color == COLOR_STAR)
		{
			return nextPix;
		}
	}

	return NULL;


}

Pixel_t* strideToNextRedPixel(Pixel_t* curPixel)
{
	int nextX, nextY;
	Pixel_t* nextPix;

	nextPix = NULL;
	for(int i=0; i<8; ++i)
	{
		nextX = curPixel->row + deltas[i][0];
		nextY = curPixel->col + deltas[i][1];

		if (nextX >= img.img_header.biWidth)
			continue;
		if (nextY >= img.img_header.biHeight)
			continue;
		if (nextX < 0)
			continue;
		if (nextY < 0)
			continue;

		if(getpix(nextX, nextY)->color != COLOR_STAR)
			continue;



		//Count adjacent red pixels near the next pixel
		Pixel_t* nearPix;

		int adj_pixels = 0;
		nearPix = NULL;
		for(int i=0; i<8; ++i)
		{
			nearPix = getpix(nextX + deltas[i][0], nextY + deltas[i][1]);

			if(nearPix == curPixel) //Way back is closed
				continue;

			if(nearPix->color == COLOR_STAR)
			{
				adj_pixels++;
			}
		}


		if(adj_pixels == 0)
		{
			nearPix = NULL;
		}

		if(nearPix)
		{
			nextPix = getpix(nextX, nextY);
			break;
		}
	}

	return nextPix;
}







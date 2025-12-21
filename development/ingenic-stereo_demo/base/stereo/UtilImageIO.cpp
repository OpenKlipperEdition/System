#include <UtilImageIO.hpp>
#include <stdlib.h>
#define STBI_WINDOWS_UTF8

// #define STB_IMAGE_WRITE_IMPLEMENTATION
// #include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_DEFINE
#include "stb.h"
#include <stdio.h>

namespace JzStereo {
IMat &UtilImageIO::loadImage(std::string filename)
{
  uint8_t *data;
  int width, height, channel;
  data = stbi_load(filename.c_str(), &width, &height, &channel, 0);

  if (channel == 3)
    {
      IMat *image =  new IMat(data, width, height, IMat_8UC3);
      printf("widht %d height %d channel %d\n", width, height, channel);
      return *image;
    }
  else
    {
      printf("invalid image channel\n");
      abort();
    }
};

IMat &UtilImageIO::rgb2Gray(IMat &src)
{
  return rgbResize2Gray(src, src.width, src.height);
}


IMat &UtilImageIO::rgbResize2Gray (IMat &src, int _width, int _height)
{
  float xRatio_f = (float)src.width/_width;
  float yRatio_f = (float)src.height/_height;
  IMat *dest = new IMat(_width, _height, IMat_8U);
  uint8_t* sdata = src.ptr<uint8_t>();
  uint8_t* ddata = dest->ptr<uint8_t>();

  for (int i = 0; i < _height; i++)
    {
      float srcY = i * yRatio_f;
      int IntY = (int)srcY;
      float vertA = srcY - IntY;
      float vertB = 1.0 - vertA;
      
      for (int j = 0; j < _width; j++)
	{
	  float srcX = j * xRatio_f;
	  int IntX = (int)srcX;
	  float horA = srcX - IntX;
	  float horB = 1.0 - horA;

	  int index00 = IntY * src.stride + IntX * src.channels();
	  int index10;
	  if (IntY < src.height - 1)
	    {
	      index10 = index00 + src.stride;	      
	    }
	  
	  int index01, index11;
	  if (IntX < src.width - 1)
	    {
	      index01 = index00 + src.channels(); // pixedsize
	      index11 = index10 + src.channels(); // pixedsize
	    }
	  else
	    {
	      index01 = index00; // pixedsize
	      index11 = index10; // pixedsize	      
	    }
	  float R =
	    vertB * (horA * sdata[index01] +
		     horB * sdata[index00]) +
	    vertA * (horA * sdata[index11] +
		     horB * sdata[index10]);
	  
	  float G =
	    vertB * (horA * sdata[index01 + src.channels()] +
		     horB * sdata[index00 + src.channels()]) +
	    vertA * (horA * sdata[index11 + src.channels()] +
		     horB * sdata[index10 + src.channels()]);
	  
	  float B =
	    vertB * (horA * sdata[index01 + 2 * src.channels()] +
		     horB * sdata[index00 + 2 * src.channels()]) +
	    vertA * (horA * sdata[index11 + 2 * src.channels()] +
		     horB * sdata[index10 + 2 * src.channels()]);
	  
	  int gray = ((uint8_t)R * 38 + (uint8_t)G * 75 + (uint8_t)B * 15) >> 7;
	  ddata[i * dest->stride + j] = gray;
	}
    }
  return *dest;
}
}

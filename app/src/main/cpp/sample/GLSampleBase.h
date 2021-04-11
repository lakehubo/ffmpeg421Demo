//
// Created by ByteFlow on 2019/7/9.
//

#ifndef NDK_OPENGLES_3_0_GLSAMPLEBASE_H
#define NDK_OPENGLES_3_0_GLSAMPLEBASE_H

#include "stdint.h"
#include <GLES3/gl3.h>
#include <GLFrame.h>
//For PI define
#define MATH_PI 3.1415926535897932384626433832802

#define SAMPLE_TYPE                             200
#define SAMPLE_TYPE_KEY_YUV_TEXTURE_MAP         SAMPLE_TYPE + 1

class GLSampleBase
{
public:
	GLSampleBase()
	{
		yuv_ProgramObj = 0;
	}

	virtual ~GLSampleBase()
	{

	}

	virtual void LoadFrame(AVFrame *pImage) = 0;

	virtual void Init() = 0;
	virtual void Draw(int screenW, int screenH) = 0;

	virtual void Destroy() = 0;

protected:
	GLuint yuv_ProgramObj;
};


#endif //NDK_OPENGLES_3_0_GLSAMPLEBASE_H

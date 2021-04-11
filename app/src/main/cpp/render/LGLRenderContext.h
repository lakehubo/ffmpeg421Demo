//
// Created by ByteFlow on 2019/7/9.
//

#ifndef NDK_OPENGLES_3_0_MYGLRENDERCONTEXT_H
#define NDK_OPENGLES_3_0_MYGLRENDERCONTEXT_H

#include "stdint.h"
#include <GLES3/gl3.h>
#include <jni.h>
#include "YUV420PTexture.h"
#include "NV21Texture.h"
#include "NV12Texture.h"

class LGLRenderContext
{
	LGLRenderContext();

	~LGLRenderContext();

public:
	void SetFrameData(AVFrame *pFrame);

	void OnSurfaceCreated();

	void initProgram(int fmt);

	void OnSurfaceChanged(int width, int height);

	void OnDrawFrame();

	static LGLRenderContext* GetInstance();
	static void DestroyInstance();

private:
	static LGLRenderContext *m_pContext;
	GLSampleBase *m_pCurSample;
	int format;
	bool init;
	int m_ScreenW;
	int m_ScreenH;

    void deleteCurSample();
};


#endif //NDK_OPENGLES_3_0_MYGLRENDERCONTEXT_H

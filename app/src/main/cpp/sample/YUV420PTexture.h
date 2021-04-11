//
// Created by ByteFlow on 2019/7/10.
//

#ifndef NDK_OPENGLES_3_0_YUVTEXTUREMAPSAMPLE_H
#define NDK_OPENGLES_3_0_YUVTEXTUREMAPSAMPLE_H

extern "C" {
#include <libavutil/frame.h>
};

#include "GLSampleBase.h"

class YUV420PTexture : public GLSampleBase {
public:
    YUV420PTexture() {
        m_yTextureId = GL_NONE;
        m_uTextureId = GL_NONE;
        m_vTextureId = GL_NONE;

        m_ySamplerLoc = GL_NONE;
        m_uSamplerLoc = GL_NONE;
        m_vSamplerLoc = GL_NONE;
    }

    virtual ~YUV420PTexture() {
        GLFrameUtil::FreeGLFrame(&m_RenderFrame);
    }

    virtual void LoadFrame(AVFrame *frame);

    virtual void Init();

    virtual void Draw(int screenW, int screenH);

    virtual void Destroy();

private:
    GLuint m_yTextureId;
    GLuint m_uTextureId;
    GLuint m_vTextureId;

    GLint m_ySamplerLoc;
    GLint m_uSamplerLoc;
    GLint m_vSamplerLoc;

    GLFrame m_RenderFrame;
};


#endif //NDK_OPENGLES_3_0_YUVTEXTUREMAPSAMPLE_H

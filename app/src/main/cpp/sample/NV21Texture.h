//
// Created by lake on 2020/4/14.
//

#ifndef FFMPEG421DEMO_NV21TEXTURE_H
#define FFMPEG421DEMO_NV21TEXTURE_H

#include <libavutil/frame.h>
#include "GLSampleBase.h"

class NV21Texture : public GLSampleBase {
public:
    NV21Texture() {
        m_yTextureId = GL_NONE;
        m_uvTextureId = GL_NONE;

        m_ySamplerLoc = GL_NONE;
        m_uvSamplerLoc = GL_NONE;
    }

    virtual ~NV21Texture() {
        GLFrameUtil::FreeGLFrame(&m_RenderFrame);
    }

    virtual void LoadFrame(AVFrame *frame);

    virtual void Init();

    virtual void Draw(int screenW, int screenH);

    virtual void Destroy();

private:
    GLuint m_yTextureId;
    GLuint m_uvTextureId;

    GLint m_ySamplerLoc;
    GLint m_uvSamplerLoc;

    GLFrame m_RenderFrame;
};

#endif //FFMPEG421DEMO_NV21TEXTURE_H

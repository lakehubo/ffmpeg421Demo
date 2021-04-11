
#include "LGLRenderContext.h"
#include "LogUtil.h"

LGLRenderContext *LGLRenderContext::m_pContext = nullptr;

LGLRenderContext::LGLRenderContext() {
    format = AV_PIX_FMT_NONE;
    m_pCurSample = nullptr;
    init = false;
}

void LGLRenderContext::initProgram(int fmt) {
    deleteCurSample();
    switch (fmt){
        case AV_PIX_FMT_YUV420P:
            m_pCurSample = new YUV420PTexture();
            break;
        case AV_PIX_FMT_NV21:
            m_pCurSample = new NV21Texture();
            break;
        case AV_PIX_FMT_NV12:
            m_pCurSample = new NV12Texture();
            break;
        default:
            break;
    }
    format = fmt;
}

LGLRenderContext::~LGLRenderContext() {
    deleteCurSample();
}

void LGLRenderContext::deleteCurSample(){
    if (m_pCurSample) {
        m_pCurSample->Destroy();
        delete m_pCurSample;
        m_pCurSample = nullptr;
        init = false;
    }
}

void LGLRenderContext::SetFrameData(AVFrame *pFrame) {
    if (m_pCurSample) {
        m_pCurSample->LoadFrame(pFrame);
    }
}

void LGLRenderContext::OnSurfaceCreated() {
    LOGCATE("LGLRenderContext::OnSurfaceCreated");
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);//清屏颜色
}

void LGLRenderContext::OnSurfaceChanged(int width, int height) {
    LOGCATE("LGLRenderContext::OnSurfaceChanged [w, h] = [%d, %d]", width, height);
    glViewport(0, 0, width, height);
    m_ScreenW = width;
    m_ScreenH = height;
}

void LGLRenderContext::OnDrawFrame() {
    LOGCATE("LGLRenderContext::OnDrawFrame");
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    if (m_pCurSample) {
        if(!init){
            init = true;
            m_pCurSample->Init();
        }
        m_pCurSample->Draw(m_ScreenW, m_ScreenH);
    }
}

LGLRenderContext *LGLRenderContext::GetInstance() {
    LOGCATE("LGLRenderContext::GetInstance");
    if (m_pContext == nullptr) {
        m_pContext = new LGLRenderContext();
    }
    return m_pContext;
}

void LGLRenderContext::DestroyInstance() {
    LOGCATE("LGLRenderContext::DestroyInstance");
    if (m_pContext) {
        delete m_pContext;
        m_pContext = nullptr;
    }
}



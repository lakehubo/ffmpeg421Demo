//
// Created by lake on 2020/3/23.
//

#ifndef FFMPEG421DEMO_GLFRAME_H
#define FFMPEG421DEMO_GLFRAME_H

#include <unistd.h>
#include <LogUtil.h>

typedef struct GLFrame {
    int width;
    int height;
    int format;
    uint8_t *data[3];

    GLFrame() {
        width = 0;
        height = 0;
        format = 0;
        data[0] = nullptr;
        data[1] = nullptr;
        data[2] = nullptr;
    };
} GLFrame;

class GLFrameUtil {
public:
    static void AllocGLFrame(GLFrame *pFrame) {
        if (pFrame->height == 0 || pFrame->width == 0) return;
        switch (pFrame->format){
            case AV_PIX_FMT_YUV420P:
                pFrame->data[0] = (uint8_t *) malloc(static_cast<size_t>(pFrame->width * pFrame->height));
                pFrame->data[1] = (uint8_t *) malloc(static_cast<size_t>((pFrame->width * pFrame->height) >> 2));
                pFrame->data[2] = (uint8_t *) malloc(static_cast<size_t>((pFrame->width * pFrame->height) >> 2));
                break;
            case AV_PIX_FMT_NV21:
            case AV_PIX_FMT_NV12:
                pFrame->data[0] = (uint8_t *) malloc(static_cast<size_t>(pFrame->width * pFrame->height));
                pFrame->data[1] = (uint8_t *) malloc(static_cast<size_t>((pFrame->width * pFrame->height) >> 1));
                break;
            default:
                break;
        }
    }

    static void FreeGLFrame(GLFrame *pFrame) {
        if (pFrame == nullptr || pFrame->data[0] == nullptr) return;
        free(pFrame->data[0]);
        free(pFrame->data[1]);
        free(pFrame->data[2]);
        pFrame->data[0] = nullptr;
        pFrame->data[1] = nullptr;
        pFrame->data[1] = nullptr;
    }

    static void CopyFrameToGLFrame(AVFrame *src_frame, GLFrame *dst_frame) {
        if (src_frame == nullptr || src_frame->data == nullptr) return;
        if (src_frame->format != dst_frame->format ||
            src_frame->width != dst_frame->width ||
            src_frame->height != dst_frame->height)
            return;
        if (dst_frame->data[0] == nullptr)AllocGLFrame(dst_frame);
        switch (src_frame->format){
            case AV_PIX_FMT_YUV420P:
                memcpy(dst_frame->data[0], src_frame->data[0], static_cast<size_t>(src_frame->width * src_frame->height));
                memcpy(dst_frame->data[1], src_frame->data[1], static_cast<size_t>((src_frame->width * src_frame->height) >> 2));
                memcpy(dst_frame->data[2], src_frame->data[2], static_cast<size_t>((src_frame->width * src_frame->height) >> 2));
                break;
            case AV_PIX_FMT_NV21:
            case AV_PIX_FMT_NV12:
                memcpy(dst_frame->data[0], src_frame->data[0], static_cast<size_t>(src_frame->width * src_frame->height));
                memcpy(dst_frame->data[1], src_frame->data[1], static_cast<size_t>((src_frame->width * src_frame->height) >> 1));
                break;
        }
    }
};
#endif //FFMPEG421DEMO_GLFRAME_H

//
// Created by ByteFlow on 2019/7/10.
//

#include <GLUtils.h>
#include <LogUtil.h>
#include "YUV420PTexture.h"

void YUV420PTexture::LoadFrame(AVFrame *frame) {
    LOGCATE("YUV420PTexture::LoadImage pFrame = %p", frame);
    LOGCATE("YUV420PTexture::LoadImage pFrame = %p", &m_RenderFrame);
    if (frame->data) {
        m_RenderFrame.format = frame->format;
        m_RenderFrame.width = frame->width;
        m_RenderFrame.height = frame->height;
        GLFrameUtil::CopyFrameToGLFrame(frame, &m_RenderFrame);
    }
}

void YUV420PTexture::Init() {
    char vShaderStr[] =
            "#version 300 es                            \n"
            "layout(location = 0) in vec4 a_position;   \n"
            "layout(location = 1) in vec2 a_texCoord;   \n"
            "out vec2 v_texCoord;                       \n"
            "void main()                                \n"
            "{                                          \n"
            "   gl_Position = a_position;               \n"
            "   v_texCoord = a_texCoord;                \n"
            "}                                          \n";

    char yuv420pShaderStr[] =
            "#version 300 es                                     \n"
            "precision mediump float;                            \n"
            "in vec2 v_texCoord;                                 \n"
            "layout(location = 0) out vec4 outColor;             \n"
            "uniform sampler2D y_texture;                        \n"
            "uniform sampler2D u_texture;                        \n"
            "uniform sampler2D v_texture;                        \n"
            "void main()                                         \n"
            "{                                                   \n"
            "	vec3 yuv;										\n"
            "   yuv.r = texture(y_texture, v_texCoord).r;  	\n"
            "   yuv.g = texture(u_texture, v_texCoord).r-0.5f;	\n"
            "   yuv.b = texture(v_texture, v_texCoord).r-0.5f;	\n"
            "	highp vec3 rgb = mat3( 1,       1,       	1,					\n"
            "               0, 		-0.34414, 	1.772,					\n"
            "               1.402,  -0.71414,       0) * yuv; 			\n"
            "	outColor = vec4(rgb, 1);						\n"
            "}                                                   \n";

    // Load the shaders and get a linked program object
    yuv_ProgramObj = GLUtils::CreateProgram(vShaderStr, yuv420pShaderStr);

    // Get the sampler location
    m_ySamplerLoc = glGetUniformLocation(yuv_ProgramObj, "y_texture");
    m_uSamplerLoc = glGetUniformLocation(yuv_ProgramObj, "u_texture");
    m_vSamplerLoc = glGetUniformLocation(yuv_ProgramObj, "v_texture");

    //create textures
    GLuint textureIds[3] = {0};
    glGenTextures(3, textureIds);

    m_yTextureId = textureIds[0];
    m_uTextureId = textureIds[1];
    m_vTextureId = textureIds[2];

    GLFrameUtil::AllocGLFrame(&m_RenderFrame);
}

void YUV420PTexture::Draw(int screenW, int screenH) {
    LOGCATE("YUV420PTexture::Draw()");

    if (yuv_ProgramObj == GL_NONE || m_yTextureId == GL_NONE
        || m_uTextureId == GL_NONE || m_vTextureId == GL_NONE)
        return;

    //upload Y plane data
    glBindTexture(GL_TEXTURE_2D, m_yTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_RenderFrame.width, m_RenderFrame.height, 0,
                 GL_LUMINANCE, GL_UNSIGNED_BYTE, m_RenderFrame.data[0]);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, GL_NONE);

    //update U plane data
    glBindTexture(GL_TEXTURE_2D, m_uTextureId);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_RenderFrame.width >> 1,
                 m_RenderFrame.height >> 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                 m_RenderFrame.data[1]);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, GL_NONE);

    //update UV plane data
    glBindTexture(GL_TEXTURE_2D, m_vTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_RenderFrame.width >> 1,
                 m_RenderFrame.height >> 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                 m_RenderFrame.data[2]);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, GL_NONE);

    GLfloat verticesCoords[] = {
            -1.0f, 0.78f, 0.0f,  // Position 0
            -1.0f, -0.78f, 0.0f,  // Position 1
            1.0f, -0.78f, 0.0f,  // Position 2
            1.0f, 0.78f, 0.0f,  // Position 3
    };

    GLfloat textureCoords[] = {
            0.0f, 0.0f,        // TexCoord 0
            0.0f, 1.0f,        // TexCoord 1
            1.0f, 1.0f,        // TexCoord 2
            1.0f, 0.0f         // TexCoord 3
    };

    GLushort indices[] = {0, 1, 2, 0, 2, 3};

    // Use the program object
    glUseProgram(yuv_ProgramObj);

    // Load the vertex position 点坐标
    glVertexAttribPointer(0, 3, GL_FLOAT,
                          GL_FALSE, 3 * sizeof(GLfloat), verticesCoords);
    // Load the texture coordinate 纹理坐标
    glVertexAttribPointer(1, 2, GL_FLOAT,
                          GL_FALSE, 2 * sizeof(GLfloat), textureCoords);

    //a_position
    glEnableVertexAttribArray(0);
    //a_texCoord
    glEnableVertexAttribArray(1);

    // Bind the Y plane map
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_yTextureId);

    // Bind the U plane map
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_uTextureId);



    // Set the Y plane sampler to texture unit to 0
    glUniform1i(m_ySamplerLoc, 0);
    // Set the U plane sampler to texture unit to 1
    glUniform1i(m_uSamplerLoc, 1);
    // Bind the V plane map
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_vTextureId);
    // Set the V plane sampler to texture unit to 2
    glUniform1i(m_vSamplerLoc, 2);


    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
}

void YUV420PTexture::Destroy() {
    if (yuv_ProgramObj) {
        glDeleteProgram(yuv_ProgramObj);
        yuv_ProgramObj = GL_NONE;
    }
    glDeleteTextures(1, &m_yTextureId);
    glDeleteTextures(1, &m_uTextureId);
    glDeleteTextures(1, &m_vTextureId);
}

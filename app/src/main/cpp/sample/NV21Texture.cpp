//
// Created by lake on 2020/4/14.
//

#include <GLUtils.h>
#include "NV21Texture.h"

void NV21Texture::LoadFrame(AVFrame *frame) {
    if (frame->data) {
        m_RenderFrame.format = frame->format;
        m_RenderFrame.width = frame->width;
        m_RenderFrame.height = frame->height;
        GLFrameUtil::CopyFrameToGLFrame(frame, &m_RenderFrame);
    }
}

void NV21Texture::Init() {
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

    char nv21ShaderStr[] =
            "#version 300 es                                     \n"
            "precision mediump float;                            \n"
            "in vec2 v_texCoord;                                 \n"
            "layout(location = 0) out vec4 outColor;             \n"
            "uniform sampler2D y_texture;                        \n"
            "uniform sampler2D uv_texture;                        \n"
            "void main()                                         \n"
            "{                                                   \n"
            "	vec3 yuv;										\n"
            "   yuv.x = texture(y_texture, v_texCoord).r;  	\n"
            "   yuv.y = texture(uv_texture, v_texCoord).a-0.5;	\n"
            "   yuv.z = texture(uv_texture, v_texCoord).r-0.5;	\n"
            "	highp vec3 rgb = mat3( 1,       1,       	1,					\n"
            "               0, 		-0.344, 	1.770,					\n"
            "               1.403,  -0.714,       0) * yuv; 			\n"
            "	outColor = vec4(rgb, 1);						\n"
            "}                                                   \n";

    yuv_ProgramObj = GLUtils::CreateProgram(vShaderStr, nv21ShaderStr);
    m_ySamplerLoc = glGetUniformLocation(yuv_ProgramObj, "y_texture");
    m_uvSamplerLoc = glGetUniformLocation(yuv_ProgramObj, "uv_texture");

    GLuint textureIds[2] = {0};
    glGenTextures(2, textureIds);

    m_yTextureId = textureIds[0];
    m_uvTextureId = textureIds[1];

    GLFrameUtil::AllocGLFrame(&m_RenderFrame);
}

void NV21Texture::Draw(int screenW, int screenH) {
    if (yuv_ProgramObj == GL_NONE|| m_yTextureId == GL_NONE|| m_uvTextureId == GL_NONE)
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
    glBindTexture(GL_TEXTURE_2D, m_uvTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, m_RenderFrame.width >> 1,
                     m_RenderFrame.height >> 1, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,
                     m_RenderFrame.data[1]);
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
    glBindTexture(GL_TEXTURE_2D, m_uvTextureId);

    // Set the Y plane sampler to texture unit to 0
    glUniform1i(m_ySamplerLoc, 0);
    // Set the UV plane sampler to texture unit to 1
    glUniform1i(m_uvSamplerLoc, 1);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
}

void NV21Texture::Destroy() {
    if (yuv_ProgramObj) {
        glDeleteProgram(yuv_ProgramObj);
        yuv_ProgramObj = GL_NONE;
    }
    glDeleteTextures(1, &m_yTextureId);
    glDeleteTextures(1, &m_uvTextureId);
}

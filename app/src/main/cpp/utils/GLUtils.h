#ifndef _BYTE_FLOW_GL_UTILS_H_
#define _BYTE_FLOW_GL_UTILS_H_

#include <GLES3/gl3.h>
#include <string>
//#include <glm.hpp>

class GLUtils
{
public:
	static GLuint LoadShader(GLenum shaderType, const char *pSource);

	static GLuint CreateProgram(const char *pVertexShaderSource, const char *pFragShaderSource);

	static void DeleteProgram(GLuint &program);

	static void CheckGLError(const char *pGLOperation);

};
#endif // _BYTE_FLOW_GL_UTILS_H_
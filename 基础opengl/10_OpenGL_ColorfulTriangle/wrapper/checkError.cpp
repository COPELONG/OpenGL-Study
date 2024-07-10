#include "checkError.h"
#include <glad/glad.h>
#include <string>
#include <iostream>
#include <assert.h>

void checkError() {
	GLenum errorCode = glGetError();
	std::string error = "";
	if (errorCode != GL_NO_ERROR) {
		switch (errorCode)
		{
		case GL_INVALID_ENUM: error = "INVALID_ENUM"; break;
		case GL_INVALID_VALUE:  error = "INVALID_VALUE"; break;
		case GL_INVALID_OPERATION: error = "INVALID_OPERATION"; break;
		case GL_OUT_OF_MEMORY: error = "OUT OF MEMORY"; break;
		default:
			error = "UNKNOWN";
			break;
		}
		std::cout << error << std::endl;

		//assert会根据传入的bool值，来决定程序是否停止
		//true：程序顺利运行
		//false：程序会断死
		assert(false);
	}
}
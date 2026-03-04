#include "glsl_program.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

namespace vray {

	void GlslException::forceThrowFromLog(GLuint objectId, ExceptionType type) {
		GLint logLength = 0;
		std::string errorBuff;

		if (type == ExceptionType::SHADER) {
			glGetShaderiv(objectId, GL_INFO_LOG_LENGTH, &logLength);
			errorBuff = std::string(logLength, ' ');
			glGetShaderInfoLog(objectId, errorBuff.size(), nullptr, errorBuff.data());
			glDeleteShader(objectId);
		}
		else if (type == ExceptionType::PROGRAM) {
			glGetProgramiv(objectId, GL_INFO_LOG_LENGTH, &logLength);
			errorBuff = std::string(logLength, ' ');
			glGetProgramInfoLog(objectId, errorBuff.size(), nullptr, errorBuff.data());
			glDeleteProgram(objectId);
		}
		else {
			throw GlslException("Unnacceptable object type on GlslException!");
		}

		throw GlslException(errorBuff);
	}

	std::string loadFile(const std::string& filename) {
		std::ifstream fin(filename, std::ios::binary);

		if (!fin) {
			fin.close();
			throw GlslException("Can't load shader file!");
		}

		fin.seekg(0);

		std::ostringstream strBuff;
		strBuff << fin.rdbuf();
		fin.close();

		return strBuff.str();
	}

	GlslUniform::GlslUniform(GlslProgram* glslProgram, const std::string& _name)
	: program(glslProgram->getHandle()), name(_name) {
		location = glslProgram->getUniformLocation(name);
	}

	GlslProgram::~GlslProgram() {
		glDeleteProgram(handle);
	}

	void GlslProgram::compileShader(const std::string& filename, ShaderType type) {
		GLuint shaderId = glCreateShader((GLenum)type);
		std::string sourceStr = loadFile(filename);

		const char* sourcePtr = sourceStr.c_str();
		glShaderSource(shaderId, 1, &sourcePtr, nullptr);
		glCompileShader(shaderId);

		GLint successful;
		glGetShaderiv(shaderId, GL_COMPILE_STATUS, &successful);

		if (!successful) {
			GlslException::forceThrowFromLog(shaderId, GlslException::SHADER);
		}

		if (!handle) handle = glCreateProgram();
		glAttachShader(handle, shaderId);
		glDeleteShader(shaderId); // !!!
	}

	void GlslProgram::link() {
		GLint successful = 0;

		glLinkProgram(handle);
		glGetProgramiv(handle, GL_LINK_STATUS, &successful);

		if (!successful) {
			GlslException::forceThrowFromLog(handle, GlslException::PROGRAM);
		}

		linked = true;
	}

	void GlslProgram::use() { glUseProgram(handle);	}
	void GlslProgram::unuse() { glUseProgram(0); }

	void GlslProgram::validate() {
		GLint successful = 0;

		glValidateProgram(handle);
		glGetProgramiv(handle, GL_VALIDATE_STATUS, &successful);

		if (!successful) {
			GlslException::forceThrowFromLog(handle, GlslException::PROGRAM);
		}

		std::cout << "Program " + std::to_string(handle) + " passed validation!" << std::endl;
	}

	int GlslProgram::getUniformLocation(const std::string& name) {
		GLint location = glGetUniformLocation(handle, name.c_str());
		if (location < 0) throw GlslException(name + " - uniform name is unacceptable! Code " + std::to_string(location));
		return location;
	}

	GlslUniform GlslProgram::getUniform(const std::string& name) {
		return GlslUniform(this, name);
	}

	void GlslProgram::setUniform(const GlslUniform& uniform, const glm::mat2& matrix) {
		glUniformMatrix2fv(uniform.getLocation(), 1, GL_FALSE, &matrix[0][0]);
	}

	void GlslProgram::setUniform(const GlslUniform& uniform, const glm::mat3& matrix) {
		glUniformMatrix3fv(uniform.getLocation(), 1, GL_FALSE, &matrix[0][0]);
	}

	void GlslProgram::setUniform(const GlslUniform& uniform, const glm::mat4& matrix) {
		glUniformMatrix4fv(uniform.getLocation(), 1, GL_FALSE, &matrix[0][0]);
	}

	void GlslProgram::setUniform(const GlslUniform& uniform, const glm::vec3& vec) {
		glUniform3fv(uniform.getLocation(), 1, &vec[0]);
	}

	void GlslProgram::setUniform(const GlslUniform& uniform, const glm::vec4& vec) {
		glUniform4fv(uniform.getLocation(), 1, &vec[0]);
	}

	void GlslProgram::printActiveUniforms() const {
//		static GLuint properties[] = {GL_NAME_LENGTH, GL_TYPE, GL_LOCATION, GL_BLOCK_INDEX};
//
//		GLint uniformCount = 0;
//		glGetProgramInterfaceiv(handle, GL_UNIFORM, GL_ACTIVE_RESOURCES, &uniformCount);
//
//#ifdef VR_INTERNAL
//		VR_ENGINE_LOGINFO("Active uniforms:");
//#else
//		VR_LOGINFO("Active uniforms:");
//#endif
//
//		for (int i = 0; i < uniformCount; i++) {
//			GLint results[4];
//			glGetProgramResourceiv(handle, GL_UNIFORM, i, 4, properties, 4, nullptr, results);
//
//			if (results[3] != -1) continue;
//
//			GLint nameBuffSize = results[0] + 1;
//			char* name = new char[nameBuffSize];
//
//			glGetProgramResourceName(handle, GL_UNIFORM, i, nameBuffSize, nullptr, name);
//#ifdef VR_INTERNAL
//			VR_ENGINE_LOGINFO(std::to_string(i) + " " + name);
//#else
//			VR_LOGINFO(std::to_string(i) + " " + name);
//#endif
//
//			delete[] name;
//		}
	}

}
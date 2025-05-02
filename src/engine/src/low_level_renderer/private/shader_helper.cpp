#include "shader_helper.h"

#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/SpvTools.h>

#include "core/logger/logger.h"

namespace graphics {
std::vector<uint32_t> compileFromGLSLToSPIRV(
	const UncompiledShader& uncompiledShader,
	std::span<const std::string> defines
) {
	// From Andrew Huang:
	// https://www.andrewhuang.llc/vulkan/integrating-glslang-for-runtime-shader-compilation/

	const EShLanguage language = [&]() {
		switch (uncompiledShader.stage) {
			case vk::ShaderStageFlagBits::eVertex:	 return EShLangVertex;
			case vk::ShaderStageFlagBits::eFragment: return EShLangFragment;
			case vk::ShaderStageFlagBits::eCompute:	 return EShLangCompute;
			default:
				LLOG_ERROR << "Shader stage "
						   << vk::to_string(uncompiledShader.stage)
						   << " shader compilation not supported";
				__builtin_unreachable();
		}
	}();

	const char* codeAsCString = uncompiledShader.code.data();
	constexpr int defaultVersion = 100;

	glslang::TShader shader(language);
	shader.setStrings(&codeAsCString, 1);

	const std::string preamble = [&]() {
		std::string result;
		for (std::string_view define : defines) {
			result += "#define ";
			result += define;
			result += "\n";
		}
		return result;
	}();
	shader.setPreamble(preamble.c_str());

	shader.setEnvInput(
		glslang::EShSourceGlsl,
		language,
		glslang::EShClientVulkan,
		defaultVersion
	);
	shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
	shader.setEnvTarget(glslang::EshTargetSpv, glslang::EShTargetSpv_1_0);

	shader.parse(GetDefaultResources(), defaultVersion, false, EShMsgDefault);

    if (shader.getInfoDebugLog()) {
	    LLOG_INFO << "Parsing shader " << shader.getInfoDebugLog();
    }

	glslang::TProgram program;
	program.addShader(&shader);
	program.link(EShMsgDefault);

    if (program.getInfoDebugLog()) {
	    LLOG_INFO << "Link program " << program.getInfoDebugLog();
    }

	const glslang::TIntermediate* intermediate =
		program.getIntermediate(language);
	std::vector<uint32_t> spriv;
	glslang::GlslangToSpv(*intermediate, spriv);

	return spriv;
}
};	// namespace graphics

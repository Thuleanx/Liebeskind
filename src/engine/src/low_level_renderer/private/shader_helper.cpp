#include "shader_helper.h"

#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/SPIRV/SpvTools.h>

#include "core/logger/logger.h"

namespace graphics {
std::vector<uint32_t> compileFromGLSLToSPIRV(
	std::string_view code, vk::ShaderStageFlagBits shaderStage
) {
	// From Andrew Huang:
	// https://www.andrewhuang.llc/vulkan/integrating-glslang-for-runtime-shader-compilation/

	const EShLanguage language = [&]() {
		switch (shaderStage) {
			case vk::ShaderStageFlagBits::eVertex:	 return EShLangVertex;
			case vk::ShaderStageFlagBits::eFragment: return EShLangFragment;
			case vk::ShaderStageFlagBits::eCompute:	 return EShLangCompute;
			default:
				LLOG_ERROR << "Shader stage " << vk::to_string(shaderStage)
						   << " shader compilation not supported";
				__builtin_unreachable();
		}
	}();

	const char* codeAsCString = code.data();
    constexpr int defaultVersion = 100;

	glslang::TShader shader(language);
	shader.setStrings(&codeAsCString, 1);


	shader.setEnvInput(
		glslang::EShSourceGlsl, language, glslang::EShClientVulkan, defaultVersion
	);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
    shader.setEnvTarget(glslang::EshTargetSpv, glslang::EShTargetSpv_1_0);

    shader.parse(GetDefaultResources(), defaultVersion, false, EShMsgDefault);

    LLOG_INFO << "Parsing shader " << shader.getInfoLog();
    
    glslang::TProgram program;
    program.addShader(&shader);
    program.link(EShMsgDefault);

    LLOG_INFO << "Link program: " << program.getInfoLog();

    const glslang::TIntermediate* intermediate = program.getIntermediate(language);
    std::vector<uint32_t> spriv;
    glslang::GlslangToSpv(*intermediate, spriv);

	return spriv;
}
};	// namespace graphics

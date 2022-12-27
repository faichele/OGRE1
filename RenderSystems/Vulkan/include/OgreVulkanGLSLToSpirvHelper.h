#ifndef OGREVULKANGLSLTOSPIRVHELPER_H
#define OGREVULKANGLSLTOSPIRVHELPER_H

#include "glslang/SPIRV/GlslangToSpv.h"

#include "OgreVulkanPrerequisites.h"
#include "vulkan/vulkan_core.h"

namespace Ogre
{
    class OgreVulkanGLSLToSpirvHelper
    {
        public:
            static void Init();
            static void Finalize();

            static void InitResources(TBuiltInResource &Resources);
            static EShLanguage FindLanguage(const VkShaderStageFlagBits shader_type);

            static bool GLSLtoSPV(const VkShaderStageFlagBits shader_type, const char *pshader, std::vector<unsigned int> &spirv);
    };
}

#endif // OGREVULKANGLSLTOSPIRVHELPER_H

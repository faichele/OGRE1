#ifndef _OgreVulkanGLSLProgram_H_
#define _OgreVulkanGLSLProgram_H_

#include "OgreVulkanPrerequisites.h"

#include "OgreGpuProgramManager.h"

namespace Ogre
{
    /** Specialisation of GpuProgram to provide support for GLSL to
     *  SPIRV translation
    */
    class VulkanGLSLProgram : public GpuProgram
    {
        public:
            VulkanGLSLProgram(ResourceManager *creator, const String &name, ResourceHandle handle,
                                     const String &group, bool isManual, ManualResourceLoader *loader,
                                     VulkanDevice *device );
            virtual ~VulkanGLSLProgram();

            const String &getLanguage( void ) const override;
            GpuProgramParametersSharedPtr createParameters( void ) override;

            VkPipelineShaderStageCreateInfo getPipelineShaderStageCi() const;

            uint32 getDrawIdLocation() const { return mDrawIdLocation; }

        private:
            void loadFromSource() override;
            void unloadImpl() override;

            VulkanDevice *mDevice;
            VkShaderModule mShaderModule;
            uint32 mDrawIdLocation;
    };

    /** Factory class for Vulkan programs. */
    class VulkanGLSLProgramFactory : public GpuProgramFactory
    {
        protected:
            String mLanguageName;
            VulkanDevice *mDevice;

        public:
            VulkanGLSLProgramFactory( VulkanDevice *device);
            virtual ~VulkanGLSLProgramFactory( void );
            /// Get the name of the language this factory creates programs for
            const String &getLanguage( void ) const;
            /// Create an instance of VulkanProgram
            GpuProgram *create( ResourceManager *creator, const String &name, ResourceHandle handle,
                                         const String &group, bool isManual, ManualResourceLoader *loader ) override;
    };
} // namespace Ogre

#endif // _OgreVulkanGLSLProgram_H_

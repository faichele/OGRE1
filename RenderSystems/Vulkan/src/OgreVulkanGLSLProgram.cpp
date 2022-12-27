#include "OgreVulkanGLSLProgram.h"

#include "OgreLogManager.h"
#include "OgreProfiler.h"
#include "OgreVulkanDevice.h"
#include "OgreVulkanMappings.h"

#include "OgreStringConverter.h"
#include "OgreVulkanUtils.h"

#include "OgreRenderSystemCapabilities.h"
#include "OgreRoot.h"

#include "OgreVulkanGLSLToSpirvHelper.h"

namespace Ogre
{
    VulkanGLSLProgram::VulkanGLSLProgram(ResourceManager *creator, const String &name, ResourceHandle handle,
                             const String &group, bool isManual, ManualResourceLoader *loader,
                             VulkanDevice *device ):
      GpuProgram( creator, name, handle, group, isManual, loader ),
      mDevice( device )
    {

    }

    VulkanGLSLProgram::~VulkanGLSLProgram()
    {

    }

    //-----------------------------------------------------------------------
    void VulkanGLSLProgram::loadFromSource()
    {
        mCompileError = false;

        VkShaderModuleCreateInfo moduleCi = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        moduleCi.codeSize = mSource.size();
        moduleCi.pCode = (const uint32*)mSource.data();
        OGRE_VK_CHECK(vkCreateShaderModule(mDevice->mDevice, &moduleCi, 0, &mShaderModule));

        setObjectName(mDevice->mDevice, (uint64_t)mShaderModule, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT,
                      mName.c_str());
    }

    //---------------------------------------------------------------------------
    void VulkanGLSLProgram::unloadImpl()
    {
        vkDestroyShaderModule( mDevice->mDevice, mShaderModule, 0 );
        mShaderModule = VK_NULL_HANDLE;
    }

    GpuProgramParametersSharedPtr VulkanGLSLProgram::createParameters( void )
    {
        GpuProgramParametersSharedPtr params = GpuProgram::createParameters();
        params->setTransposeMatrices( true );
        return params;
    }

    //-----------------------------------------------------------------------
    static VkShaderStageFlagBits get( GpuProgramType programType )
    {
        switch( programType )
        {
            // clang-format off
            case GPT_VERTEX_PROGRAM:    return VK_SHADER_STAGE_VERTEX_BIT;
            case GPT_FRAGMENT_PROGRAM:  return VK_SHADER_STAGE_FRAGMENT_BIT;
            case GPT_GEOMETRY_PROGRAM:  return VK_SHADER_STAGE_GEOMETRY_BIT;
            case GPT_HULL_PROGRAM:      return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
            case GPT_DOMAIN_PROGRAM:    return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
            case GPT_COMPUTE_PROGRAM:   return VK_SHADER_STAGE_COMPUTE_BIT;
            // clang-format on
        }
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    VkPipelineShaderStageCreateInfo VulkanGLSLProgram::getPipelineShaderStageCi() const
    {
        VkPipelineShaderStageCreateInfo ret = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        ret.stage = get( mType );
        ret.module = mShaderModule;
        ret.pName = "main";
        return ret;
    }

    //-----------------------------------------------------------------------
    const String &VulkanGLSLProgram::getLanguage( void ) const
    {
        static const String language = "glsl";
        return language;
    }

    //-----------------------------------------------------------------------
    VulkanGLSLProgramFactory::VulkanGLSLProgramFactory( VulkanDevice *device ) :
        mDevice( device )
    {

    }
    //-----------------------------------------------------------------------
    VulkanGLSLProgramFactory::~VulkanGLSLProgramFactory( void )
    {

    }
    //-----------------------------------------------------------------------
    const String& VulkanGLSLProgramFactory::getLanguage(void) const
    {
        static String language = "glsl";
        return language;
    }
    //-----------------------------------------------------------------------
    GpuProgram *VulkanGLSLProgramFactory::create( ResourceManager *creator, const String &name,
                                                       ResourceHandle handle, const String &group,
                                                       bool isManual, ManualResourceLoader *loader )
    {
        return OGRE_NEW VulkanGLSLProgram(creator, name, handle, group, isManual, loader, mDevice);
    }
}

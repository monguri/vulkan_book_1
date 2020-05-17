#pragma once

#include "../commonSutraCopy/vkappbase.h"

class TriangleApp : public VulkanAppBase
{
protected:
	virtual void prepare() override;
	virtual void cleanup() override;
	virtual void makeCommand(VkCommandBuffer command) override;
};


#pragma once

#include "ShaderPackage.h"

class ShaderPackageSerializer
{
public:
	static void Serialize(const std::string& filepath, const ShaderPackage& shaderPackage);
	static bool Deserialize(ShaderPackage& shaderPackage, const std::string& filepath);
};
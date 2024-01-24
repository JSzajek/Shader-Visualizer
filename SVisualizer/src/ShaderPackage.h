#pragma once

#include "Elysium/Core/Memory.h"

#include "Elysium/Math/iVec2.h"
#include "Elysium/Graphics/Shader.h"

#include <string>
#include <array>

struct ShaderPackage
{
public:
	ShaderPackage()
		: Dimensions(800, 600),
		Gamma(2.2f),
		Exposure(1.0f),
		BloomEnabled(false),
		Shader(nullptr)
	{
	}
public:
	void Reset()
	{
		Dimensions.x = 800;
		Dimensions.y = 600;
		Gamma = 2.2f;
		Exposure = 1.0f;
		BloomEnabled = false;
		Shader = nullptr;
	}
public:
	Elysium::Math::iVec2 Dimensions;
	
	float Gamma;
	float Exposure;
	bool BloomEnabled;

	std::array<std::string, 8> Textures;

	std::string Code;
	Elysium::Shared<Elysium::Shader> Shader;
};
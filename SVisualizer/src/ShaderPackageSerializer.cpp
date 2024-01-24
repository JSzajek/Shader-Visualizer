#include "svis_pch.h"
#include "ShaderPackageSerializer.h"

#include "Elysium/Utils/YamlUtils.h"

#include <fstream>

void ShaderPackageSerializer::Serialize(const std::string& filepath, const ShaderPackage& shaderPackage)
{
	YAML::Emitter out;

	out << YAML::BeginMap;

	out << YAML::Key << "Render_Settings" << YAML::BeginMap;
	out << YAML::Key << "Width" << YAML::Value << shaderPackage.Dimensions.width;
	out << YAML::Key << "Height" << YAML::Value << shaderPackage.Dimensions.height;
	out << YAML::Key << "Bloom" << YAML::Value << shaderPackage.BloomEnabled;
	out << YAML::Key << "Gamma" << YAML::Value << shaderPackage.Gamma;
	out << YAML::Key << "Exposure" << YAML::Value << shaderPackage.Exposure;
	out << YAML::EndMap;
	
	out << YAML::Key << "Textures" << YAML::Value << YAML::BeginSeq;
	for (uint8_t i = 0; i < shaderPackage.Textures.size(); ++i)
	{
		out << YAML::BeginMap;
		out << YAML::Key << "Slot" << YAML::Value << static_cast<int>(i);
		out << YAML::Key << "Path" << YAML::Value << shaderPackage.Textures[i];
		out << YAML::EndMap;
	}
	out << YAML::EndSeq;

	out << YAML::Key << "Code" << YAML::Value << shaderPackage.Code;

	out << YAML::EndMap;

	std::ofstream stream(filepath);
	stream << out.c_str();
}

bool ShaderPackageSerializer::Deserialize(ShaderPackage& shaderPackage, const std::string& filepath)
{
	auto data = YAML::LoadFile(filepath);
	if (!data["Render_Settings"])
		return false;

	auto renderSettings = data["Render_Settings"];
	if (renderSettings)
	{
		shaderPackage.Dimensions.width = renderSettings["Width"].as<int>();
		shaderPackage.Dimensions.height = renderSettings["Height"].as<int>();
		shaderPackage.BloomEnabled = renderSettings["Bloom"].as<bool>();
		shaderPackage.Gamma = renderSettings["Gamma"].as<float>();
		shaderPackage.Exposure = renderSettings["Exposure"].as<float>();
	}

	auto textures = data["Textures"];
	if (textures)
	{
		for (auto texture : textures)
		{
			int slot = texture["Slot"].as<int>();
			shaderPackage.Textures[slot] = texture["Path"].as<std::string>();
		}
	}

	auto code = data["Code"];
	if (code)
		shaderPackage.Code = code.as<std::string>();

	return true;
}

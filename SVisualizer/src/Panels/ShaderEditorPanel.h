#pragma once

#include "Elysium.h"

class TextEditor;

class ShaderEditorPanel
{
public:
	ShaderEditorPanel();
	~ShaderEditorPanel();
public:
	void OnUpdate();
	void OnImGuiRender();
public:
	void GetCurrentShader(Elysium::Shared<Elysium::Shader>& output);

	void NewFile();
	void OpenFile();
	void SaveFile();
	void Compile();
private:
	void LoadFromFile();
	void SaveAsFile();
	void SaveCurrentCode();
	
	void ResetShader();
	void LoadCodeFromFile(const std::string& filepath = "");
	void CompileShader();

	void AddTextureSlot();
public:
	std::string m_baseShaderCode;
	std::string m_defaultPixelShaderCode;

	Elysium::Shared<Elysium::Shader> m_currentShader;
	std::string m_currentShaderCode;
	std::string m_savedShaderCode;
	bool m_shaderCompileRequested;
	bool m_shaderCompiled;

	bool m_textChanged;
	bool m_textFileChanged;

	std::string m_currentFile;
	std::string m_currentFileName;

	Elysium::Unique<TextEditor> m_textEditor;

	bool m_imageEditorVisible;

	struct LoadedImages
	{
	public:
		static constexpr uint8_t MaxNumImages = 8;
	public:
		LoadedImages();
	public:
		void TryAddToSlot(uint8_t slot);
		void RemoveSlot(uint8_t slot);
	public:
		std::array<Elysium::Shared<Elysium::Texture2D>, 8> m_textures;
		std::array<std::string, 8> m_filenames;
	};
	LoadedImages m_loadedImages;
};
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
};
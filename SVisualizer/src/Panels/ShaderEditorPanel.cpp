#include "svis_pch.h"

#include "Panels/ShaderEditorPanel.h"

#include "Elysium/Utils/FileUtils.h"
#include "Elysium/Factories/ShaderFactory.h"

#include <TextEditor.h>
#include <imgui_internal.h>

ShaderEditorPanel::ShaderEditorPanel()
	: m_currentShader(nullptr),
	m_currentShaderCode(),
	m_savedShaderCode(),
	m_shaderCompileRequested(false),
	m_shaderCompiled(false),
	m_textChanged(false),
	m_textFileChanged(false),
	m_currentFile("null"), 
	m_currentFileName()
{
	// Initialize the text editor
	m_textEditor = Elysium::CreateUnique<TextEditor>();
	
	auto textLanguage = TextEditor::LanguageDefinition::GLSL();
	m_textEditor->SetLanguageDefinition(textLanguage);
	
	m_textEditor->SetPalette(TextEditor::GetDarkPalette());
	m_textEditor->SetShowWhitespaces(false);
	
	const std::string solvedFilepath = Elysium::FileUtils::GetAssetPath_Str("Content/shaders/default.shader");
	std::ifstream defaultShaderStream(solvedFilepath);
	if (defaultShaderStream.good())
		m_baseShaderCode = std::move(std::string((std::istreambuf_iterator<char>(defaultShaderStream)), std::istreambuf_iterator<char>()));
	else
		ELYSIUM_ERROR("Error Opening Default Shader File!");

	m_defaultPixelShaderCode = "void PixelProcess(out vec4 pColor)\n{\n\tpColor = vec4(UVS.x, UVS.y, 0, 1.0);\n}";

	ResetShader();
}

ShaderEditorPanel::~ShaderEditorPanel()
{
	if (m_textFileChanged && !m_currentFile.empty())
	{
		Elysium::FileDialogs::DialogResult result = Elysium::FileDialogs::YesNoMessage("Save Changes Before Closing?", m_currentFileName.c_str());
		if (result == Elysium::FileDialogs::DialogResult::Yes)
			SaveFile();
	}
}

void ShaderEditorPanel::OnUpdate()
{
	std::string currentText = m_textEditor->GetText();
	if (currentText != m_currentShaderCode)
		m_textChanged = true;

	if (currentText != m_savedShaderCode)
		m_textFileChanged = true;

	if (m_shaderCompileRequested)
	{
		CompileShader();
		m_shaderCompileRequested = false;
		m_textChanged = false;

		if (!m_currentFile.empty())
			SaveCurrentCode();
	}
}

void ShaderEditorPanel::OnImGuiRender()
{
	ImGuiWindowClass window_class;
	window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;
	ImGui::SetNextWindowClass(&window_class);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	if (!ImGui::Begin("Shader Editor Panel", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar))
		ImGui::End();

	ImGui::Spacing();

	// Compile, Save, Load ----------------------
	ImGui::Spacing();
	ImGui::SameLine();

	const float panelWidth = ImGui::GetWindowWidth();

	ImGui::Columns(3, "Controls", false);
	ImGui::SetColumnWidth(0, 250.f);
	ImGui::SetColumnWidth(1, std::max(10.0f, panelWidth - 250.f - 175.f));
	if (ImGui::Button(ICON_FA_PLAY_CIRCLE, ImVec2(50, 25)))
	{
		Compile();
	}

	ImVec4 statusColor;
	const char* statusIcon = "";
	if (m_shaderCompiled)
	{
		if (m_textChanged)
		{
			statusColor = ImVec4(0.8f, 0.8f, 0.2f, 1.f);
			statusIcon = ICON_FA_PEN_SQUARE;
		}
		else
		{
			statusColor = ImVec4(0.2f, 1.f, 0.2f, 1.f);
			statusIcon = ICON_FA_CHECK_SQUARE;
		}
	}
	else
	{
		statusColor = ImVec4(1.f, 0.2f, 0.2f, 1.f);
		statusIcon = ICON_FA_MINUS_SQUARE;
	}

	ImGui::SameLine();
	ImGui::TextColored(statusColor, "Status:");
	ImGui::SameLine();
	ImGui::TextColored(statusColor, statusIcon);
	ImGui::SameLine();

	std::stringstream filestatus;
	filestatus << "\tFile:" << m_currentFileName;

	ImVec4 fileStatusColor;
	if (m_textFileChanged)
	{
		fileStatusColor = ImVec4(0.8f, 0.8f, 0.2f, 1.f);
		filestatus << "*";
	}
	else
	{
		fileStatusColor = ImVec4(0.2f, 1.f, 0.2f, 1.f);
	}
	ImGui::Spacing();
	ImGui::SameLine();
	ImGui::TextColored(fileStatusColor, filestatus.str().c_str());
	if (!m_currentFile.empty() && ImGui::IsItemHovered())
		ImGui::SetTooltip(m_currentFile.c_str());

	ImGui::NextColumn();

	ImGui::NextColumn();
	if (ImGui::Button(ICON_FA_FILE, ImVec2(50, 25)))
	{
		NewFile();
	}
	ImGui::SameLine();
	if (ImGui::Button(ICON_FA_SAVE, ImVec2(50, 25)))
	{
		SaveFile();
	}
	ImGui::SameLine();
	
	if (ImGui::Button(ICON_FA_FOLDER_OPEN, ImVec2(50, 25)))
	{
		OpenFile();
	}
	// ------------------------------------------

	ImGui::Columns(1);

	ImGui::Separator();

	// Shader Input Dropdown
	if (ImGui::CollapsingHeader("Pre-Defines"))
	{
		ImGui::BulletText("Math_PI [float]: mathematical pi constant.");
		ImGui::BulletText("Math_TAU [float]: mathematical tau (2*pi) constant.");
		ImGui::BulletText("UVS [vec2]: uv texture coordinates.");
		ImGui::BulletText("PIXCOORD [vec2]: pixel coordinates.");
		ImGui::BulletText("RESOLUTION [vec2]: output resolution.");
		ImGui::BulletText("TIME [float]: shader time value.");
	}

	ImGui::Separator();

	// Shader Text Editor
	m_textEditor->Render("TextEditor");

	ImGui::End();
	ImGui::PopStyleVar();
}

void ShaderEditorPanel::GetCurrentShader(Elysium::Shared<Elysium::Shader>& output)
{
	output = m_currentShader;
}

void ShaderEditorPanel::NewFile()
{
	if (m_textFileChanged && !m_currentFile.empty())
	{
		Elysium::FileDialogs::DialogResult result = Elysium::FileDialogs::YesNoCancelMessage("Save Changes Before Closing?", m_currentFileName.c_str());
		if (result == Elysium::FileDialogs::DialogResult::Yes)
			SaveFile();
		else if (result == Elysium::FileDialogs::DialogResult::Cancel)
			return;
	}

	m_currentFile = "";
	m_currentFileName = "Untitled";

	m_currentShaderCode = m_defaultPixelShaderCode;
	m_textFileChanged = true;

	ResetShader();
}

void ShaderEditorPanel::OpenFile()
{
	LoadFromFile();
	CompileShader();
}

void ShaderEditorPanel::SaveFile()
{
	if (m_currentFile.empty())
		SaveAsFile();
	else
		SaveCurrentCode();
}

void ShaderEditorPanel::Compile()
{
	m_currentShaderCode = m_textEditor->GetText();
	m_shaderCompileRequested = true;
}

void ShaderEditorPanel::LoadFromFile()
{
	const std::string filepath = Elysium::FileDialogs::OpenFile("Pixel Shader (*.pshader)\0*.pshader\0");

	if (Elysium::FileUtils::FileExists(filepath))
		LoadCodeFromFile(filepath);

	m_textEditor->SetText(m_currentShaderCode);
	m_savedShaderCode = m_textEditor->GetText();
	m_currentShaderCode = m_savedShaderCode;
	m_textFileChanged = false;
}

void ShaderEditorPanel::SaveAsFile()
{
	const std::string filepath = Elysium::FileDialogs::SaveFile("Pixel Shader (*.pshader)\0*.pshader\0");

	if (!filepath.empty())
	{
		m_currentFile = filepath;
		m_currentFileName = Elysium::FileUtils::GetFileName(m_currentFile);
	}
	SaveCurrentCode();
}

void ShaderEditorPanel::SaveCurrentCode()
{
	if (m_currentFile.empty())
		return;

	std::ofstream outfileStream(m_currentFile);
	if (outfileStream.is_open())
	{
		m_currentShaderCode = m_textEditor->GetText();

		outfileStream << m_currentShaderCode;
		outfileStream.close();

		m_savedShaderCode = m_currentShaderCode;
		m_textFileChanged = false;
	}
	else
	{
		ELYSIUM_WARN("Error saving to file: {0}", m_currentFile);
	}
}

void ShaderEditorPanel::ResetShader()
{
	LoadCodeFromFile();
	m_textEditor->SetText(m_currentShaderCode);
	m_currentShaderCode = m_textEditor->GetText();
	CompileShader();

	m_textChanged = false;
	m_textFileChanged = false;
}

void ShaderEditorPanel::LoadCodeFromFile(const std::string& filepath)
{
	if (m_currentFile == filepath)
		return;

	m_currentFile = filepath;
	m_currentShaderCode = "";

	if (m_currentFile.empty())
	{
		// Use the default
		m_currentShaderCode.append(m_defaultPixelShaderCode);
		m_currentFileName = "Untitled";
	}
	else
	{
		m_currentFileName = Elysium::FileUtils::GetFileName(m_currentFile);

		const std::string solvedCurrentFilepath = Elysium::FileUtils::GetAssetPath_Str(m_currentFile);
		std::ifstream filepathShaderStream(solvedCurrentFilepath);
		if (filepathShaderStream.good())
		{
			const std::string filepathShaderCode((std::istreambuf_iterator<char>(filepathShaderStream)), std::istreambuf_iterator<char>());
			m_currentShaderCode.append(filepathShaderCode);
		}
		else
		{
			// Use the default
			m_currentShaderCode.append(m_defaultPixelShaderCode);
		}
	}
}

void ShaderEditorPanel::CompileShader()
{
	std::stringstream shaderCode;
	shaderCode << m_baseShaderCode;
	shaderCode << m_currentShaderCode;

	// Compile this shader code
	std::string compileError;
	Elysium::Shared<Elysium::Shader> newShader = Elysium::ShaderFactory::CreateFromCode(shaderCode.str(), &compileError);
	if (newShader == nullptr)
	{
		ELYSIUM_WARN("Failed To Compile Shader: {0}", compileError);
		m_shaderCompiled = false;
	}
	else
	{
		m_currentShader = newShader;
		m_shaderCompiled = true;
	}
}

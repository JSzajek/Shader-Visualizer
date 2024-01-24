#include "svis_pch.h"

#include "Panels/ShaderEditorPanel.h"

#include "Elysium/Utils/FileUtils.h"
#include "Elysium/Factories/ShaderFactory.h"
#include "Elysium/Renderer/RendererBase.h"

#include "ShaderPackage.h"
#include "ShaderPackageSerializer.h"

#include <TextEditor.h>
#include <imgui_internal.h>

ShaderEditorPanel::ShaderEditorPanel(ShaderPackage* package)
	: m_package(package),
	m_savedShaderCode(),
	m_shaderCompileRequested(false),
	m_shaderCompiled(false),
	m_textChanged(false),
	m_textFileChanged(false),
	m_currentFile(), 
	m_currentFileName(),
	m_imageEditorVisible(false)
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
	if (currentText != m_package->Code)
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
	ImGui::SetColumnWidth(0, 275.f);
	ImGui::SetColumnWidth(1, std::max(10.0f, panelWidth - 275.f - 200.f));
	if (ImGui::Button(ICON_FA_PLAY_CIRCLE, ImVec2(40, 25)))
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

	if (ImGui::Button(ICON_FA_IMAGE, ImVec2(40, 25)))
	{
		m_imageEditorVisible = !m_imageEditorVisible;
	}
	ImGui::SameLine();
	if (ImGui::Button(ICON_FA_FILE, ImVec2(40, 25)))
	{
		NewFile();
	}
	ImGui::SameLine();
	if (ImGui::Button(ICON_FA_SAVE, ImVec2(40, 25)))
	{
		SaveFile();
	}
	ImGui::SameLine();
	
	if (ImGui::Button(ICON_FA_FOLDER_OPEN, ImVec2(40, 25)))
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

	if (m_imageEditorVisible)
	{
		ImGui::SetNextWindowClass(&window_class);
		if (!ImGui::Begin("Images", &m_imageEditorVisible, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | 
														   ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoResize))
			ImGui::End();

		const float imagesPanelWidth = ImGui::GetWindowWidth();

		// Images Group -----------------------------
		ImGui::PushID("##ImagesGroup");
		ImGui::BeginGroup();

		const ImGuiWindowFlags child_flags = ImGuiWindowFlags_MenuBar;
		const ImGuiID child_id = ImGui::GetID((void*)(intptr_t)0);
		const bool child_is_visible = ImGui::BeginChild(child_id, ImVec2(imagesPanelWidth, 260.0f), true, child_flags);
		if (ImGui::BeginMenuBar())
		{
			ImGui::TextUnformatted("Slots");
			ImGui::EndMenuBar();
		}
		ImGui::Spacing();

		ImGui::Columns(3, "SlotsColumns", false);
		ImGui::SetColumnWidth(0, 400.f);
		ImGui::SetColumnWidth(1, imagesPanelWidth - 400.f - 115.f);

		ImVec4 unselectedColor(0.6f, 0.8f, 0.8f, 1.0f);
		for (uint8_t i = 0; i < LoadedImages::MaxNumImages; ++i)
		{
			Elysium::Shared<Elysium::Texture2D> tex = m_loadedImages.m_textures[i];
			ImGui::Text("%i.", i);
			ImGui::SameLine();

			if (tex != nullptr)
			{
				ImGui::Text(m_loadedImages.m_filenames[i].c_str());
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip(tex->GetName().c_str());
			}
			else
			{
				ImGui::TextColored(unselectedColor, "[empty]");
			}

			ImGui::NextColumn();
			ImGui::NextColumn();
			if (ImGui::Button(ICON_FA_PLUS, ImVec2(40, 25)))
			{
				if (m_loadedImages.TryAddToSlot(i))
					m_package->Textures[i] = m_loadedImages.m_textures[i]->GetName();
			}
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_TRASH, ImVec2(40, 25)))
			{
				m_loadedImages.RemoveSlot(i);
				m_package->Textures[i] = "";
			}

			ImGui::NextColumn();
		}	

		ImGui::EndChild();
		ImGui::EndGroup();
		ImGui::PopID();
		// ---------------------------------------------------

		ImGui::End();
	}

	ImGui::PopStyleVar();
}

void ShaderEditorPanel::GetCurrentShader(Elysium::Shared<Elysium::Shader>& output)
{
	// Bind the loaded images to the correct slots
	m_package->Shader->Bind();
	for (uint8_t i = 0; i < LoadedImages::MaxNumImages; ++i)
	{
		Elysium::Shared<Elysium::Texture2D> tex = m_loadedImages.m_textures[i];
		if (tex != nullptr)
			tex->Bind(i);
		else
			Elysium::GlobalRendererBase::GetDefaultTexture()->Bind(i);
	}
	m_package->Shader->Unbind();

	output = m_package->Shader;
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

	m_package->Reset();

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
	m_package->Code = m_textEditor->GetText();
	m_shaderCompileRequested = true;
}

void ShaderEditorPanel::LoadFromFile()
{
	const std::string filepath = Elysium::FileDialogs::OpenFile("Pixel Shader (*.pshader)\0*.pshader\0");

	if (m_currentFile == filepath)
		return;

	if (Elysium::FileUtils::FileExists(filepath))
	{
		m_currentFile = filepath;
		if (ShaderPackageSerializer::Deserialize(*m_package, m_currentFile))
		{
			for (uint8_t i = 0; i < LoadedImages::MaxNumImages; ++i)
				m_loadedImages.ForceAddToSlot(i, m_package->Textures[i]);
		}

		m_currentFileName = Elysium::FileUtils::GetFileName(m_currentFile);

		m_textEditor->SetText(m_package->Code);
		m_savedShaderCode = m_textEditor->GetText();
		m_textFileChanged = false;
		m_shaderCompileRequested = true;
	}
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

	m_package->Code = m_textEditor->GetText();

	ShaderPackageSerializer::Serialize(m_currentFile, *m_package);

	m_savedShaderCode = m_package->Code;
	m_textFileChanged = false;

#if 0
	std::ofstream outfileStream(m_currentFile);
	if (outfileStream.is_open())
	{
		m_package->Code = m_textEditor->GetText();

		outfileStream << m_package->Code;
		outfileStream.close();

		m_savedShaderCode = m_package->Code;
		m_textFileChanged = false;
	}
	else
	{
		ELYSIUM_WARN("Error saving to file: {0}", m_currentFile);
	}
#endif
}

void ShaderEditorPanel::ResetShader()
{
	m_package->Code = m_defaultPixelShaderCode;

	m_currentFileName = "Untitled";
	m_textEditor->SetText(m_package->Code);
	m_package->Code = m_textEditor->GetText();
	CompileShader();

	m_textChanged = false;
	m_textFileChanged = false;
}

void ShaderEditorPanel::CompileShader()
{
	std::stringstream shaderCode;
	shaderCode << m_baseShaderCode;
	shaderCode << m_package->Code;

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
		m_package->Shader = newShader;
		m_shaderCompiled = true;

		// Rebind texture slots
		int samplers[LoadedImages::MaxNumImages];
		for (int i = 0; i < LoadedImages::MaxNumImages; ++i)
			samplers[i] = i;

		m_package->Shader->Bind();
		m_package->Shader->SetIntArray("textureMaps", samplers, LoadedImages::MaxNumImages);
		m_package->Shader->Unbind();
	}
}

void ShaderEditorPanel::LoadedImages::ForceAddToSlot(uint8_t slot, const std::string& filepath)
{
	if (Elysium::FileUtils::FileExists(filepath))
	{
		m_filenames[slot] = Elysium::FileUtils::GetFileName(filepath, true);
		m_textures[slot] = Elysium::Texture2D::Create(filepath);
	}
}

bool ShaderEditorPanel::LoadedImages::TryAddToSlot(uint8_t slot)
{
	const std::string textureFilepath = Elysium::FileDialogs::OpenFile("PNG Image (*.png)\0*.png\0"
																	   "JPEG Image (*.jpg, *.jpeg, *.jpe)\0*.jpg;*.jpeg;*.jpe\0");
	if (Elysium::FileUtils::FileExists(textureFilepath))
	{
		m_filenames[slot] = Elysium::FileUtils::GetFileName(textureFilepath, true);
		m_textures[slot] = Elysium::Texture2D::Create(textureFilepath);
		return true;
	}
	return false;
}

void ShaderEditorPanel::LoadedImages::RemoveSlot(uint8_t slot)
{
	m_textures[slot] = nullptr;
	m_filenames[slot] = "";
}

ShaderEditorPanel::LoadedImages::LoadedImages()
{
	for (uint8_t i = 0; i < LoadedImages::MaxNumImages; ++i)
	{
		m_textures[i] = nullptr;
		m_filenames[i] = "";
	}
}

#include "svis_pch.h"
#include "ViewerPanel.h"

#include "Elysium.h"
#include "Elysium/Factories/ShaderFactory.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <opencv2/opencv.hpp>

ViewerPanel::ViewerPanel()
	: m_size(1, 1), 
	m_desiredSize(800, 600),
	m_outputSize(1, 1),
	m_outputSizeChanged(false),
	m_orthoSize(500.f),
	m_zoomModifier(10.f),
	m_focused(false),
	m_hovered(false),
	m_currentTime(0),
	m_useBloom(false),
	m_gamma(1.1f),
	m_exposure(1.0f),
	m_playing(true),
	m_settingsVisible(false),
	m_debugPass(DrawPass::None)
{
	Elysium::FrameBufferSpecification bufferspecs;
	bufferspecs.Attachments = { Elysium::FrameBufferTextureFormat::RGBA8 };
	bufferspecs.Width = m_size.x;
	bufferspecs.Height = m_size.y;
	bufferspecs.SwapChainTarget = false;

	m_fbo = Elysium::FrameBuffer::Create(bufferspecs);
	m_shaderfbo = Elysium::FrameBuffer::Create(bufferspecs);

	Elysium::FrameBufferSpecification hdrbufferspecs;
	hdrbufferspecs.Attachments = { 
		Elysium::FrameBufferTextureFormat::RGBA16F,
		Elysium::FrameBufferTextureFormat::RGBA16F 
	};
	hdrbufferspecs.Width = m_size.x;
	hdrbufferspecs.Height = m_size.y;
	hdrbufferspecs.SwapChainTarget = false;
	m_hdrfbo = Elysium::FrameBuffer::Create(hdrbufferspecs);

	Elysium::FrameBufferSpecification bloombufferspecs = hdrbufferspecs;
	bloombufferspecs.Attachments = {
		Elysium::FrameBufferTextureFormat::RGBA16F
	};
	m_bloomFbos[0] = Elysium::FrameBuffer::Create(bloombufferspecs);
	m_bloomFbos[1] = Elysium::FrameBuffer::Create(bloombufferspecs);

	m_camera = Elysium::CreateShared<Elysium::OrthographicCamera>();

	m_scene = Elysium::CreateShared<Elysium::Scene>();

	m_sprite = m_scene->CreateEntity("Sprite_01");
	auto& rectComp = m_sprite.AddComponent<Elysium::RectTransformComponent>();
	auto& spriteComp = m_sprite.AddComponent<Elysium::SpriteComponent>();

	spriteComp.Texture = Elysium::Texture2D::Create(m_shaderfbo->GetColorAttachementRendererID(), 0u, 0u);

	m_spriteShader = Elysium::ShaderFactory::Create("Content/Engine/shaders/default_sprite.shader");
	m_blurShader = Elysium::ShaderFactory::Create("Content/shaders/blur.shader");
	m_bloomShader = Elysium::ShaderFactory::Create("Content/shaders/bloom.shader");
	m_debugShader = Elysium::ShaderFactory::Create("Content/shaders/debug.shader");

	int samplers[Elysium::RendererCaps::MaxTextureSlots];
	for (int i = 0; i < Elysium::RendererCaps::MaxTextureSlots; ++i)
		samplers[i] = i;

	m_spriteShader->Bind();
	m_spriteShader->SetIntArray("textureMaps", samplers, Elysium::RendererCaps::MaxTextureSlots);
	m_spriteShader->Unbind();

	ELYSIUM_CORE_ASSERT(m_spriteShader->IsCompiled(), "Sprite Shader Failed to Compile.");
	ELYSIUM_CORE_ASSERT(m_blurShader->IsCompiled(), "Blur Shader Failed to Compile.");
	ELYSIUM_CORE_ASSERT(m_bloomShader->IsCompiled(), "Bloom Shader Failed to Compile.");
	ELYSIUM_CORE_ASSERT(m_debugShader->IsCompiled(), "Debug Shader Failed to Compile.");

	UpdateCameraView();
	UpdateCameraProjection();
}

void ViewerPanel::OnUpdate()
{
	if (m_desiredSize != m_size)
	{
		m_size = m_desiredSize;
		const uint32_t clampSizeX = std::max(1, m_size.x);
		const uint32_t clampSizeY = std::max(1, m_size.y);

		m_shaderfbo->Resize(clampSizeX, clampSizeY);
		m_hdrfbo->Resize(clampSizeX, clampSizeY);
		m_bloomFbos[0]->Resize(clampSizeX, clampSizeY);
		m_bloomFbos[1]->Resize(clampSizeX, clampSizeY);

		auto& rectComp = m_sprite.GetComponent<Elysium::RectTransformComponent>();
		const Elysium::Math::Vec2 dim((float)m_size.x, (float)m_size.y);
		const Elysium::Math::Vec2 offset(m_size.x * 0.5f, m_size.y * 0.5f);
		rectComp.SetTranslation(-offset);
		rectComp.SetDimensions(dim);

		FocusCamera();
		UpdateCameraProjection();
	}

	if (m_outputSizeChanged)
	{
		uint32_t clampSizeX = std::max(1, m_outputSize.x);
		uint32_t clampSizeY = std::max(1, m_outputSize.y);
		m_fbo->Resize(clampSizeX, clampSizeY);

		m_outputSizeChanged = false;

		FocusCamera();
		UpdateCameraProjection();
	}
	
	if (m_playing)
	{
		m_scene->Computations();
		m_currentTime = Elysium::Time::TotalTime();
	}
}

void ViewerPanel::DrawTo(const Elysium::Shared<Elysium::Shader>& shader)
{
	if (shader)
	{
		if (m_useBloom)
		{
			Elysium::GraphicsCalls::ClearBuffers();
			Elysium::RenderCommands::DrawScreenShader(m_hdrfbo, shader);

			// Blur Pass - blur the bright color values
			bool horizontal = true;
			uint8_t amount = 10;
			for (uint8_t i = 0; i < amount; ++i)
			{
				m_blurShader->Bind();
				m_blurShader->SetInt("horizontal", horizontal);

				Elysium::GraphicsCalls::ClearBuffers();
				Elysium::RenderCommands::DrawTexture(m_bloomFbos[(int)horizontal], Elysium::RenderCommands::TextureDrawType::Color,
													 i == 0 ? m_hdrfbo->GetColorAttachment(1) : m_bloomFbos[(int)!horizontal]->GetColorAttachment(), 
													 m_blurShader);
				horizontal = !horizontal;
			}

			if (m_debugPass == DrawPass::None)
			{
				// Combination Pass - tonemap hdr color and blend to shader output.
				m_bloomShader->Bind();
				m_bloomShader->SetFloat("inputGamma", m_gamma);
				m_bloomShader->SetFloat("inputExposure", m_exposure);

				Elysium::GraphicsCalls::ClearBuffers();
				Elysium::RenderCommands::DrawTextures(m_shaderfbo, m_bloomShader,
													  { m_hdrfbo->GetColorAttachment(), m_bloomFbos[(int)!horizontal]->GetColorAttachment()});
			}
			else
			{
				Elysium::Shared<Elysium::Texture2D> debugTexToDraw = nullptr;
				if (m_debugPass == DrawPass::BrightPass)
					debugTexToDraw = m_hdrfbo->GetColorAttachment(1);
				else if (m_debugPass == DrawPass::BlurPass)
					debugTexToDraw = m_bloomFbos[(int)!horizontal]->GetColorAttachment();

				m_debugShader->Bind();
				m_debugShader->SetFloat("inputGamma", m_gamma);
				m_debugShader->SetFloat("inputExposure", m_exposure);

				Elysium::GraphicsCalls::ClearBuffers();
				Elysium::RenderCommands::DrawTexture(m_shaderfbo, Elysium::RenderCommands::TextureDrawType::Color, 
												     debugTexToDraw, m_debugShader);
			}
		}
		else
		{
			Elysium::GraphicsCalls::ClearBuffers();
			Elysium::RenderCommands::DrawScreenShader(m_shaderfbo, shader);
		}
	}

	// Draw Scene
	if (m_fbo)
	{
		m_fbo->Bind();
		m_spriteShader->Bind();

		Elysium::GraphicsCalls::ClearBuffers();
		Elysium::GraphicsCalls::SetColor(Elysium::Math::Vec4(0.3f));

		m_scene->Draw(Elysium::Scene::SceneMask::_2D);

		m_spriteShader->Unbind();
		m_fbo->Unbind();
	}
}

void ViewerPanel::OnImGuiRender()
{
	ImGuiWindowClass window_class;
	window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;
	ImGui::SetNextWindowClass(&window_class);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	if (!ImGui::Begin("Viewer Panel", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar))
		ImGui::End();

	const float panelWidth = ImGui::GetWindowWidth();
	const float panelHeight = ImGui::GetWindowHeight();

	ImGui::Spacing();

	ImGui::Columns(3, "Viewer Details", false);

	ImGui::SetColumnWidth(0, 105.f);
	ImGui::SetColumnWidth(1, std::max(10.0f, panelWidth - 105.f - 105.f));

	// Snapshot
	if (ImGui::Button(ICON_FA_CAMERA, ImVec2(40, 25)))
	{
		SnapShot();
	}
	ImGui::SameLine();

	// Re-focus
	if (ImGui::Button(ICON_FA_COMPRESS_ARROWS_ALT, ImVec2(40, 25)))
	{
		FocusCamera();
		UpdateCameraProjection();
	}

	ImGui::NextColumn();

	ImGui::NextColumn();

	if (ImGui::Button(m_playing ? ICON_FA_PAUSE : ICON_FA_PLAY, ImVec2(40, 25)))
	{
		m_playing = !m_playing;
	}
	ImGui::SameLine();
	if (ImGui::Button(ICON_FA_COG, ImVec2(40, 25)))
	{
		m_settingsVisible = !m_settingsVisible;
	}

	//ImGui::SameLine();
	//std::stringstream time_str;
	//time_str << "T:" << std::fixed << std::setprecision(2) << m_currentTime;
	//std::string time = time_str.str();
	//if (time.length() > 8)
	//	time = time.substr(0, 8);
	//ImGui::Text(time.c_str());

	ImGui::Columns(1);

	ImGui::Separator();

	const ImVec2 panelSize = ImGui::GetContentRegionAvail();
	ImGui::Image(reinterpret_cast<void*>(static_cast<uint64_t>(m_fbo->GetColorAttachementRendererID())), panelSize, ImVec2(0, 1), ImVec2(1, 0));
	
	if (m_outputSize.x != panelSize.x || m_outputSize.y != panelSize.y)
	{
		m_outputSize.x = static_cast<uint32_t>(panelSize.x);
		m_outputSize.y = static_cast<uint32_t>(panelSize.y);

		m_outputSizeChanged = true;
	}
	
	m_focused = ImGui::IsWindowFocused();
	m_hovered = ImGui::IsWindowHovered();

	ImGui::End();

	if (m_settingsVisible)
	{
		ImGui::SetNextWindowClass(&window_class);
		if (!ImGui::Begin("Settings", &m_settingsVisible, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking))
			ImGui::End();

		const float settingsPanelWidth = ImGui::GetWindowWidth();

		// Render Settings Group -----------------------------
		ImGui::PushID("##RenderSettingsGroup");
		ImGui::BeginGroup();

		const ImGuiWindowFlags child_flags = ImGuiWindowFlags_MenuBar;
		const ImGuiID child_id = ImGui::GetID((void*)(intptr_t)0);
		const bool child_is_visible = ImGui::BeginChild(child_id, ImVec2(settingsPanelWidth, 200.0f), true, child_flags);
		if (ImGui::BeginMenuBar())
		{
			ImGui::Text("Render Settings");

			ImGui::EndMenuBar();
		}
		ImGui::Spacing();

		ImVec4 titleColor(0.5f, 0.5f, 0.5f, 1.0f);

		ImGui::Spacing();
		ImGui::SameLine();
		ImGui::TextColored(titleColor, "Visual");
		ImGui::Separator();

		ImGui::Columns(2, "VisualSettingsColumns", false);
		ImGui::SetColumnWidth(0, 5);
		ImGui::NextColumn();

		ImGui::Text("Width:");
		ImGui::SameLine();
		ImGui::PushItemWidth(50.f);
		ImGui::InputScalar("##desired_width", ImGuiDataType_U32, &m_desiredSize.x);
		m_desiredSize.x = std::min(std::max(m_desiredSize.x, 1), 4096);
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::Text("Height:");
		ImGui::SameLine();
		ImGui::PushItemWidth(50.f);
		ImGui::InputScalar("##desired_height", ImGuiDataType_U32, &m_desiredSize.y);
		m_desiredSize.y = std::min(std::max(m_desiredSize.y, 1), 4096);
		ImGui::PopItemWidth();

		ImGui::Text("Bloom:");
		ImGui::SameLine();
		ImGui::Checkbox("##bloom", &m_useBloom);

		if (m_useBloom)
		{
			ImGui::Text("Gamma:");
			ImGui::SameLine();
			ImGui::PushItemWidth(75.f);
			ImGui::DragFloat("##gammavalue", &m_gamma, 0.1f);
			ImGui::PopItemWidth();
			ImGui::SameLine();
			ImGui::Spacing();
			ImGui::SameLine();
			ImGui::Text("Exposure:");
			ImGui::SameLine();
			ImGui::PushItemWidth(75.f);
			ImGui::DragFloat("##exposurevalue", &m_exposure, 0.1f);
			ImGui::PopItemWidth();
		}

		ImGui::Spacing();

		ImGui::Columns(1);

		ImGui::Spacing();
		ImGui::SameLine();
		ImGui::TextColored(titleColor, "Debug");
		ImGui::Separator();

		ImGui::Columns(2, "DebugVSettingsColumns", false);
		ImGui::SetColumnWidth(0, 5);
		ImGui::NextColumn();

		ImGui::Spacing();
		ImGui::Text("Debug Pass:");
		ImGui::SameLine();

		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted("Only Applies when Bloom is Enabled.");
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}

		ImGui::SameLine();
		ImGui::PushItemWidth(125.f);
		ImGui::Combo("##debugpass", (int*)&m_debugPass, m_drawPassStrs, (int)DrawPass::Count);
		ImGui::PopItemWidth();

		ImGui::EndChild();
		ImGui::EndGroup();
		ImGui::PopID();
		// ---------------------------------------------------

		ImGui::End();
	}

	ImGui::PopStyleVar();
}

void ViewerPanel::OnEvent(Elysium::Event& _event)
{
	Elysium::EventDispatcher dispatcher(_event);

	dispatcher.Dispatch<Elysium::MouseScrolledEvent>(BIND_EVENT_FN(ViewerPanel::OnMouseScroll));
}

bool ViewerPanel::OnMouseScroll(Elysium::MouseScrolledEvent& _event)
{
	const float delta = _event.GetYOffset();
	m_orthoSize -= delta * m_zoomModifier;

	if (m_orthoSize < 10.0f)
		m_orthoSize = 10.0f;

	UpdateCameraProjection();

	return false;
}

void ViewerPanel::UpdateCameraProjection()
{
	const float aspectRatio = m_outputSize.x / (float)m_outputSize.y;

	m_camera->SetProperties(aspectRatio, m_orthoSize);

	Elysium::CameraData& cameraRef = Elysium::CoreUniformBuffers::GetCameraDataRef();
	cameraRef.m_viewport = Elysium::Math::Vec4((float)m_size.x, (float)m_size.y, 0, 0);
	cameraRef.m_orthoProjectionMatrix = m_camera->GetProjection();
}

void ViewerPanel::UpdateCameraView()
{
	m_camera->SetPosition(Elysium::Math::Vec2(0, 0));

	Elysium::CameraData& cameraRef = Elysium::CoreUniformBuffers::GetCameraDataRef();
	cameraRef.m_orthoViewMatrix = m_camera->GetView();
}

void ViewerPanel::FocusCamera()
{
	m_orthoSize = static_cast<float>(std::max(m_size.x, m_size.y));
}

void ViewerPanel::SnapShot()
{
	// Retrieve the fbo data that represents the rotated normals
	const uint32_t img_width = m_shaderfbo->GetColorAttachment(0)->GetWidth();
	const uint32_t img_height = m_shaderfbo->GetColorAttachment(0)->GetHeight();

	cv::Mat currentImage(img_height, img_width, CV_8UC4, cv::Scalar(255, 255, 255, 0));

	const uint32_t datasize = img_width * img_height * 4;
	m_shaderfbo->Bind();
	uint8_t* pixelData = m_shaderfbo->ReadPixelBuffer(0, 0, 0, img_width, img_height);
	m_shaderfbo->Unbind();
	std::memcpy(currentImage.data, pixelData, datasize);
	delete[] pixelData;

	cv::Mat convertedImg;
	cv::cvtColor(currentImage, convertedImg, cv::COLOR_RGBA2BGRA);
	currentImage.release();

	const std::string outputFilepath = Elysium::FileDialogs::SaveFile("PNG Image (*.png)\0*.png\0"
																	  "JPEG Image (*.jpg, *.jpeg, *.jpe)\0*.jpg;*.jpeg;*.jpe\0");

	ELYSIUM_INFO("Writing Snapshot to {0}", outputFilepath);
	cv::imwrite(outputFilepath, convertedImg);
	convertedImg.release();
}

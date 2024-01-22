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
	m_playing(true)
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

	int samplers[Elysium::RendererCaps::MaxTextureSlots];
	for (int i = 0; i < Elysium::RendererCaps::MaxTextureSlots; ++i)
		samplers[i] = i;

	m_spriteShader->Bind();
	m_spriteShader->SetIntArray("textureMaps", samplers, Elysium::RendererCaps::MaxTextureSlots);
	m_spriteShader->Unbind();

	ELYSIUM_CORE_ASSERT(m_spriteShader->IsCompiled(), "Sprite Shader Failed to Compile.");
	ELYSIUM_CORE_ASSERT(m_blurShader->IsCompiled(), "Blur Shader Failed to Compile.");
	ELYSIUM_CORE_ASSERT(m_bloomShader->IsCompiled(), "Bloom Shader Failed to Compile.");

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
			m_hdrfbo->Bind();

			shader->Bind();

			Elysium::GraphicsCalls::ClearBuffers();
			Elysium::RenderCommands::DrawScreenShader(m_hdrfbo, shader);

			shader->Unbind();
			m_hdrfbo->Unbind();

			// Blur Pass - blur the bright color values
			bool horizontal = true, first_iteration = true;
			uint8_t amount = 10;
			for (uint8_t i = 0; i < amount; ++i)
			{
				m_blurShader->Bind();
				m_blurShader->SetInt("horizontal", horizontal);

				Elysium::GraphicsCalls::ClearBuffers();
				Elysium::RenderCommands::DrawTexture(m_bloomFbos[(int)horizontal], Elysium::RenderCommands::TextureDrawType::Color,
													 first_iteration ? m_hdrfbo->GetColorAttachment(1) : m_bloomFbos[(int)!horizontal]->GetColorAttachment(), 
													 m_blurShader);
				horizontal = !horizontal;
				if (first_iteration)
					first_iteration = false;
			}

			// Combination Pass - tonemap hdr color and blend to shader output.
			Elysium::GraphicsCalls::ClearBuffers();
			Elysium::RenderCommands::DrawTextures(m_shaderfbo, m_bloomShader,
												  { m_hdrfbo->GetColorAttachment(), m_bloomFbos[(int)!horizontal]->GetColorAttachment()});
		}
		else
		{
			m_shaderfbo->Bind();

			shader->Bind();

			Elysium::GraphicsCalls::ClearBuffers();
			Elysium::RenderCommands::DrawScreenShader(m_shaderfbo, shader);

			shader->Unbind();
			m_shaderfbo->Unbind();
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

	ImGui::SetColumnWidth(0, 400);
	ImGui::SetColumnWidth(1, std::max(10.0f, panelWidth - 400.f - 75.f));

	// Snapshot
	if (ImGui::Button(ICON_FA_CAMERA, ImVec2(50, 25)))
	{
		SnapShot();
	}
	ImGui::SameLine();

	// Re-focus
	if (ImGui::Button(ICON_FA_COMPRESS_ARROWS_ALT, ImVec2(35, 25)))
	{
		FocusCamera();
		UpdateCameraProjection();
	}
	ImGui::SameLine();

	ImGui::Text("W:");
	ImGui::SameLine();
	ImGui::PushItemWidth(50.f);
	ImGui::InputScalar("##desired_width", ImGuiDataType_U32, &m_desiredSize.x);
	ImGui::PopItemWidth();
	ImGui::SameLine();
	ImGui::Text("H:");
	ImGui::SameLine();
	ImGui::PushItemWidth(50.f);
	ImGui::InputScalar("##desired_height", ImGuiDataType_U32, &m_desiredSize.y);
	ImGui::PopItemWidth();
	
	ImGui::SameLine();
	ImGui::Spacing();
	ImGui::SameLine();
	ImGui::Text("Bloom:");
	ImGui::SameLine();
	ImGui::Checkbox("##bloom", &m_useBloom);

	ImGui::NextColumn();

	ImGui::NextColumn();

	if (ImGui::Button(m_playing ? ICON_FA_PAUSE : ICON_FA_PLAY, ImVec2(50, 25)))
	{
		m_playing = !m_playing;
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

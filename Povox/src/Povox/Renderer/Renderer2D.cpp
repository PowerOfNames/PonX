#include "pxpch.h"
#include "Povox/Renderer/Renderer2D.h"

#include "Povox/Core/Application.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Povox {
	
	
	Renderer2D::Renderer2D(const Renderer2DSpecification& specs)
		: m_Specification(specs)
	{
		Renderer::GetTextureSystem()->RegisterTexture("green");
		m_GreenTexture = std::dynamic_pointer_cast<Texture2D>(Renderer::GetTextureSystem()->GetTexture("green"));

		// TODO: Instead of taking a name and a path, just take in a name and pass the path upon ShaderLib creation inside the RendererBackend, pointing to root/.../assets/shaders/
		Renderer::GetShaderLibrary()->Add("TextureShader", Shader::Create("assets/shaders/Texture.glsl"));
		Renderer::GetShaderLibrary()->Add("FlatColorShader", Shader::Create("assets/shaders/FlatColor.glsl"));
		Renderer::GetShaderLibrary()->Add("Renderer2D_Quad", Shader::Create("assets/shaders/Renderer2D_Quad.glsl"));

		//Quads
		{
			FramebufferSpecification quadFBSpecs{};
			quadFBSpecs.DebugName = "RenderFramebuffer";
			quadFBSpecs.Attachments = { {ImageFormat::RGBA8}, {ImageFormat::Depth} };
			quadFBSpecs.Width = m_Specification.ViewportWidth;
			quadFBSpecs.Height = m_Specification.ViewportHeight;
			quadFBSpecs.SwapChainTarget = false;
			m_QuadFramebuffer = Framebuffer::Create(quadFBSpecs);


			RenderPassSpecification quadRPSpecs{};
			quadRPSpecs.DebugName = "RenderRenderpass";
			quadRPSpecs.TargetFramebuffer = m_QuadFramebuffer;
			quadRPSpecs.ColorAttachmentCount = 1;
			quadRPSpecs.HasDepthAttachment = true;
			m_QuadRenderpass = RenderPass::Create(quadRPSpecs);


			PipelineSpecification quadPLSpecs{};
			quadPLSpecs.DebugName = "TexturePipeline";
			quadPLSpecs.DynamicViewAndScissors = true;
			quadPLSpecs.Culling = PipelineUtils::CullMode::BACK;
			quadPLSpecs.TargetRenderPass = m_QuadRenderpass;
			quadPLSpecs.Shader = Renderer::GetShaderLibrary()->Get("Renderer2D_Quad");
			m_QuadPipeline = Pipeline::Create(quadPLSpecs);
		}
	}

	bool Renderer2D::Init()
	{
		PX_PROFILE_FUNCTION();


		PX_CORE_TRACE("Renderer2D::Init: Starting...");		
		
		BufferSpecification vertexBufferSpecs{};
		vertexBufferSpecs.Usage = BufferUsage::VERTEX_BUFFER;
		vertexBufferSpecs.MemUsage = MemoryUtils::MemoryUsage::GPU_ONLY;
		vertexBufferSpecs.ElementCount = m_Specification.MaxVertices;
		vertexBufferSpecs.ElementSize = sizeof(QuadVertex);
		vertexBufferSpecs.Size = sizeof(QuadVertex) * m_Specification.MaxVertices;

		m_QuadVertexPositions[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
		m_QuadVertexPositions[1] = { 0.5f, -0.5f, 0.0f, 1.0f };
		m_QuadVertexPositions[2] = { 0.5f, 0.5f, 0.0f, 1.0f };
		m_QuadVertexPositions[3] = { -0.5f, 0.5f, 0.0f, 1.0f };

		uint32_t* quadIndices = new uint32_t[m_Specification.MaxIndices];
		uint32_t offset = 0;
		for (uint32_t index = 0; index < m_Specification.MaxIndices; index += 6)
		{
			quadIndices[index + 0] = offset + 0;
			quadIndices[index + 1] = offset + 1;
			quadIndices[index + 2] = offset + 2;

			quadIndices[index + 3] = offset + 2;
			quadIndices[index + 4] = offset + 3;
			quadIndices[index + 5] = offset + 0;

			offset += 4;
		}
		BufferSpecification indexBufferSpecs{};
		indexBufferSpecs.Usage = BufferUsage::INDEX_BUFFER;
		indexBufferSpecs.MemUsage = MemoryUtils::MemoryUsage::GPU_ONLY;
		indexBufferSpecs.ElementCount = m_Specification.MaxIndices;
		indexBufferSpecs.ElementSize = sizeof(uint32_t);
		indexBufferSpecs.Size = sizeof(uint32_t) * m_Specification.MaxIndices;
		indexBufferSpecs.Data = quadIndices;

		uint32_t maxFrames = Renderer::GetSpecification().MaxFramesInFlight;
		m_QuadVertexBuffers.resize(maxFrames);
		m_QuadVertexBufferBases.resize(maxFrames);

		m_QuadIndexBuffers.resize(maxFrames);
		for (uint32_t i = 0; i < maxFrames; i++)
		{
			vertexBufferSpecs.DebugName = "Renderer2D Vertexbuffer Frame: " + std::to_string(i);

			m_QuadVertexBuffers[i] = Buffer::Create(vertexBufferSpecs);
			m_QuadVertexBufferBases[i] = new QuadVertex[m_Specification.MaxVertices];

			indexBufferSpecs.DebugName = "Renderer2D Indices Frame: " + std::to_string(i);
			m_QuadIndexBuffers[i] = Buffer::Create(indexBufferSpecs);
		}
		delete[] quadIndices;
		m_QuadMaterial = Material::Create(Renderer::GetShaderLibrary()->Get("Renderer2D_Quad"), "Quad");

		/* -> move to Pipeline.Layout to check against Pipeline.Shader.Layout -> For VertexInput checkup
		m_QuadVertexBuffer->SetLayout({
			{ ShaderUtils::ShaderDataType::Float3, "a_Position" },
			{ ShaderUtils::ShaderDataType::Float4, "a_Color" },
			{ ShaderUtils::ShaderDataType::Float2, "a_TexCoord" },
			{ ShaderUtils::ShaderDataType::Int, "a_EntityID" }
			});
		*/

		m_WhiteTexture = Texture2D::Create(1, 1, 4, "WhiteTexture");
		uint32_t whiteTextureData = 0xffffffff;
		m_WhiteTexture->SetData(&whiteTextureData);
		Renderer::GetTextureSystem()->RegisterTexture("WhiteTexture", m_WhiteTexture);

		m_WhiteTextureSlot = Renderer::GetTextureSystem()->BindFixedTexture(m_WhiteTexture);
		

		PX_CORE_TRACE("Renderer2D::Init: Completed.");
		return true;
	}

	void Renderer2D::Shutdown()
	{
		PX_PROFILE_FUNCTION();

		//TODO: Investigate shutdown read access error here!
		uint32_t maxFrames = Renderer::GetSpecification().MaxFramesInFlight;
		for (uint32_t i = 0; i < maxFrames; i++)
		{
			delete[] m_QuadVertexBufferBases[i];
		}
	}

	void Renderer2D::OnResize(uint32_t width, uint32_t height)
	{
		PX_PROFILE_FUNCTION();

		if((width <= 0 || height <= 0 ))
			return;

		if (width == m_Specification.ViewportWidth
			&& height == m_Specification.ViewportHeight)
			return;

		m_Specification.ViewportWidth = width;
		m_Specification.ViewportHeight = height;

		// Quads
		m_QuadFramebuffer->Recreate(width, height);
		m_QuadRenderpass->Recreate();
		m_QuadPipeline->Recreate();
	}

	void Renderer2D::BeginScene(const OrthographicCamera& camera)
	{
		PX_PROFILE_FUNCTION();

		m_CameraData.ViewProjMatrix = camera.GetViewProjectionMatrix();
		StartBatch();
	}

	void Renderer2D::BeginScene(const Camera& camera, const glm::mat4& transform)
	{
		PX_PROFILE_FUNCTION();

		m_CameraData.ViewProjMatrix = camera.GetProjection() * glm::inverse(transform);
		StartBatch();
	}

	void Renderer2D::BeginScene(const EditorCamera& camera)
	{
		PX_PROFILE_FUNCTION();


		uint32_t currentFrameIndex = Renderer::GetCurrentFrameIndex();
		auto cmd = Renderer::GetCommandBuffer(currentFrameIndex);
		Renderer::BeginCommandBuffer(cmd);
		Renderer::StartTimestampQuery("QuadRenderpass");
		Renderer::BeginRenderPass(m_QuadRenderpass);

		Renderer::BindPipeline(m_QuadPipeline);		

		m_CameraData.ViewMatrix = camera.GetViewMatrix();
		m_CameraData.ProjectionMatrix = camera.GetProjection();
		m_CameraData.ViewProjMatrix = camera.GetViewProjectionMatrix();
		Renderer::UpdateCamera(m_CameraData);

		StartBatch();

		Renderer2D::DrawQuad({ -2.0f, -3.0f,-4.0f }, { 1.0f, 1.0f }, m_GreenTexture, 0.5f);
		Renderer2D::DrawQuad({ 0.0f, 0.0f, -2.0f }, { 2.0f, 3.0f }, m_GreenTexture, 2.0f);
		{
			PX_PROFILE_SCOPE("DrawQuadLoop");
			int quads = 10;
			float dquads = quads * 2.0f;
			for (int i = -quads; i <= quads; i++)
			{
				for (int j = -quads; j <= quads; j++)
				{
					Renderer2D::DrawQuad({ i, j, 0.0f }, { 0.9f, 0.9f }, { (i + quads) / dquads, (j + quads) / dquads, 0.1f, 1.0f });
				}
			}
		}
	}

	void Renderer2D::StartBatch()
	{
		PX_PROFILE_FUNCTION();


		uint32_t currentFrame = Renderer::GetCurrentFrameIndex();
		m_QuadIndexCount = 0;
		m_QuadVertexBufferPtr = m_QuadVertexBufferBases[currentFrame];

		Renderer::GetTextureSystem()->ResetActiveTextures();
		m_RenderedObjects.clear();
	}

	void Renderer2D::Flush()
	{
		PX_PROFILE_FUNCTION();


		uint32_t currentFrame = Renderer::GetCurrentFrameIndex();
		if (m_QuadIndexCount == 0)
			return; // nothing to draw

		uint32_t dataSize = (uint32_t)((uint8_t*)m_QuadVertexBufferPtr - (uint8_t*)m_QuadVertexBufferBases[currentFrame]);
		m_QuadVertexBuffers[currentFrame]->SetData(m_QuadVertexBufferBases[currentFrame], dataSize);		
		Renderer::Draw(m_QuadVertexBuffers[currentFrame], m_QuadMaterial, m_QuadIndexBuffers[currentFrame], m_QuadIndexCount);

		
		m_Stats.DrawCalls++;
	}

	void Renderer2D::EndScene()
	{
		PX_PROFILE_FUNCTION();

		Flush();


		Renderer::EndRenderPass();
		Renderer::StopTimestampQuery("QuadRenderpass");
		Renderer::EndCommandBuffer();


		//CopyFinalImage into current SwapchainImage
		if (!Application::Get()->GetSpecification().ImGuiEnabled)
			Renderer::PrepareSwapchainImage(m_QuadFramebuffer->GetColorAttachment(0));
		else
			Renderer::CreateFinalImage(m_QuadFramebuffer->GetColorAttachment(0));
	}

	void Renderer2D::NextBatch()
	{
		Flush();
		StartBatch();
	}


// Quads ColorOnly
	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2 size, const glm::vec4& color)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, color);
	}
	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2 size, const glm::vec4& color)
	{
		constexpr glm::vec2 textureCoords[4] = { {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f} };

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });		

		DrawQuad(transform, color);
	}
	void Renderer2D::DrawQuad(const glm::mat4& transform, const glm::vec4& color, UUID entityID)
	{
		//PX_PROFILE_FUNCTION();


		constexpr glm::vec2 textureCoords[4] = { {1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 1.0f} };

		uint32_t currentFrame = Renderer::GetCurrentFrameIndex();
		if (m_QuadIndexCount >= m_Specification.MaxIndices)
			NextBatch();

		for (uint32_t i = 0; i < 4; i++)
		{
			m_QuadVertexBufferPtr->Position = transform * m_QuadVertexPositions[i];
			m_QuadVertexBufferPtr->Color = color;
			m_QuadVertexBufferPtr->TexCoord = textureCoords[i];
			m_QuadVertexBufferPtr->TexID = m_WhiteTextureSlot;
			m_QuadVertexBufferPtr++;
		}
		m_QuadIndexCount += 6;
		m_Stats.QuadCount++;
	}

// Quads Texture
	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2 size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintingColor)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, texture, tilingFactor, tintingColor);
	}
	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2 size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintingColor)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, texture, tilingFactor, tintingColor);
	}
	void Renderer2D::DrawQuad(const glm::mat4& transform, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintingColor, UUID entityID)
	{
		PX_PROFILE_FUNCTION();


		constexpr glm::vec2 textureCoords[4] = { {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f} };
		
		uint32_t currentFrame = Renderer::GetCurrentFrameIndex();
		if (m_QuadIndexCount >= m_Specification.MaxIndices)
			NextBatch();

		//Will return the first slot possible if allSlotsFull is true
		uint32_t textureIndex = Renderer::GetTextureSystem()->BindTexture(texture);
		if (textureIndex >= m_Specification.MaxTextureSlots)
			NextBatch();
		textureIndex = Renderer::GetTextureSystem()->BindTexture(texture);

		for (uint32_t i = 0; i < 4; i++)
		{
			m_QuadVertexBufferPtr->Position = transform * m_QuadVertexPositions[i];
			m_QuadVertexBufferPtr->Color = tintingColor;
			m_QuadVertexBufferPtr->TexCoord = textureCoords[i];
			m_QuadVertexBufferPtr->TexID = textureIndex;
			m_QuadVertexBufferPtr++;
		}
		m_QuadIndexCount += 6;
		m_Stats.QuadCount++;
	}

// Quads Subtexture
	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2 size, const Ref<SubTexture2D>& subTexture, float tilingFactor, const glm::vec4& tintingColor)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, subTexture, tilingFactor, tintingColor);
	}
	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2 size, const Ref<SubTexture2D>& subTexture, float tilingFactor, const glm::vec4& tintingColor)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, subTexture, tilingFactor, tintingColor);
	}
	void Renderer2D::DrawQuad(const glm::mat4& transform, const Ref<SubTexture2D>& subTexture, float tilingFactor, const glm::vec4& tintingColor, UUID entityID)
	{
		const glm::vec2* textureCoords = subTexture->GetTexCoords();
		const Ref<Texture2D> texture = subTexture->GetTexture();

		uint32_t currentFrame = Renderer::GetCurrentFrameIndex();

		if (m_QuadIndexCount >= m_Specification.MaxIndices)
			NextBatch();

		//float textureIndex = 0.0f;
		//for (uint32_t i = 1; i < m_TextureSlotIndex; i++)
		//{
		//	if (*m_TextureSlots[i].get() == *texture.get()) // m_TextureSlots[i] == texture -> compares the shared ptr, so .get() gives the ptr and * dereferences is to the fnc uses the boolean == operator defined in the openGLTexture2D class
		//	{
		//		textureIndex = (float)i;
		//		break;
		//	}

		//}

		//if (textureIndex == 0.0f)
		//{
		//	textureIndex = (float)m_TextureSlotIndex;
		//	m_TextureSlots[m_TextureSlotIndex] = texture;
		//	m_TextureSlotIndex++;
		//}

		//for (uint32_t i = 0; i < 4; i++)
		//{
		//	m_QuadVertexBufferPtr->Position = transform * m_QuadVertexPositions[i];
		//	m_QuadVertexBufferPtr->Color = tintingColor;
		//	m_QuadVertexBufferPtr->TexCoord = textureCoords[i];
		//	m_QuadVertexBufferPtr->TexID = textureIndex;
		//	m_QuadVertexBufferPtr->TilingFactor = tilingFactor;
		//	m_QuadVertexBufferPtr->EntityID = entityID;
		//	m_QuadVertexBufferPtr++;
		//}
		//m_QuadIndexCount += 6;

		//m_Stats.QuadCount++;
	}

//Rotatables
	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2 size, float rotation, const glm::vec4& color)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, color);
	}
	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2 size, float rotation, const glm::vec4& color)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, color);
	}
	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2 size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintingColor)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, texture, tilingFactor, tintingColor);
	}
	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2 size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintingColor)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });
		
		DrawQuad(transform, texture, tilingFactor, tintingColor);
	}
	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2 size, float rotation, const Ref<SubTexture2D>& subTexture, float tilingFactor, const glm::vec4& tintingColor)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, subTexture, tilingFactor, tintingColor);
	}
	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2 size, float rotation, const Ref<SubTexture2D>& subTexture, float tilingFactor, const glm::vec4& tintingColor)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, subTexture, tilingFactor, tintingColor);
	}


//Sprite
	void Renderer2D::DrawSprite(const glm::mat4& transform, SpriteRendererComponent& src, UUID entityID)
	{
		DrawQuad(transform, src.Color, entityID);
	}


	const Renderer2DStatistics& Renderer2D::GetStatistics() const 
	{
		return m_Stats;
	}
	void Renderer2D::ResetStats()
	{
		memset(&m_Stats, 0, sizeof(Renderer2DStatistics));
	}
}

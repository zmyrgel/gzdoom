/*
**  Hardpoly renderer
**  Copyright (c) 2016 Magnus Norddahl
**
**  This software is provided 'as-is', without any express or implied
**  warranty.  In no event will the authors be held liable for any damages
**  arising from the use of this software.
**
**  Permission is granted to anyone to use this software for any purpose,
**  including commercial applications, and to alter it and redistribute it
**  freely, subject to the following restrictions:
**
**  1. The origin of this software must not be misrepresented; you must not
**     claim that you wrote the original software. If you use this software
**     in a product, an acknowledgment in the product documentation would be
**     appreciated but is not required.
**  2. Altered source versions must be plainly marked as such, and must not be
**     misrepresented as being the original software.
**  3. This notice may not be removed or altered from any source distribution.
**
*/

#pragma once

#include "polyrenderer/hardpoly/gpu_context.h"

struct subsector_t;
struct particle_t;
class PolyDrawArgs;
class PolyRenderThread;
class HardpolyRenderer;
class DCanvas;

struct FrameUniforms
{
	Mat4f WorldToView;
	Mat4f ViewToProjection;
	float GlobVis;
	float Padding1, Padding2, Padding3;
};

struct FaceUniforms
{
	float Light;
	float AlphaTest;
	int Mode;
	int Padding3;
	Vec4f FillColor;
	Vec4f ClipPlane0;
	Vec4f ClipPlane1;
	Vec4f ClipPlane2;
};

struct RectUniforms
{
	float x0, y0, u0, v0;
	float x1, y1, u1, v1;
	float Light;
	float Padding1, Padding2, Padding3;
};

struct DrawRun
{
	FTexture *Texture = nullptr;
	const uint8_t *Pixels = nullptr;
	int PixelsWidth = 0;
	int PixelsHeight = 0;
	const uint8_t *BaseColormap = nullptr;
	const uint8_t *Translation = nullptr;
	int Start = 0;
	int NumVertices = 0;
	PolyDrawMode DrawMode = PolyDrawMode::Triangles;
	FaceUniforms Uniforms;
	TriBlendMode BlendMode;
	uint32_t SrcAlpha = 0;
	uint32_t DestAlpha = 0;
	bool DepthTest = true;
	bool WriteDepth = true;
};

struct DrawBatch
{
	std::shared_ptr<GPUVertexArray> VertexArray;
	std::shared_ptr<GPUVertexBuffer> Vertices;
	std::shared_ptr<GPUUniformBuffer> FaceUniforms;
	std::vector<DrawRun> DrawRuns;

	std::vector<TriVertex> CpuVertices;
};

class DrawBatcher
{
public:
	void GetVertices(int numVertices);
	void Flush();

	void DrawBatches(HardpolyRenderer *hardpoly);
	void NextFrame();

	TriVertex *mVertices = nullptr;
	int mNextVertex = 0;
	DrawBatch *mCurrentBatch = nullptr;

private:
	enum { MaxVertices = 16 * 1024 };

	std::vector<std::unique_ptr<DrawBatch>> mCurrentFrameBatches;
	std::vector<std::unique_ptr<DrawBatch>> mLastFrameBatches;
	size_t mDrawStart = 0;
	size_t mNextBatch = 0;
};

class HardpolyRenderer
{
public:
	HardpolyRenderer();
	~HardpolyRenderer();

	void Begin();
	void ClearBuffers(DCanvas *canvas);
	void SetViewport(int x, int y, int width, int height, DCanvas *canvas);
	void DrawArray(PolyRenderThread *thread, const PolyDrawArgs &args);
	void DrawRect(const RectDrawArgs &args);
	void End();

	void RenderBatch(DrawBatch *batch);

	Mat4f worldToView;
	Mat4f viewToClip;

private:
	void SetupFramebuffer();
	void CompileShaders();
	void CreateSamplers();
	void UpdateFrameUniforms();

	std::shared_ptr<GPUTexture2D> GetTexturePal(FTexture *texture);
	std::shared_ptr<GPUTexture2D> GetTextureBgra(FTexture *texture);
	std::shared_ptr<GPUTexture2D> GetColormapTexture(const uint8_t *basecolormap);
	std::shared_ptr<GPUTexture2D> GetTranslationTexture(const uint8_t *translation);
	std::shared_ptr<GPUTexture2D> GetEngineTexturePal(const uint8_t *pixels, int width, int height);

	std::shared_ptr<GPUContext> mContext;

	std::shared_ptr<GPUTexture2D> mAlbedoBuffer;
	std::shared_ptr<GPUTexture2D> mDepthStencilBuffer;
	std::shared_ptr<GPUTexture2D> mNormalBuffer;
	std::shared_ptr<GPUTexture2D> mSpriteDepthBuffer;

	std::shared_ptr<GPUFrameBuffer> mSceneFB;
	std::shared_ptr<GPUFrameBuffer> mTranslucentFB;

	std::shared_ptr<GPUUniformBuffer> mFrameUniforms[3];
	int mCurrentFrameUniforms = 0;
	bool mFrameUniformsDirty = true;

	std::shared_ptr<GPUUniformBuffer> mRectUniforms;

	std::shared_ptr<GPUVertexArray> mVertexArray;
	std::map<FTexture*, std::shared_ptr<GPUTexture2D>> mTextures;
	std::map<const uint8_t *, std::shared_ptr<GPUTexture2D>> mColormaps;
	std::map<const uint8_t *, std::shared_ptr<GPUTexture2D>> mTranslationTextures;
	std::map<const uint8_t *, std::shared_ptr<GPUTexture2D>> mEngineTextures;

	std::shared_ptr<GPUVertexArray> mScreenQuad;
	std::shared_ptr<GPUVertexBuffer> mScreenQuadVertexBuffer;

	std::shared_ptr<GPUProgram> mOpaqueProgram;
	std::shared_ptr<GPUProgram> mStencilProgram;
	std::shared_ptr<GPUProgram> mRectProgram;

	std::shared_ptr<GPUSampler> mSamplerLinear;
	std::shared_ptr<GPUSampler> mSamplerNearest;

	typedef void(HardpolyRenderer::*BlendSetterFunc)(int srcalpha, int destalpha);
	static BlendSetterFunc GetBlendSetter(TriBlendMode triblend);

	static int GetSamplerMode(TriBlendMode triblend);

	void SetOpaqueBlend(int srcalpha, int destalpha);
	void SetMaskedBlend(int srcalpha, int destalpha);
	void SetAddClampBlend(int srcalpha, int destalpha);
	void SetSubClampBlend(int srcalpha, int destalpha);
	void SetRevSubClampBlend(int srcalpha, int destalpha);
	void SetAddSrcColorBlend(int srcalpha, int destalpha);
	void SetShadedBlend(int srcalpha, int destalpha);
	void SetAddClampShadedBlend(int srcalpha, int destalpha);
	void SetAlphaBlendFunc(int srcalpha, int destalpha);
};

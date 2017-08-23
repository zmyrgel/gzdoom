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
class DCanvas;

struct FrameUniforms
{
	Mat4f WorldToView;
	Mat4f ViewToProjection;
};

struct FaceUniforms
{
	float LightLevel;
	float AlphaTest;
	float Padding2, Padding3;
};

struct DrawRun
{
	FTexture *Texture = nullptr;
	const uint8_t *BaseColormap = nullptr;
	int Start = 0;
	int NumVertices = 0;
	PolyDrawMode DrawMode = PolyDrawMode::Triangles;
	FaceUniforms Uniforms;
};

struct DrawBatch
{
	std::shared_ptr<GPUVertexArray> VertexArray;
	std::shared_ptr<GPUVertexBuffer> Vertices;
	std::vector<DrawRun> DrawRuns;
};

class DrawBatcher
{
public:
	void GetVertices(HardpolyRenderer *hardpoly, int numVertices);
	void Flush(HardpolyRenderer *hardpoly);
	void NextFrame();

	TriVertex *mVertices = nullptr;
	int mNextVertex = 0;
	DrawBatch *mCurrentBatch = nullptr;

private:
	enum { MaxVertices = 16 * 1024 };

	std::vector<std::unique_ptr<DrawBatch>> mCurrentFrameBatches;
	std::vector<std::unique_ptr<DrawBatch>> mLastFrameBatches;
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
	void DrawArray(const PolyDrawArgs &args);
	void End();

	void RenderBatch(DrawBatch *batch);

	Mat4f worldToView;
	Mat4f viewToClip;

private:
	void SetupFramebuffer();
	void CompileShaders();
	void CreateSamplers();

	DrawBatcher mDrawBatcher;

	std::shared_ptr<GPUTexture2D> GetTexturePal(FTexture *texture);
	std::shared_ptr<GPUTexture2D> GetTextureBgra(FTexture *texture);
	std::shared_ptr<GPUTexture2D> GetColormapTexture(const uint8_t *basecolormap);

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

	std::shared_ptr<GPUUniformBuffer> mFaceUniforms;

	std::shared_ptr<GPUVertexArray> mVertexArray;
	std::map<FTexture*, std::shared_ptr<GPUTexture2D>> mTextures;
	std::map<const uint8_t *, std::shared_ptr<GPUTexture2D>> mColormaps;

	std::shared_ptr<GPUProgram> mOpaqueProgram;
	std::shared_ptr<GPUProgram> mStencilProgram;

	std::shared_ptr<GPUSampler> mSamplerLinear;
	std::shared_ptr<GPUSampler> mSamplerNearest;

};

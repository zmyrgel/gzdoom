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

#include <memory>
#include <vector>
#include <map>
#include "polyrenderer/math/gpu_types.h"

enum class GPUPixelFormat
{
	RGBA8,
	sRGB8_Alpha8,
	RGBA16,
	RGBA16f,
	RGBA32f,
	Depth24_Stencil8,
	R32f,
	R8
};

class GPUTexture
{
public:
	virtual ~GPUTexture() { }
	virtual int Handle() const = 0;
};

class GPUTexture2D : public GPUTexture
{
public:
	GPUTexture2D(int width, int height, bool mipmap, int sampleCount, GPUPixelFormat format, const void *pixels = nullptr);
	~GPUTexture2D();

	void Upload(int x, int y, int width, int height, int level, const void *pixels);

	int Handle() const override { return mHandle; }
	int SampleCount() const { return mSampleCount; }
	int Width() const { return mWidth; }
	int Height() const { return mHeight; }

private:
	GPUTexture2D(const GPUTexture2D &) = delete;
	GPUTexture2D &operator =(const GPUTexture2D &) = delete;

	static int NumLevels(int width, int height);
	static int ToInternalFormat(GPUPixelFormat format);
	static int ToUploadFormat(GPUPixelFormat format);
	static int ToUploadType(GPUPixelFormat format);

	int mHandle = 0;
	int mWidth = 0;
	int mHeight = 0;
	bool mMipmap = false;
	int mSampleCount = 0;
	GPUPixelFormat mFormat;
};

/////////////////////////////////////////////////////////////////////////////

class GPUFrameBuffer
{
public:
	GPUFrameBuffer(const std::vector<std::shared_ptr<GPUTexture2D>> &color, const std::shared_ptr<GPUTexture2D> &depthstencil);
	~GPUFrameBuffer();

	int Handle() const { return mHandle; }

private:
	GPUFrameBuffer(const GPUFrameBuffer &) = delete;
	GPUFrameBuffer &operator =(const GPUFrameBuffer &) = delete;

	int mHandle = 0;
};

/////////////////////////////////////////////////////////////////////////////

class GPUIndexBuffer
{
public:
	GPUIndexBuffer(const void *data, int size);
	~GPUIndexBuffer();

	int Handle() const { return mHandle; }

	void Upload(const void *data, int size);

	void *MapWriteOnly();
	void Unmap();

private:
	GPUIndexBuffer(const GPUIndexBuffer &) = delete;
	GPUIndexBuffer &operator =(const GPUIndexBuffer &) = delete;

	int mHandle = 0;
};

/////////////////////////////////////////////////////////////////////////////

enum class GPUShaderType
{
	Vertex,
	Fragment
};

class GPUProgram
{
public:
	GPUProgram();
	~GPUProgram();

	int Handle() const { return mHandle; }

	void SetDefine(const std::string &name);
	void SetDefine(const std::string &name, int value);
	void SetDefine(const std::string &name, float value);
	void SetDefine(const std::string &name, double value);
	void SetDefine(const std::string &name, const std::string &value);
	void Compile(GPUShaderType type, const char *lumpName);
	void Compile(GPUShaderType type, const char *name, const std::string &code);
	void SetAttribLocation(const std::string &name, int index);
	void SetFragOutput(const std::string &name, int index);
	void Link(const std::string &name);
	void SetUniformBlock(const std::string &name, int index);

private:
	GPUProgram(const GPUProgram &) = delete;
	GPUProgram &operator =(const GPUProgram &) = delete;

	std::string PrefixCode() const;
	std::string GetShaderInfoLog(int handle) const;
	std::string GetProgramInfoLog() const;

	int mHandle = 0;
	std::map<int, int> mShaderHandle;
	std::map<std::string, std::string> mDefines;
};

/////////////////////////////////////////////////////////////////////////////

enum class GPUSampleMode
{
	Nearest,
	Linear
};

enum class GPUMipmapMode
{
	None,
	Nearest,
	Linear
};

enum class GPUWrapMode
{
	Repeat,
	Mirror,
	ClampToEdge
};

class GPUSampler
{
public:
	GPUSampler(GPUSampleMode minfilter, GPUSampleMode magfilter, GPUMipmapMode mipmap, GPUWrapMode wrapU, GPUWrapMode wrapV);
	~GPUSampler();

	int Handle() const { return mHandle; }

private:
	GPUSampler(const GPUSampler &) = delete;
	GPUSampler &operator =(const GPUSampler &) = delete;

	int mHandle = 0;
	GPUSampleMode mMinfilter = GPUSampleMode::Nearest;
	GPUSampleMode mMagfilter = GPUSampleMode::Nearest;
	GPUMipmapMode mMipmap = GPUMipmapMode::None;
	GPUWrapMode mWrapU = GPUWrapMode::Repeat;
	GPUWrapMode mWrapV = GPUWrapMode::Repeat;
};

/////////////////////////////////////////////////////////////////////////////

class GPUStorageBuffer
{
public:
	GPUStorageBuffer(const void *data, int size);
	~GPUStorageBuffer();

	void Upload(const void *data, int size);

	int Handle() const { return mHandle; }

private:
	GPUStorageBuffer(const GPUStorageBuffer &) = delete;
	GPUStorageBuffer &operator =(const GPUStorageBuffer &) = delete;

	int mHandle = 0;
};

/////////////////////////////////////////////////////////////////////////////

class GPUUniformBuffer
{
public:
	GPUUniformBuffer(const void *data, int size);
	~GPUUniformBuffer();

	void Upload(const void *data, int size);

	int Handle() const { return mHandle; }

private:
	GPUUniformBuffer(const GPUUniformBuffer &) = delete;
	GPUUniformBuffer &operator =(const GPUUniformBuffer &) = delete;

	int mHandle = 0;
};

/////////////////////////////////////////////////////////////////////////////

class GPUVertexBuffer;

enum class GPUVertexAttributeType
{
	Int8,
	Uint8,
	Int16,
	Uint16,
	Int32,
	Uint32,
	HalfFloat,
	Float
};

class GPUVertexAttributeDesc
{
public:
	GPUVertexAttributeDesc(int index, int size, GPUVertexAttributeType type, bool normalized, int stride, std::size_t offset, std::shared_ptr<GPUVertexBuffer> buffer)
		: Index(index), Size(size), Type(type), Normalized(normalized), Stride(stride), Offset(offset), Buffer(buffer) { }

	int Index;
	int Size;
	GPUVertexAttributeType Type;
	bool Normalized;
	int Stride;
	std::size_t Offset;
	std::shared_ptr<GPUVertexBuffer> Buffer;
};

class GPUVertexArray
{
public:
	GPUVertexArray(const std::vector<GPUVertexAttributeDesc> &attributes);
	~GPUVertexArray();

	int Handle() const { return mHandle; }

private:
	GPUVertexArray(const GPUVertexArray &) = delete;
	GPUVertexArray &operator =(const GPUVertexArray &) = delete;

	static int FromType(GPUVertexAttributeType type);

	int mHandle = 0;
	std::vector<GPUVertexAttributeDesc> mAttributes;
};

/////////////////////////////////////////////////////////////////////////////

class GPUVertexBuffer
{
public:
	GPUVertexBuffer(const void *data, int size);
	~GPUVertexBuffer();

	void Upload(const void *data, int size);

	void *MapWriteOnly();
	void Unmap();

	int Handle() const { return mHandle; }

private:
	GPUVertexBuffer(const GPUVertexBuffer &) = delete;
	GPUVertexBuffer &operator =(const GPUVertexBuffer &) = delete;

	int mHandle = 0;
};

/////////////////////////////////////////////////////////////////////////////

enum class GPUIndexFormat
{
	Uint16,
	Uint32
};

enum class GPUDrawMode
{
	Points,
	LineStrip,
	LineLoop,
	Lines,
	TriangleStrip,
	TriangleFan,
	Triangles
};

class GPUContext
{
public:
	GPUContext();
	~GPUContext();
	
	void Begin();
	void End();
	
	void ClearError();
	void CheckError();

	void SetFrameBuffer(const std::shared_ptr<GPUFrameBuffer> &fb);
	void SetViewport(int x, int y, int width, int height);

	void SetProgram(const std::shared_ptr<GPUProgram> &program);

	void SetSampler(int index, const std::shared_ptr<GPUSampler> &sampler);
	void SetTexture(int index, const std::shared_ptr<GPUTexture> &texture);
	void SetUniforms(int index, const std::shared_ptr<GPUUniformBuffer> &buffer);
	void SetStorage(int index, const std::shared_ptr<GPUStorageBuffer> &storage);

	void SetVertexArray(const std::shared_ptr<GPUVertexArray> &vertexarray);
	void SetIndexBuffer(const std::shared_ptr<GPUIndexBuffer> &indexbuffer, GPUIndexFormat format = GPUIndexFormat::Uint16);

	void Draw(GPUDrawMode mode, int vertexStart, int vertexCount);
	void DrawIndexed(GPUDrawMode mode, int indexStart, int indexCount);
	void DrawInstanced(GPUDrawMode mode, int vertexStart, int vertexCount, int instanceCount);
	void DrawIndexedInstanced(GPUDrawMode mode, int indexStart, int indexCount, int instanceCount);

	void ClearColorBuffer(int index, float r, float g, float b, float a);
	void ClearDepthBuffer(float depth);
	void ClearStencilBuffer(int stencil);
	void ClearDepthStencilBuffer(float depth, int stencil);

private:
	GPUContext(const GPUContext &) = delete;
	GPUContext &operator =(const GPUContext &) = delete;

	static int FromDrawMode(GPUDrawMode mode);

	GPUIndexFormat mIndexFormat = GPUIndexFormat::Uint16;
	
	int oldDrawFramebufferBinding = 0;
	int oldReadFramebufferBinding = 0;
	int oldProgram = 0;
	int oldTextureBinding0 = 0;
	int oldTextureBinding1 = 0;
};

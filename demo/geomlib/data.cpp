/*
 * Copyright (c) 2014 Roman Kuznetsov 
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "stdafx.h"
#include "data.h"

namespace geom
{

const char* POSITION_SEMANTIC = "POSITION";
const char* NORMAL_SEMANTIC = "NORMAL";
const char* TEXCOORD_SEMANTIC = "TEXCOORD";
const char* TANGENT_SEMANTIC = "TANGENT";
const char* BINORMAL_SEMANTIC = "BINORMAL";

Data::Data() :
	m_additionalUVsCount(0),
	m_verticesCount(0)
{

}

Data::Data(const Data& data):
	m_additionalUVsCount(data.m_additionalUVsCount),
	m_verticesCount(data.m_verticesCount),
	m_meshes(data.m_meshes),
	m_boundingBox(data.m_boundingBox),
	m_vertexBuffer(data.m_vertexBuffer),
	m_indexBuffer(data.m_indexBuffer),
	m_lastError(data.m_lastError)
{
}

Data::Data(Data&& data)
{
	m_additionalUVsCount = std::move(data.m_additionalUVsCount);
	m_verticesCount = std::move(data.m_verticesCount);
	m_meshes = std::move(data.m_meshes);
	m_boundingBox = std::move(data.m_boundingBox);
	m_vertexBuffer = std::move(data.m_vertexBuffer);
	m_indexBuffer = std::move(data.m_indexBuffer);
	m_lastError = std::move(data.m_lastError);
}

size_t Data::getVertexComponentsCount() const
{
	return 5 + m_additionalUVsCount;
}

size_t Data::getVertexComponentSize(size_t index) const
{
	static Vertex v;
	if (index == 0) return sizeof(v.position);
	else if (index == 1) return sizeof(v.normal);
	else if (index == 2) return sizeof(v.texCoord0);
	else if (index == 3) return sizeof(v.tangent);
	else if (index == 4) return sizeof(v.binormal);
	else if (index >= 5 && index < getVertexComponentsCount()) return sizeof(v.texCoord0);
	return 0;
}

size_t Data::getVertexComponentOffset(size_t index) const
{
	if (index >= getVertexComponentsCount()) return 0;

	size_t offset = 0;
	for (size_t i = 0; i < index; i++) offset += getVertexComponentSize(i);
	return offset;
}

size_t Data::getVertexSize() const
{
	return sizeof(Vertex) + m_additionalUVsCount * sizeof(vector2);
}

const char* Data::getSemanticName(size_t index) const
{
	if (index == 0) return POSITION_SEMANTIC;
	else if (index == 1) return NORMAL_SEMANTIC;
	else if (index == 2) return TEXCOORD_SEMANTIC;
	else if (index == 3) return TANGENT_SEMANTIC;
	else if (index == 4) return BINORMAL_SEMANTIC;
	else if (index >= 5 && index < getVertexComponentsCount()) return TEXCOORD_SEMANTIC;
	return "";
}
size_t Data::getSemanticIndex(size_t index) const
{
	if (index < 5) return 0;
	return index - 4;
}

Data& Data::operator=(const Data& data)
{
	if (this == &data) return *this;

	m_additionalUVsCount = data.m_additionalUVsCount;
	m_verticesCount = data.m_verticesCount;
	m_meshes = data.m_meshes;
	m_boundingBox = data.m_boundingBox;
	m_vertexBuffer = data.m_vertexBuffer;
	m_indexBuffer = data.m_indexBuffer;
	return *this;
}

Data& Data::operator=(Data&& data)
{
	if (this == &data) return *this;

	m_additionalUVsCount = std::move(data.m_additionalUVsCount);
	m_verticesCount = std::move(data.m_verticesCount);
	m_meshes = std::move(data.m_meshes);
	m_boundingBox = std::move(data.m_boundingBox);
	m_vertexBuffer = std::move(data.m_vertexBuffer);
	m_indexBuffer = std::move(data.m_indexBuffer);
	return *this;
}

bool Data::isCorrect() const
{
	return m_lastError.empty();
}

const std::string& Data::getLastError() const
{
	return m_lastError;
}

const Data::Meshes& Data::getMeshes() const
{
	return m_meshes;
}

size_t Data::getAdditionalUVsCount() const
{
	return m_additionalUVsCount;
}

size_t Data::getVerticesCount() const
{
	return m_verticesCount;
}

const bbox3& Data::getBoundingBox() const
{
	return m_boundingBox;
}

const std::vector<unsigned char>& Data::getVertexBuffer() const
{
	return m_vertexBuffer;
}

const std::vector<unsigned int>& Data::getIndexBuffer() const
{
	return m_indexBuffer;
}

int GetAdjacentIndex(int edge[2], int triangle[3])
{
	int pnts_tmp[3] = { triangle[0], triangle[1], triangle[2] };
	int c = 0;
	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			if (edge[i] == triangle[j]) { pnts_tmp[j] = -1; c++; break; }
		}
	}

	if (c == 2)
	{
		if (pnts_tmp[0] != -1) return pnts_tmp[0];
		else if (pnts_tmp[1] != -1) return pnts_tmp[1];
		else return pnts_tmp[2];
	}
	return -1;
}

std::vector<Data::TriangleAdjacency> Data::calculateAdjacency() const
{
	std::vector<TriangleAdjacency> output;
	if (!isCorrect()) return output;

	size_t trianglesCount = m_indexBuffer.size() / 3;
	output.reserve(trianglesCount);

	TriangleAdjacency t;
	int edge[2];
	int triangle[3];
	for (size_t i = 0; i < trianglesCount; i++)
	{
		t.points[0] = m_indexBuffer[i * 3];
		t.points[1] = m_indexBuffer[i * 3 + 1];
		t.points[2] = m_indexBuffer[i * 3 + 2];
		t.adjacentPoints[0] = -1;
		t.adjacentPoints[1] = -1;
		t.adjacentPoints[2] = -1;
		 
		for (size_t j = 0; j < trianglesCount; j++)
		{
			if (j == i) continue;

			triangle[0] = m_indexBuffer[j * 3];
			triangle[1] = m_indexBuffer[j * 3 + 1];
			triangle[2] = m_indexBuffer[j * 3 + 2];

			if (t.adjacentPoints[0] == -1)
			{
				edge[0] = t.points[0]; edge[1] = t.points[1];
				t.adjacentPoints[0] = GetAdjacentIndex(edge, triangle);
			}

			if (t.adjacentPoints[1] == -1)
			{
				edge[0] = t.points[1]; edge[1] = t.points[2];
				t.adjacentPoints[1] = GetAdjacentIndex(edge, triangle);
			}

			if (t.adjacentPoints[2] == -1)
			{
				edge[0] = t.points[2]; edge[1] = t.points[0];
				t.adjacentPoints[2] = GetAdjacentIndex(edge, triangle);
			}

			if (t.adjacentPoints[0] != -1 && t.adjacentPoints[1] != -1 && t.adjacentPoints[2] != -1) break;
		}

		output.push_back(t);
	}

	return output;
}

}
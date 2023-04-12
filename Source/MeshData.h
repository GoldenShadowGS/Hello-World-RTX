#pragma once
#include "PCH.h"

//struct Vertex
//{
//	float x, y, z, nx, ny, nz, u, v;
//	UINT matID;
//};

struct Vertex
{
	float x, y, z;
};

struct StructuredVertex
{
	Vertex normal;
	float pad;
	Vertex color;
	float pad2;
};

struct MeshData
{
	std::vector<Vertex> m_Vertices;
	std::vector<UINT> m_Indices;
	UINT stride;
};

inline Vertex Cross(Vertex v1, Vertex v2)
{
	return { v1.y * v2.z - v1.z * v2.y,	v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x };
}

inline Vertex Subtract(Vertex v1, Vertex v2)
{
	return { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
}
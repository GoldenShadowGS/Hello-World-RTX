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
	float normal[4];
	float color[4];
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

inline Vertex Divide(Vertex v, float s)
{
	return { v.x / s, v.y / s, v.z / s };
}

inline float LengthSq(Vertex v)
{
	return v.x * v.x + v.y * v.y + v.z * v.z;
}

inline float Length(Vertex v)
{
	return sqrt(LengthSq(v));
}

inline Vertex Normalize(Vertex v)
{
	return Divide(v,Length(v));
}
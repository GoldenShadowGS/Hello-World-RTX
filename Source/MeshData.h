#pragma once
#include "PCH.h"

struct Vertex
{
	float x, y, z, nx, ny, nz, u, v;
	UINT matID;
};

struct MeshData
{
	std::vector<Vertex> m_Vertices;
	std::vector<UINT> m_Indices;
	UINT stride = sizeof(Vertex);
};
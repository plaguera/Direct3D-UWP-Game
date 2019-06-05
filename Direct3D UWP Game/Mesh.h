#pragma once
#include "pch.h"
#include <sstream>
#include <Windows.h>
#include <iostream>

using namespace DirectX;

struct Vertex {

	XMFLOAT3 pos;
	XMFLOAT4 col;

};

class Mesh
{
public:
	Mesh();
	~Mesh();

	unsigned int GetVSize() const;
	unsigned int GetISize() const;

	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;

private:
	unsigned int vsize;
	unsigned int isize;
};

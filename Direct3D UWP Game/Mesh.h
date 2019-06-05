#pragma once
#include "pch.h"
#include <iostream>
#include <filesystem>

using namespace DirectX;

struct Vertex {
	XMFLOAT3 pos;
	XMFLOAT4 col;
	XMFLOAT3 normal;
};

class Mesh
{
public:
	Mesh();
	Mesh(std::string const fileName);
	~Mesh();

	unsigned int GetVSize() const;
	unsigned int GetISize() const;
	void readFile(std::string fileName);

	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;

private:
	unsigned int vsize;
	unsigned int isize;
	XMFLOAT4 defaultColor;
};

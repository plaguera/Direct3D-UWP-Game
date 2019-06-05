#include "pch.h"
#include "Mesh.h"

Mesh::Mesh()
{
	vertices = {
		{XMFLOAT3(0.0f,0.0f,-0.15f),XMFLOAT4(Colors::Red)},		// 0
		{XMFLOAT3(0.0f,0.0f,0.15f),XMFLOAT4(Colors::Red)},		// 1

		{XMFLOAT3(0.0f,0.5f,0.0f),XMFLOAT4(Colors::Blue)},		// 2
		{XMFLOAT3(0.48f,0.15f,0.0f),XMFLOAT4(Colors::Blue)},	// 3
		{XMFLOAT3(-0.48f,0.15f,0.0f),XMFLOAT4(Colors::Blue)},	// 4
		{XMFLOAT3(0.29f,-0.4f,0.0f),XMFLOAT4(Colors::Blue)},	// 5
		{XMFLOAT3(-0.29f,-0.4f,0.0f),XMFLOAT4(Colors::Blue)},	// 6
		
		{XMFLOAT3(0.11f,0.15f,0.0f),XMFLOAT4(Colors::Green)},	// 7
		{XMFLOAT3(-0.11f,0.15f,0.0f),XMFLOAT4(Colors::Green)},	// 8
		{XMFLOAT3(0.18f,-0.06f,0.0f),XMFLOAT4(Colors::Green)},	// 9
		{XMFLOAT3(-0.18f,-0.06f,0.0f),XMFLOAT4(Colors::Green)},	// 10
		{XMFLOAT3(0.0f,-0.19f,0.0f),XMFLOAT4(Colors::Green)}	// 11
	};
	/*
		0,1,2,
		0,2,3,
		4,6,5,
		5,7,6,
		4,5,1,
		4,1,0,
		3,2,6,
		3,6,7,
		1,5,6,
		1,6,2,
		4,0,3,
		4,3,7
		*/

		/*
		4, 0,8,
		8, 0,2,*/
	indices = {
		2,1,7,
		7,1,3,
		3,1,9,
		9,1,5,
		5,1,11,
		11,1,6,
		6,1,10,
		10,1,4,
		4,1,8,
		8,1,2,
		2,7,0,
		7,3,0,
		3,9,0,
		9,5,0,
		5,11,0,
		11,6,0,
		6,10,0,
		10,4,0,
		4,8,0,
		8,2,0
	};

	unsigned int sv = sizeof(Vertex);
	unsigned int nvertices = vertices.size();
	vsize = nvertices * sv;
	unsigned int sind = sizeof(unsigned int);
	unsigned int nindices = indices.size();
	isize = nindices * sind;
	std::wostringstream ss;
	ss << sind << " * " << nindices << " = " << isize << "\n";
	OutputDebugStringW(ss.str().c_str());
}


Mesh::~Mesh()
{
}

unsigned int Mesh::GetVSize() const {
	return vsize;
}

unsigned int Mesh::GetISize() const {
	return isize;
}
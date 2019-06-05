//
// Game.h
//

#pragma once

#include "HelperFunctions.h"
#include "Mesh.h"
#include "StepTimer.h"


// A basic game implementation that creates a D3D12 device and
// provides a game loop.
class Game
{
public:

    Game() noexcept ;

    // Initialization and management
    void Initialize(::IUnknown* window, int width, int height, DXGI_MODE_ROTATION rotation);

    // Basic game loop
    void Tick();

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowSizeChanged(int width, int height, DXGI_MODE_ROTATION rotation);
    void ValidateDevice();

    // Properties
    void GetDefaultSize( int& width, int& height ) const;

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();
    void Present();

    void CreateDevice();
    void CreateResources();

    void WaitForGpu();
    void MoveToNextFrame();
    void GetAdapter(IDXGIAdapter1** ppAdapter);

    void OnDeviceLost();

    // Application state
    IUnknown*                                           m_window;
    int                                                 m_outputWidth;
    int                                                 m_outputHeight;
    DXGI_MODE_ROTATION                                  m_outputRotation;

    // Direct3D Objects
    D3D_FEATURE_LEVEL                                   m_featureLevel;
    static const UINT                                   c_swapBufferCount = 2;
    UINT                                                m_backBufferIndex;
    UINT                                                m_rtvDescriptorSize;
    Microsoft::WRL::ComPtr<ID3D12Device>                m_d3dDevice;
    Microsoft::WRL::ComPtr<IDXGIFactory4>               m_dxgiFactory;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>          m_commandQueue;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_rtvDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_dsvDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>      m_commandAllocators[c_swapBufferCount];
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   m_commandList;
    Microsoft::WRL::ComPtr<ID3D12Fence>                 m_fence;
    UINT64                                              m_fenceValues[c_swapBufferCount];
    Microsoft::WRL::Wrappers::Event                     m_fenceEvent;

    // Rendering resources
    Microsoft::WRL::ComPtr<IDXGISwapChain3>             m_swapChain;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_renderTargets[c_swapBufferCount];
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_depthStencil;

    // Game state
    DX::StepTimer                                       m_timer;

	void CreateMainInputFlowResources(const Mesh& mesh);
	Mesh												m_mesh { std::string("mesh.dat") };

	D3D12_VERTEX_BUFFER_VIEW							m_vBufferView;
	D3D12_INDEX_BUFFER_VIEW								m_iBufferView;
	Microsoft::WRL::ComPtr<ID3D12RootSignature>         m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12Resource>              m_vBufferDefault; // Buffer para v�rtices
	Microsoft::WRL::ComPtr<ID3D12Resource>              m_vBufferUpload; // Buffer para v�rtices
	Microsoft::WRL::ComPtr<ID3D12Resource>              m_iBufferDefault; // Buffer para v�rtices
	Microsoft::WRL::ComPtr<ID3D12Resource>              m_iBufferUpload; // Buffer para v�rtices
	Microsoft::WRL::ComPtr<ID3D12Resource>				m_vConstantBuffer; // Buffer de constantes
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		m_cDescriptorHeap;
	unsigned int m_cDescriptorSize;

	struct vConstants {

		//DirectX::XMFLOAT4X4 GTransform;
		DirectX::XMFLOAT4X4 GTransform = { 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0 };
		DirectX::XMFLOAT4X4 GNormalTransform = { 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0 };

	} m_vConstants;

	void LoadPrecompiledShaders();

	Microsoft::WRL::ComPtr<ID3DBlob>					m_vsByteCode;
	Microsoft::WRL::ComPtr<ID3DBlob>					m_psByteCode;
	D3D12_SHADER_BYTECODE								m_vs;
	D3D12_SHADER_BYTECODE								m_ps;

	void PSO();
	std::vector<D3D12_INPUT_ELEMENT_DESC>				m_inputLayout;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC					m_psoDescriptor;
	Microsoft::WRL::ComPtr<ID3D12PipelineState>			m_pso;

	XMFLOAT4X4											m_world;
	XMFLOAT4X4											m_view;
	XMFLOAT4X4											m_projection;
	float count = 0.0f;
};

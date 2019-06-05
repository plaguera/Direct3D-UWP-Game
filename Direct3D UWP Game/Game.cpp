//
// Game.cpp
//

#include "pch.h"
#include "Game.h"

extern void ExitGame();

using namespace DirectX;

using Microsoft::WRL::ComPtr;

Game::Game() noexcept :
    m_window(nullptr),
    m_outputWidth(800),
    m_outputHeight(600),
    m_outputRotation(DXGI_MODE_ROTATION_IDENTITY),
    m_featureLevel(D3D_FEATURE_LEVEL_11_0),
    m_backBufferIndex(0),
    m_rtvDescriptorSize(0),
    m_fenceValues{}
{
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(::IUnknown* window, int width, int height, DXGI_MODE_ROTATION rotation)
{
	m_window = window; // Inicializamos a la ventana de visualización.
	m_outputWidth = std::max(width, 1); // Ancho y alto
	m_outputHeight = std::max(height, 1);
	m_outputRotation = rotation; // Rotación.

	CreateDevice(); // Creamos el dispotivo
	CreateResources(); // Creamos recursos que dependen del tamaño de la ventana de visualización

	CreateMainInputFlowResources(m_mesh); //Creamos recursos y objetos D3D12 que permiten el flujo de entrada de datos al pipeline
	LoadPrecompiledShaders(); // Cargamos shaders precompilados

	PSO(); // Creamos un estado del pipeline básico.

	// Inicializamos matrices de transformación
	XMStoreFloat4x4(&m_world, XMMatrixIdentity());
	XMStoreFloat4x4(&m_view, XMMatrixIdentity());
	XMStoreFloat4x4(&m_projection, XMMatrixIdentity());
}

// Executes the basic game loop.
void Game::Tick()
{
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();
}
static float radius = 3.0f;
static float theta = (3.0f * XM_PI) / 2.0f;
static float phi = XM_PI / 2;
// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{
	float elapsedTime = float(timer.GetElapsedSeconds());

	// TODO: Actualización de las transformaciones en la escena

	float x = radius * cosf(theta+count) * sinf(phi);
	float y = radius/2.0;//* cosf(phi-count);
	float z = radius * sinf(theta+count) * sinf(phi);
	count += 0.035;
	// Recalculamos la matriz de vista
	XMVECTOR location = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorSet(0.0, 0.0, 0.0, 1.0f);
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX view = XMMatrixLookAtLH(location, target, up);
	XMStoreFloat4x4(&m_view, view);

	XMMATRIX world = XMLoadFloat4x4(&m_world);
	XMMATRIX projection = XMMatrixPerspectiveFovLH(0.25*XM_PI, m_outputWidth / m_outputHeight, 0.5f, 1000.0f);
	XMMATRIX transform = world * view*projection;

	XMStoreFloat4x4(&m_vConstants.GTransform, XMMatrixTranspose(transform));

	// Actualización del buffer de constantes

	//void* data=nullptr;
	void* data;

	m_vConstantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&data)); // realizamos el mapeo
	memcpy(data, reinterpret_cast<const void*>(&m_vConstants), sizeof(vConstants)); //Copia de la transformación
	if (m_vConstantBuffer != nullptr)
		m_vConstantBuffer->Unmap(0, nullptr);
	elapsedTime;
}

// Draws the scene.
void Game::Render()
{
	// Don't try to render anything before the first Update.
	if (m_timer.GetFrameCount() == 0)
	{
		return;
	}

	// Prepare the command list to render a new frame.
	Clear();

	// TODO: Add your rendering code here.
	m_commandList->DrawIndexedInstanced(240, 1, 0, 0, 0);
	// Show the new frame.
	Present();
}

// Helper method to prepare the command list for rendering and clear the back buffers.
void Game::Clear()
{
	// Reset command list and allocator.
	DX::ThrowIfFailed(m_commandAllocators[m_backBufferIndex]->Reset());
	DX::ThrowIfFailed(m_commandList->Reset(m_commandAllocators[m_backBufferIndex].Get(), m_pso.Get())); // Nota: establecer el PSO.

	// Transition the render target into the correct state to allow for drawing into it.
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_backBufferIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_commandList->ResourceBarrier(1, &barrier);

	// Clear the views.
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptor(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_backBufferIndex, m_rtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvDescriptor(m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	m_commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
	m_commandList->ClearRenderTargetView(rtvDescriptor, Colors::CornflowerBlue, 0, nullptr);
	m_commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// Set the viewport and scissor rect.
	D3D12_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>(m_outputWidth), static_cast<float>(m_outputHeight), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
	D3D12_RECT scissorRect = { 0, 0, m_outputWidth, m_outputHeight };
	m_commandList->RSSetViewports(1, &viewport);
	m_commandList->RSSetScissorRects(1, &scissorRect);

	/*  Establecemos en el pipeline la root signauture:
	La utilización de la root signature lleva tres acciones en la lista de comandos:
	a) Establecer la root signature
	b) Establecer un array de heaps de descriptores: en este caso el root parameter 0, es un descriptor table.
	Esto es un rango de descriptores, procedentes de un heap de descriptores.
	c) Establecer el punto de comienzo del rango de descriptores a usar (se referencia a partir del heap anterior)

	La lista de comandos que haga el Draw debe establecer la root signature
	*/
	// Subtarea a: Establecer la root signature
	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

	// Subtarea b: b Establecer un array de descriptor heaps
	ID3D12DescriptorHeap* arrayHeaps[] = { m_cDescriptorHeap.Get() };
	m_commandList->SetDescriptorHeaps(_countof(arrayHeaps), arrayHeaps);

	// Subtarea c Establece el punto de comienzo del rango de descriptores.

	CD3DX12_GPU_DESCRIPTOR_HANDLE cHandle(m_cDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	//cHandle.Offset(0, m_cDescriptorSize);

	m_commandList->SetGraphicsRootDescriptorTable(0, // número de root parameter
		cHandle //manejador a la posición del heap donde comenzaría el rango de descriptores
	);

	// Todo: Establecemos la vista para el buffer de indices
	D3D12_VERTEX_BUFFER_VIEW aVBufferView[1] = { m_vBufferView }; // Es necesario pasar un array de buffer views
	m_commandList->IASetVertexBuffers(0, 1, aVBufferView);
	D3D12_INDEX_BUFFER_VIEW aIBufferView[1] = { m_iBufferView }; // Es necesario pasar un array de buffer views
	m_commandList->IASetIndexBuffer(aIBufferView);

	/* Establecemos la topología: es obligatorio*/
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

// Submits the command list to the GPU and presents the back buffer contents to the screen.
void Game::Present()
{
    // Transition the render target to the state that allows it to be presented to the display.
    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_backBufferIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    m_commandList->ResourceBarrier(1, &barrier);

    // Send the command list off to the GPU for processing.
    DX::ThrowIfFailed(m_commandList->Close());
    m_commandQueue->ExecuteCommandLists(1, CommandListCast(m_commandList.GetAddressOf()));

    // The first argument instructs DXGI to block until VSync, putting the application
    // to sleep until the next VSync. This ensures we don't waste any cycles rendering
    // frames that will never be displayed to the screen.
    HRESULT hr = m_swapChain->Present(1, 0);

    // If the device was reset we must completely reinitialize the renderer.
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        OnDeviceLost();
    }
    else
    {
        DX::ThrowIfFailed(hr);

        MoveToNextFrame();
    }
}

// Message handlers
void Game::OnActivated()
{
    // TODO: Game is becoming active window.
}

void Game::OnDeactivated()
{
    // TODO: Game is becoming background window.
}

void Game::OnSuspending()
{
    // TODO: Game is being power-suspended.
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();

    // TODO: Game is being power-resumed.
}

void Game::OnWindowSizeChanged(int width, int height, DXGI_MODE_ROTATION rotation)
{
    m_outputWidth = std::max(width, 1);
    m_outputHeight = std::max(height, 1);
    m_outputRotation = rotation;

    CreateResources();

    // TODO: Game window is being resized.
}

void Game::ValidateDevice()
{
    // The D3D Device is no longer valid if the default adapter changed since the device
    // was created or if the device has been removed.

    DXGI_ADAPTER_DESC previousDesc;
    {
        ComPtr<IDXGIAdapter1> previousDefaultAdapter;
        DX::ThrowIfFailed(m_dxgiFactory->EnumAdapters1(0, previousDefaultAdapter.GetAddressOf()));

        DX::ThrowIfFailed(previousDefaultAdapter->GetDesc(&previousDesc));
    }

    DXGI_ADAPTER_DESC currentDesc;
    {
        ComPtr<IDXGIFactory4> currentFactory;
        DX::ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(currentFactory.GetAddressOf())));

        ComPtr<IDXGIAdapter1> currentDefaultAdapter;
        DX::ThrowIfFailed(currentFactory->EnumAdapters1(0, currentDefaultAdapter.GetAddressOf()));

        DX::ThrowIfFailed(currentDefaultAdapter->GetDesc(&currentDesc));
    }

    // If the adapter LUIDs don't match, or if the device reports that it has been removed,
    // a new D3D device must be created.

    if (previousDesc.AdapterLuid.LowPart != currentDesc.AdapterLuid.LowPart
        || previousDesc.AdapterLuid.HighPart != currentDesc.AdapterLuid.HighPart
        || FAILED(m_d3dDevice->GetDeviceRemovedReason()))
    {
        // Create a new device and swap chain.
        OnDeviceLost();
    }
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const
{
    // TODO: Change to desired default window size (note minimum size is 320x200).
    width = 800;
    height = 600;
}

// These are the resources that depend on the device.
void Game::CreateDevice()
{
#if defined(_DEBUG)
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    //
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf()))))
        {
            debugController->EnableDebugLayer();
        }
    }
#endif

    DX::ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(m_dxgiFactory.ReleaseAndGetAddressOf())));

    ComPtr<IDXGIAdapter1> adapter;
    GetAdapter(adapter.GetAddressOf());

    // Create the DX12 API device object.
    DX::ThrowIfFailed(D3D12CreateDevice(
        adapter.Get(),
        m_featureLevel,
        IID_PPV_ARGS(m_d3dDevice.ReleaseAndGetAddressOf())
        ));

#ifndef NDEBUG
    // Configure debug device (if active).
    ComPtr<ID3D12InfoQueue> d3dInfoQueue;
    if (SUCCEEDED(m_d3dDevice.As(&d3dInfoQueue)))
    {
#ifdef _DEBUG
        d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
#endif
        D3D12_MESSAGE_ID hide[] =
        {
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
        };
        D3D12_INFO_QUEUE_FILTER filter = {};
        filter.DenyList.NumIDs = _countof(hide);
        filter.DenyList.pIDList = hide;
        d3dInfoQueue->AddStorageFilterEntries(&filter);
    }
#endif

    // Create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    DX::ThrowIfFailed(m_d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(m_commandQueue.ReleaseAndGetAddressOf())));

    // Create descriptor heaps for render target views and depth stencil views.
    D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc = {};
    rtvDescriptorHeapDesc.NumDescriptors = c_swapBufferCount;
    rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc = {};
    dsvDescriptorHeapDesc.NumDescriptors = 1;
    dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

    DX::ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(m_rtvDescriptorHeap.ReleaseAndGetAddressOf())));
    DX::ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_PPV_ARGS(m_dsvDescriptorHeap.ReleaseAndGetAddressOf())));

    m_rtvDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Create a command allocator for each back buffer that will be rendered to.
    for (UINT n = 0; n < c_swapBufferCount; n++)
    {
        DX::ThrowIfFailed(m_d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_commandAllocators[n].ReleaseAndGetAddressOf())));
    }

    // Create a command list for recording graphics commands.
    DX::ThrowIfFailed(m_d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[0].Get(), nullptr, IID_PPV_ARGS(m_commandList.ReleaseAndGetAddressOf())));
    //DX::ThrowIfFailed(m_commandList->Close());

    // Create a fence for tracking GPU execution progress.
    DX::ThrowIfFailed(m_d3dDevice->CreateFence(m_fenceValues[m_backBufferIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.ReleaseAndGetAddressOf())));
    m_fenceValues[m_backBufferIndex]++;

    m_fenceEvent.Attach(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
    if (!m_fenceEvent.IsValid())
    {
        throw std::exception("CreateEvent");
    }

    // TODO: Initialize device dependent objects here (independent of window size).
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateResources()
{
    // Wait until all previous GPU work is complete.
    WaitForGpu();

    // Release resources that are tied to the swap chain and update fence values.
    for (UINT n = 0; n < c_swapBufferCount; n++)
    {
        m_renderTargets[n].Reset();
        m_fenceValues[n] = m_fenceValues[m_backBufferIndex];
    }

    DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;
    UINT backBufferWidth = static_cast<UINT>(m_outputWidth);
    UINT backBufferHeight = static_cast<UINT>(m_outputHeight);

    // If the swap chain already exists, resize it, otherwise create one.
    if (m_swapChain)
    {
        HRESULT hr = m_swapChain->ResizeBuffers(c_swapBufferCount, backBufferWidth, backBufferHeight, backBufferFormat, 0);

        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            // If the device was removed for any reason, a new device and swap chain will need to be created.
            OnDeviceLost();

            // Everything is set up now. Do not continue execution of this method. OnDeviceLost will reenter this method
            // and correctly set up the new device.
            return;
        }
        else
        {
            DX::ThrowIfFailed(hr);
        }
    }
    else
    {
        // Create a descriptor for the swap chain.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = backBufferWidth;
        swapChainDesc.Height = backBufferHeight;
        swapChainDesc.Format = backBufferFormat;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = c_swapBufferCount;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

        // Create a swap chain for the window.
        ComPtr<IDXGISwapChain1> swapChain;
        DX::ThrowIfFailed(m_dxgiFactory->CreateSwapChainForCoreWindow(
            m_commandQueue.Get(),
            m_window,
            &swapChainDesc,
            nullptr,
            swapChain.GetAddressOf()
            ));

        DX::ThrowIfFailed(swapChain.As(&m_swapChain));
    }

    DX::ThrowIfFailed(m_swapChain->SetRotation(m_outputRotation));

    // Obtain the back buffers for this window which will be the final render targets
    // and create render target views for each of them.
    for (UINT n = 0; n < c_swapBufferCount; n++)
    {
        DX::ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(m_renderTargets[n].GetAddressOf())));

        wchar_t name[25] = {};
        swprintf_s(name, L"Render target %u", n);
        m_renderTargets[n]->SetName(name);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptor(
            m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
            static_cast<INT>(n), m_rtvDescriptorSize);
        m_d3dDevice->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvDescriptor);
    }

    // Reset the index to the current back buffer.
    m_backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

    // Allocate a 2-D surface as the depth/stencil buffer and create a depth/stencil view
    // on this surface.
    CD3DX12_HEAP_PROPERTIES depthHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

    D3D12_RESOURCE_DESC depthStencilDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        depthBufferFormat,
        backBufferWidth,
        backBufferHeight,
        1, // This depth stencil view has only one texture.
        1  // Use a single mipmap level.
        );
    depthStencilDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
    depthOptimizedClearValue.Format = depthBufferFormat;
    depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
    depthOptimizedClearValue.DepthStencil.Stencil = 0;

    DX::ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
        &depthHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &depthOptimizedClearValue,
        IID_PPV_ARGS(m_depthStencil.ReleaseAndGetAddressOf())
        ));

    m_depthStencil->SetName(L"Depth stencil");

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = depthBufferFormat;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

    m_d3dDevice->CreateDepthStencilView(m_depthStencil.Get(), &dsvDesc, m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    // TODO: Initialize windows-size dependent objects here.
}

void Game::WaitForGpu()
{
    // Schedule a Signal command in the GPU queue.
    DX::ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValues[m_backBufferIndex]));

    // Wait until the Signal has been processed.
    DX::ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_backBufferIndex], m_fenceEvent.Get()));
    WaitForSingleObjectEx(m_fenceEvent.Get(), INFINITE, FALSE);

    // Increment the fence value for the current frame.
    m_fenceValues[m_backBufferIndex]++;
}

void Game::MoveToNextFrame()
{
    // Schedule a Signal command in the queue.
    const UINT64 currentFenceValue = m_fenceValues[m_backBufferIndex];
    DX::ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), currentFenceValue));

    // Update the back buffer index.
    m_backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

    // If the next frame is not ready to be rendered yet, wait until it is ready.
    if (m_fence->GetCompletedValue() < m_fenceValues[m_backBufferIndex])
    {
        DX::ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_backBufferIndex], m_fenceEvent.Get()));
        WaitForSingleObjectEx(m_fenceEvent.Get(), INFINITE, FALSE);
    }

    // Set the fence value for the next frame.
    m_fenceValues[m_backBufferIndex] = currentFenceValue + 1;
}

// This method acquires the first available hardware adapter that supports Direct3D 12.
// If no such adapter can be found, try WARP. Otherwise throw an exception.
void Game::GetAdapter(IDXGIAdapter1** ppAdapter)
{
    *ppAdapter = nullptr;

    ComPtr<IDXGIAdapter1> adapter;
    for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != m_dxgiFactory->EnumAdapters1(adapterIndex, adapter.ReleaseAndGetAddressOf()); ++adapterIndex)
    {
        DXGI_ADAPTER_DESC1 desc;
        DX::ThrowIfFailed(adapter->GetDesc1(&desc));

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            // Don't select the Basic Render Driver adapter.
            continue;
        }

        // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), m_featureLevel, _uuidof(ID3D12Device), nullptr)))
        {
            break;
        }
    }

#if !defined(NDEBUG)
    if (!adapter)
    {
        if (FAILED(m_dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf()))))
        {
            throw std::exception("WARP12 not available. Enable the 'Graphics Tools' optional feature");
        }
    }
#endif

    if (!adapter)
    {
        throw std::exception("No Direct3D 12 device found");
    }

    *ppAdapter = adapter.Detach();
}

void Game::OnDeviceLost()
{
    // TODO: Perform Direct3D resource cleanup.

    for (UINT n = 0; n < c_swapBufferCount; n++)
    {
        m_commandAllocators[n].Reset();
        m_renderTargets[n].Reset();
    }

    m_depthStencil.Reset();
    m_fence.Reset();
    m_commandList.Reset();
    m_swapChain.Reset();
    m_rtvDescriptorHeap.Reset();
    m_dsvDescriptorHeap.Reset();
    m_commandQueue.Reset();
    m_d3dDevice.Reset();
    m_dxgiFactory.Reset();

    CreateDevice();
    CreateResources();
}

void Game::CreateMainInputFlowResources(const Mesh& mesh) {

	/*
	Objetivo 1.
	Comenzamos por preparar los buffers de vértices y de índices. Estos buffers se van a cargar
	con datos al principio, y luego no se van a actualizar por cada frame. Por eso es mejor
	crearlos en un heap de tipo DEFAULT al que accede con mayor eficiencia la GPU. Por contra
	la CPU no puede cargar datos en un DEFAULT. Entonces hay que crear un segundo buffer en un heap
	de tipo UPLOAD y transferir del UPLOAD al DEFAULT. Los buffer de vértices e índices no requieren
	un heap de descriptores, se conectan al pipeline directamente con una vista.

	*/

	/*Tarea 1: Creación de los buffers.*/

	// Creación de los buffers default y upload para los vértices
	DX::ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(mesh.GetVSize()),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(m_vBufferDefault.GetAddressOf())));

	// Ahora un buffer upload, de esta forma tenemos un puente upload para pasar a default.

	DX::ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(mesh.GetVSize()),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_vBufferUpload.GetAddressOf())));

	// Creación de los buffers default y upload para los índices
	DX::ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(mesh.GetISize()),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(m_iBufferDefault.GetAddressOf())));

	// Ahora un buffer upload, de esta forma tenemos un puente upload para pasar a default.

	DX::ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(mesh.GetISize()),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_iBufferUpload.GetAddressOf())));

	/*Tarea 2: Preparamos el origen de los datos de vértices e índices.*/

	// Preparamos el  origen_vertices
	D3D12_SUBRESOURCE_DATA origen_vertices = {};
	origen_vertices.pData = &mesh.vertices[0];
	origen_vertices.RowPitch = mesh.GetVSize();
	origen_vertices.SlicePitch = origen_vertices.RowPitch;

	D3D12_SUBRESOURCE_DATA origen_indices = {};
	origen_indices.pData = &mesh.indices[0];
	origen_indices.RowPitch = mesh.GetISize();
	origen_indices.SlicePitch = origen_indices.RowPitch;

	/*Tarea 3: Realizamos la transferencia desde el origen hasta el buffer DEFAULT pasando por el buffer UPLOAD*/
	/*Vértices*/
	// Cambio de estado en el recurso de destino
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		m_vBufferDefault.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));

	// Ordenamos la transferencia vertices
	UpdateSubresources<1>(m_commandList.Get(), m_vBufferDefault.Get(), m_vBufferUpload.Get(), 0, 0, 1, &origen_vertices);
	// Cambio de estado en el recurso de destino
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		m_vBufferDefault.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

	/*Índices*/
	// Cambio de estado en el recurso de destino
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		m_iBufferDefault.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));

	// Ordenamos la transferencia vertices
	UpdateSubresources<1>(m_commandList.Get(), m_iBufferDefault.Get(), m_iBufferUpload.Get(), 0, 0, 1, &origen_indices);
	// Cambio de estado en el recurso de destino
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		m_iBufferDefault.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));


	/* Tarea 3: Establecemos una vista para vértices e índices*/
	// Establecemos la vista (descriptor) para el buffer de vértices
	/* Vértices*/

	m_vBufferView.BufferLocation = m_vBufferDefault->GetGPUVirtualAddress();
	m_vBufferView.StrideInBytes = sizeof(Vertex);
	m_vBufferView.SizeInBytes = mesh.GetVSize();

	D3D12_VERTEX_BUFFER_VIEW aVBufferView[1] = { m_vBufferView }; // Es necesario pasar un array de buffer views

	m_commandList->IASetVertexBuffers(0, 1, aVBufferView);

	/* Índices*/

	m_iBufferView.BufferLocation = m_iBufferDefault->GetGPUVirtualAddress();
	m_iBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_iBufferView.SizeInBytes = mesh.GetISize();

	// Todo: Establecemos la vista para el buffer de indices

	D3D12_INDEX_BUFFER_VIEW aIBufferView[1] = { m_iBufferView }; // Es necesario pasar un array de buffer views

	m_commandList->IASetIndexBuffer(aIBufferView);
	/*------------------------- Fin Objetivo 1  ----------------------------------------------------------------------------*/


	/*
	Objetivo 2: Configurar un buffer de constantes para el shader de vértices
	Este buffer contiene datos que pueden ser modificados en cada frame (por ejemplo, transformaciones)
	Por eso debe ser un buffer de tipo UPLOAD necesariamente.
	*/
	// El buffer de constantes para el shader de vértices

	/* Tarea 1: Crear el buffer en un heap upload*/
	unsigned int elementSize = CalcConstantBufferByteSize(sizeof(m_vConstants));

	m_d3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(elementSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_vConstantBuffer)
	);

	/* Tarea 2: crear un heap de descriptores*/

	// El heap de descriptores para los buffers de constantes
	D3D12_DESCRIPTOR_HEAP_DESC cHeapDescriptor;
	cHeapDescriptor.NumDescriptors = 1;
	cHeapDescriptor.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cHeapDescriptor.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cHeapDescriptor.NodeMask = 0;

	m_d3dDevice->CreateDescriptorHeap(&cHeapDescriptor, IID_PPV_ARGS(&m_cDescriptorHeap));

	/* Tarea 3: crear un descriptor para el buffer*/
	// Ahora el descriptor de la vista buffer de constantes
	D3D12_GPU_VIRTUAL_ADDRESS cAddress = m_vConstantBuffer->GetGPUVirtualAddress();
	D3D12_CONSTANT_BUFFER_VIEW_DESC cDescriptor;
	cDescriptor.BufferLocation = cAddress;
	cDescriptor.SizeInBytes = CalcConstantBufferByteSize(sizeof(vConstants));
	m_cDescriptorSize = sizeof(cDescriptor);
	// Creamos el descriptor y lo colocamos al principio del heap de descriptores.
	m_d3dDevice->CreateConstantBufferView(&cDescriptor, m_cDescriptorHeap->GetCPUDescriptorHandleForHeapStart());


	/*
	Objetivo 3: Creamos una root signature ya que usaremos registros de los shaders conectados a recursos y
	la configuramos para ser usada por el pipeline
	*/
	// Establecemos la root signature

/* Tarea 1: Crear un array de root parameters*/

	CD3DX12_ROOT_PARAMETER rootParameters[1]; //Array de root parameters
	// Creamos un rango de tablas de descriptores
	CD3DX12_DESCRIPTOR_RANGE cTable;
	cTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

	rootParameters[0].InitAsDescriptorTable(1, // Número de rangos 
		&cTable); // Puntero a un array de rangos.

/* Tarea 2: Creamos una descripción de la root signature y la serializamos */
	// Descripción de la root signature
	CD3DX12_ROOT_SIGNATURE_DESC rsDescription(1, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	//Debemos serializar la descripción root signature
	Microsoft::WRL::ComPtr<ID3DBlob> serializado = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> error = nullptr;
	D3D12SerializeRootSignature(&rsDescription, D3D_ROOT_SIGNATURE_VERSION_1, serializado.GetAddressOf(), error.GetAddressOf());

	/* Tarea 3: Creamos la root signature*/
		//Con la descripción serializada creamos el componente ID3DRootSignature
	DX::ThrowIfFailed(m_d3dDevice->CreateRootSignature(
		0,
		serializado->GetBufferPointer(),
		serializado->GetBufferSize(),
		IID_PPV_ARGS(&m_rootSignature)));

	// Cerramos la lista de comandos, lanzamos la ejeución de los mismos
	m_commandList->Close();
	m_commandQueue->ExecuteCommandLists(1, CommandListCast(m_commandList.GetAddressOf()));
	DX::ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), 1));

	// Wait until the Signal has been processed.
	DX::ThrowIfFailed(m_fence->SetEventOnCompletion(1, m_fenceEvent.Get()));
	WaitForSingleObjectEx(m_fenceEvent.Get(), INFINITE, FALSE);
}

void Game::LoadPrecompiledShaders() {

	DX::ThrowIfFailed(
		D3DReadFileToBlob(L"vertex.cso", m_vsByteCode.GetAddressOf()));

	DX::ThrowIfFailed(
		D3DReadFileToBlob(L"pixel.cso", m_psByteCode.GetAddressOf()));


	m_vs = { reinterpret_cast<char*>(m_vsByteCode->GetBufferPointer()),
			m_vsByteCode->GetBufferSize() };

	m_ps = { reinterpret_cast<char*>(m_psByteCode->GetBufferPointer()),
			m_psByteCode->GetBufferSize() };

}

void Game::PSO()
{
	// Input Layout
	m_inputLayout = {

		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};

	// Rasterizer state
	CD3DX12_RASTERIZER_DESC rasterizer(D3D12_DEFAULT);
	rasterizer.FillMode = D3D12_FILL_MODE_WIREFRAME;
	rasterizer.CullMode = D3D12_CULL_MODE_NONE;

	ZeroMemory(&m_psoDescriptor, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

	m_psoDescriptor.InputLayout = { m_inputLayout.data(), (unsigned int)m_inputLayout.size() };
	m_psoDescriptor.pRootSignature = m_rootSignature.Get();
	m_psoDescriptor.VS = { m_vs.pShaderBytecode,m_vs.BytecodeLength };
	m_psoDescriptor.PS = { m_ps.pShaderBytecode,m_ps.BytecodeLength };
	m_psoDescriptor.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	//m_psoDescriptor.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	//m_psoDescriptor.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	m_psoDescriptor.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	m_psoDescriptor.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	m_psoDescriptor.SampleMask = UINT_MAX;
	m_psoDescriptor.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	m_psoDescriptor.NumRenderTargets = 1;
	m_psoDescriptor.RTVFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM;
	m_psoDescriptor.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	m_psoDescriptor.SampleDesc.Count = 1;
	m_psoDescriptor.SampleDesc.Quality = 0;

	m_d3dDevice->CreateGraphicsPipelineState(&m_psoDescriptor, IID_PPV_ARGS(&m_pso));
}

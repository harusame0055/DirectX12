#include"device_context.h"

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

#include<cassert>
#include<string>

namespace ncc {

#pragma region public

	bool DeviceContext::Initialize(HWND hwnd, int backbuffer_width, int backbuffer_height)
	{
		if (!CreateFactory())
		{
			return false;
		}

		if (!CreateDevice(dxgi_factory_.Get()))
		{
			return false;
		}

		if (!CreateCommandQueue(device_.Get()))
		{
			return false;
		}

		if (!CreateSwapChain(hwnd, dxgi_factory_.Get(), backbuffer_width, backbuffer_height))
		{
			return false;
		}

		if (!CreateRenderTarget(device_.Get(), swap_chain_.Get()))
		{
			return false;
		}

		if (!CreateDepthStencil(device_.Get(), backbuffer_width, backbuffer_height))
		{
			return false;
		}

		if (!CreateCommandAllocator(device_.Get()))
		{
			return false;
		}

		if (!CreateCommandList(device_.Get(), command_allocators_[0].Get()))
		{
			return false;
		}

		if (!CreateFence(device_.Get()))
		{
			return false;
		}

		// �r���[�|�[�g�E�V�U�[��`�ݒ�
		screen_viewport_.TopLeftX = 0.0f;
		screen_viewport_.TopLeftY = 0.0f;
		screen_viewport_.Width = static_cast<FLOAT>(backbuffer_width);
		screen_viewport_.Height = static_cast<FLOAT>(backbuffer_height);
		screen_viewport_.MinDepth = D3D12_MIN_DEPTH;
		screen_viewport_.MaxDepth = D3D12_MAX_DEPTH;

		scissor_rect_.left = 0;
		scissor_rect_.top = 0;
		scissor_rect_.right = backbuffer_width;
		scissor_rect_.bottom = backbuffer_height;

		return true;
	}

	void DeviceContext::Finalize()
	{
		WaitForGpu();

#if _DEBUG
		{
			ComPtr<ID3D12DebugDevice> debug_interface;
			if (SUCCEEDED(
				device_->QueryInterface(IID_PPV_ARGS(&debug_interface))))
			{
				debug_interface->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL |
					D3D12_RLDO_IGNORE_INTERNAL);
			}
		}
#endif
	}

	void DeviceContext::WaitForGpu()
	{
		// ���݂̃t�F���X�l�����[�J���ɃR�s�[
		const UINT64 current_value = fence_values_[backbuffer_index_];

		// �L���[�������ɍX�V�����t�F���X�ƃt�F���X�l���Z�b�g
		if (FAILED(command_queue_->Signal(d3d12_fence_.Get(), current_value)))
		{
			return;
		}

		// �t�F���X�Ƀt�F���X�l�X�V���ɃC�x���g�����s�����悤�ɐݒ�
		if (FAILED(
			d3d12_fence_->SetEventOnCompletion(current_value, fence_event_.Get())))
		{
			return;
		}

		// �C�x���g�����ł���܂őҋ@��Ԃɂ���
		WaitForSingleObjectEx(fence_event_.Get(), INFINITE, FALSE);
		// ����̃t�F���X�p�Ƀt�F���X�l�X�V
		fence_values_[backbuffer_index_] = current_value + 1;
	}

	void DeviceContext::WaitForPreviousFrame()
	{
		// �L���[�ɃV�O�i���𑗂�
		const auto current_value = fence_values_[backbuffer_index_];
		command_queue_->Signal(d3d12_fence_.Get(), current_value);

		// ���t���[���̃o�b�N�o�b�t�@�C���f�b�N�X�����炤
		backbuffer_index_ = swap_chain_->GetCurrentBackBufferIndex();

		// ���̃t���[���̂��߂Ƀt�F���X�l�X�V
		fence_values_[backbuffer_index_] = current_value + 1;

		// �`�揈���ɂ���ē��B�������_�ŕ`�悪�I���t�F���X�l���X�V����Ă���\��������
		// ���̏�Ԃ�WaitForSingleObjectEx������Ɩ����ɑ҂��ƂɂȂ��
		// �����Ȃ�Ȃ��悤��GetCompletedValue�Ńt�F���X�̌��ݒl���m�F����.
		// GetCompletedValue�Ńt�F���X�̌��ݒl���m�F����.
		if (d3d12_fence_->GetCompletedValue() < current_value)
		{
			// �܂��`�悪�I����Ă��Ȃ��̂œ���
			d3d12_fence_->SetEventOnCompletion(current_value, fence_event_.Get());
			WaitForSingleObjectEx(fence_event_.Get(), INFINITE, FALSE);
		}
	}

	void DeviceContext::Present()
	{
		swap_chain_->Present(1, 0);
	}

	ID3D12Device* DeviceContext::device()
	{
		return device_.Get();
	}

	IDXGISwapChain4* DeviceContext::swap_chain()
	{
		return swap_chain_.Get();
	}

	ID3D12GraphicsCommandList* DeviceContext::command_list()
	{
		return graphics_commnadlist_.Get();
	}

	ID3D12CommandAllocator* DeviceContext::current_command_allocator()
	{
		return command_allocators_[backbuffer_index_].Get();
	}

	ID3D12CommandAllocator* DeviceContext::command_allocator(int index)
	{
		assert(index >= 0 && index <= backbuffer_size_);
		return command_allocators_[index].Get();
	}

	ID3D12CommandQueue* DeviceContext::command_queue()
	{
		return command_queue_.Get();
	}

	ID3D12Resource* DeviceContext::current_render_target()
	{
		return render_targets_[backbuffer_index_].Get();
	}

	const D3D12_CPU_DESCRIPTOR_HANDLE DeviceContext::current_rtv() const
	{
		auto rtv
			= rtv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart();
		rtv.ptr += static_cast<SIZE_T>(static_cast<INT64>(backbuffer_index_) *
			static_cast<INT64>(rtv_descriptor_size_));
		return rtv;
	}

	const D3D12_CPU_DESCRIPTOR_HANDLE DeviceContext::dsv() const
	{
		return dsv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart();
	}

	const int DeviceContext::backbuffer_index() const
	{
		return backbuffer_index_;
	}

	const int DeviceContext::backbuffer_size() const
	{
		return backbuffer_size_;
	}

	const D3D12_VIEWPORT& DeviceContext::screen_viewport() const
	{
		return screen_viewport_;
	}

	const D3D12_RECT& DeviceContext::scissor_rect() const
	{
		return scissor_rect_;
	}

#pragma endregion

#pragma region private

	/// @brief �t�@�N�g���쐬�t���O
	/// @retval true ����
	/// @retval false  ���s
	bool DeviceContext::CreateFactory()
	{
		// �t�@�N�g���쐬�t���O
		UINT dxgi_factory_flags = 0;

		// _DEBUG�r���h����D3D12�̃f�o�b�O�@�\�̗L��������
#if _DEBUG
		ComPtr<ID3D12Debug> debug;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
		{
			debug->EnableDebugLayer();
		}

		// DXGI�̃f�o�b�O�@�\�ݒ�
		ComPtr<IDXGIInfoQueue> info_queue;
		if (SUCCEEDED(DXGIGetDebugInterface1(
			0, IID_PPV_ARGS(info_queue.GetAddressOf()))))
		{
			dxgi_factory_flags = DXGI_CREATE_FACTORY_DEBUG;
			// DXGI �ɖ�肪���������Ƀv���O�������u���[�L����悤�ɂ���
			info_queue->SetBreakOnSeverity(
				DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
			info_queue->SetBreakOnSeverity(
				DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
		}
#endif

		// �A�_�v�^���擾���邽�߂̃t�@�N�g���쐬
		if (FAILED(CreateDXGIFactory2(dxgi_factory_flags,
			IID_PPV_ARGS(dxgi_factory_.ReleaseAndGetAddressOf()))))
		{
			return false;
		}

		return true;
	}

	/// @brief �f�o�C�X�쐬
	/// @param factory IDXGIFactory5*
	/// @retval true ����
	/// @retval false ���s
	bool DeviceContext::CreateDevice(IDXGIFactory5* factory)
	{
		// D3D12���g����A�_�v�^�̒T������
		ComPtr<IDXGIAdapter1> adapter; // �A�_�v�^�I�u�W�F�N�g
		{
			UINT adapter_index = 0;

			// dxgi_factory->EnumAdapters1 �ɂ��A�_�v�^�̐��\���`�F�b�N���Ă���
			while (DXGI_ERROR_NOT_FOUND
				!= factory->EnumAdapters1(adapter_index, &adapter))
			{
				DXGI_ADAPTER_DESC1 desc{};
				adapter->GetDesc1(&desc);
				++adapter_index;

				if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				{
					continue;	//	CPUs�����͖���
				}

				// D3D12�͎g�p�\��
				if (SUCCEEDED(D3D12CreateDevice(adapter.Get(),
					D3D_FEATURE_LEVEL_11_0,
					__uuidof(ID3D12Device), nullptr)))
				{
					break; // �g����A�_�v�^���������̂ŒT���𒆒f���Ď��̏�����
				}
			}

#if _DEBUG
			// �K�؂ȃn�[�h�E�F�A�A�_�v�^���Ȃ��AuseWarpAdapter��true�Ȃ�
			// WARP�A�_�v�^���擾
			if (adapter == nullptr && useWarpAdapter)
			{
				// WARP��GPU�@�\�̈ꕔ��CPU���Ŏ��s���Ă����A�_�v�^
				if (FAILED(factory->EnumWarpAdapter(
					IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf()))))
				{
					return false;
				}
			}
#endif

			if (!adapter)
			{
				return false; // �g�p�\�ȃA�_�v�^�[���Ȃ�����
			}
		}

		// �Q�[���Ŏg���f�o�C�X�쐬
		if (FAILED(D3D12CreateDevice(adapter.Get(), // �f�o�C�X�����Ɏg���A�_�v�^
			D3D_FEATURE_LEVEL_11_0,	// D3D�̋@�\���x��
			IID_PPV_ARGS(&device_))))
		{
			return false;
		}
		device_->SetName(L"DeviceContext::device");

		return true;
	}

	/// @brief �R�}���h�L���[�쐬
	/// @param device ID3D12Device
	/// @retval true  ����
	/// @retval false  ���s
	bool DeviceContext::CreateCommandQueue(ID3D12Device* device)
	{
		// �L���[�̐ݒ�p�\����
		D3D12_COMMAND_QUEUE_DESC desc{};
		desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; // ��ʓI�ȕ`��͂���
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

		// �L���[�̍쐬
		if (FAILED(device->CreateCommandQueue(
			&desc,
			IID_PPV_ARGS(command_queue_.ReleaseAndGetAddressOf()))))
		{
			return false;
		}
		command_queue_->SetName(L"DeviceContext::command_queue");

		return true;
	}

	/// @brief �X���b�v�`�F�C���쐬
	/// @param hwnd HWND
	/// @param factory IDXGIFactory5*
	/// @param backbuffer_width �o�b�N�o�b�t�@��
	/// @param backbuffer_height �o�b�N�o�b�t�@����
	/// @retval true ����
	/// @retval false ���s
	bool DeviceContext::CreateSwapChain(HWND hwnd, IDXGIFactory5* factory,
		int backbuffer_width, int backbuffer_height)
	{
		// �X���b�v�`�F�C���̐ݒ�p�\����
		DXGI_SWAP_CHAIN_DESC1 desc{};
		desc.BufferCount = backbuffer_size_; //	�o�b�t�@�̐�
		desc.Width = backbuffer_width;			//	�o�b�N�o�b�t�@�̕�
		desc.Height = backbuffer_height;		//	�o�b�N�o�b�t�@�̍���
		desc.Format = backbuffer_format_;	// �o�b�N�o�b�t�@�t�H�[�}�b�g
		desc.BufferUsage =
			DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.Scaling = DXGI_SCALING_STRETCH;
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		desc.Flags = 0;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;

		// �X���b�v�`�F�C���I�u�W�F�N�g�쐬
		ComPtr<IDXGISwapChain1> sc;
		if (FAILED(factory->CreateSwapChainForHwnd(
			command_queue_.Get(),	// �X���b�v�`�F�C������邽�߂̃R�}���h�L���[
			hwnd,				// �\����ɂȂ�E�B���h�E�n���h��
			&desc,				//	�X���b�v�`�F�C���̐ݒ�
			nullptr,				//	�t���X�N���[���ݒ���w�肵�Ȃ��̂�nullptr
			nullptr,				//	�}���`���j�^�ݒ���w�肵�Ȃ��̂�nullptr
			sc.GetAddressOf())))
		{
			return false;
		}

		// IDXGISwapChain1::As�֐�����IDGISwapChain4�C���^�t�F�[�X���擾
		if (FAILED(sc.As(&swap_chain_)))
		{
			return false;
		}

		// ����`��ɔ����āA���݂̃o�b�N�o�b�t�@�̃C���f�b�N�X�擾
		backbuffer_index_ = swap_chain_->GetCurrentBackBufferIndex();

		return true;
	}

	/// @brief �����_�[�^�[�Q�b�g�쐬
	/// @param device ID3D12Device
	/// @param swap_chain IDXGISwapChain4*
	/// @retval true ����
	/// @retval false ���s
	bool DeviceContext::CreateRenderTarget(ID3D12Device* device, IDXGISwapChain4* swap_chain)
	{
		// �����_�[�^�[�Q�b�g�f�X�N���v�^�q�[�v�쐬

		{
			D3D12_DESCRIPTOR_HEAP_DESC desc{};
			desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	//	���\�[�X�̎��
			desc.NumDescriptors = backbuffer_size_;	//	RT�̐������̖ڐ�����m��
			desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

			if (FAILED(device->CreateDescriptorHeap(
				&desc,
				IID_PPV_ARGS(rtv_descriptor_heap_.ReleaseAndGetAddressOf()))))
			{
				return false;
			}
			rtv_descriptor_heap_->SetName(L"DeviceContext::rtv_descriptor_heap");

			// �f�X�N���v�^�q�[�v�̃������T�C�Y���擾
			rtv_descriptor_size_ = device->GetDescriptorHandleIncrementSize(
				D3D12_DESCRIPTOR_HEAP_TYPE_RTV);	//	HEAP_TYPE_RTV��RT�̃f�X�N���v�^�T�C�Y���w��
		}

		// �����_�[�^�[�Q�b�g�r���[���쐬
		{
			auto rtv_handle = rtv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart();

			// �o�b�N�o�b�t�@�̐����������_�[�^�[�Q�b�g���쐬
			for (int i = 0; i < backbuffer_size_; i++)
			{
				// �X���b�v�`�F�C������o�b�N�o�b�t�@���擾
				if (FAILED(swap_chain->GetBuffer(
					i,	//	�o�b�N�o�b�t�@�C���f�b�N�X
					IID_PPV_ARGS(render_targets_[i].GetAddressOf()))))
				{
					return false;
				}

				// render_targets�ɖ��O��ݒ肷�邽�߂̕�����ϐ�
				std::wstring name{ L"DeviceContext::render_targets[" + std::to_wstring(i) + L"]" };
				render_targets_[i]->SetName(name.c_str());

				// �����_�[�^�[�Q�b�g�r���[�̐ݒ�p�\����
				D3D12_RENDER_TARGET_VIEW_DESC desc{};
				desc.Format = backbuffer_format_;
				desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

				// �r���[�쐬
				device_->CreateRenderTargetView(
					render_targets_[i].Get(),	// RT����
					&desc,					// ���RT�̐ݒ�
					rtv_handle);				// �����ݐ�A�h���X

				// �n���h�������炷
				rtv_handle.ptr += rtv_descriptor_size_;
			}
		}

		return true;
	}

	/// @brief �f�v�X�X�e���V���쐬
	/// @param device ID3D12Device
	/// @param backbuffer_width �o�b�N�o�b�t�@��
	/// @param backvuffer_height �o�b�N�o�b�t�@����
	/// @retval true ����
	/// @retval false ���s
	bool DeviceContext::CreateDepthStencil(ID3D12Device* device,
		int backbuffer_width, int backbuffer_height)
	{
		// �f�v�X�X�e���V���o�b�t�@�{��
		{
			D3D12_RESOURCE_DESC resource_desc{};
			resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			resource_desc.Width = backbuffer_width; // �o�b�N�o�b�t�@��
			resource_desc.Height = backbuffer_height; // �o�b�N�o�b�t�@����
			resource_desc.DepthOrArraySize = 1;
			resource_desc.MipLevels = 0;
			resource_desc.SampleDesc.Count = 1;
			resource_desc.SampleDesc.Quality = 0;
			resource_desc.Format = depth_stencil_format_;	// DS�t�H�[�}�b�g
			resource_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;	// DS�p�̐ݒ�

			// �q�[�v�ݒ�
			D3D12_HEAP_PROPERTIES heap_prop{};
			heap_prop.Type = D3D12_HEAP_TYPE_DEFAULT;
			heap_prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heap_prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heap_prop.CreationNodeMask = 1;
			heap_prop.VisibleNodeMask = 1;

			// �f�v�X�o�b�t�@���N���A����Ƃ��̐ݒ�
			D3D12_CLEAR_VALUE clear_value{};
			clear_value.Format = depth_stencil_format_;	// �f�v�X�X�e���V���o�b�t�@�t�H�[�}�b�g
			clear_value.DepthStencil.Depth = 1.0f;	// �f�v�X�͈�ԉ��̒l
			clear_value.DepthStencil.Stencil = 0;	// �X�e���V���͂Ȃ��̂łO

			// ���\�[�X�쐬
			if (FAILED(device_->CreateCommittedResource(
				&heap_prop,
				D3D12_HEAP_FLAG_NONE,
				&resource_desc,
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&clear_value,
				IID_PPV_ARGS(&depth_stencil_buffer_))))
			{
				return false;
			}
			depth_stencil_buffer_->SetName(L"DeviceContext::depth_stencil_buffer");
		}

		// �f�X�N���v�^�q�[�v�E�r���[�쐬
		{
			D3D12_DESCRIPTOR_HEAP_DESC descriptor_desc{};
			descriptor_desc.NumDescriptors = 1;	//	���̃v���O�����ł̓f�v�X�X�e���V����1��
			descriptor_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;	//	�f�v�X�X�e���V���p�̒l


			if (FAILED(device->CreateDescriptorHeap(
				&descriptor_desc,
				IID_PPV_ARGS(&dsv_descriptor_heap_))))
			{
				return false;
			}
			dsv_descriptor_heap_->SetName(L"DeviceContext::dsv_descriptor_heap");
			dsv_descriptor_size_ = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

			// �r���[�쐬
			D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
			dsv_desc.Format = depth_stencil_format_; // �f�v�X�o�b�t�@�̃t�H�[�}�b�g
			dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsv_desc.Flags = D3D12_DSV_FLAG_NONE;

			auto dsv = dsv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart();
			// �r���[�쐬�C1�����Ȃ��̂Ő擪�ɏ�������
			device->CreateDepthStencilView(
				depth_stencil_buffer_.Get(),
				&dsv_desc,
				dsv);
		}

		return true;
	}

	/// @brief �R�}���h�A���P�[�^�쐬
	/// @param device ID3D12Device*
	/// @retval true ����
	/// @retval false ���s
	bool DeviceContext::CreateCommandAllocator(ID3D12Device* device)
	{
		for (int i = 0; i < backbuffer_size_; i++)
		{
			if (FAILED(device->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(command_allocators_[i].ReleaseAndGetAddressOf()))))
			{
				return false;
			}
			std::wstring name{ L"DeviceContext::command_allocators[" + std::to_wstring(i) + L"]" };
			command_allocators_[i]->SetName(name.c_str());
		}

		return true;
	}

	/// @brief �O���t�B�b�N�X�R�}���h�쐬
	/// @param device ID3D12Device*
	/// @param allocator ID3D12CommandAllocator*
	/// @retval true ����
	/// @retval false ���s
	bool DeviceContext::CreateCommandList(ID3D12Device* device, ID3D12CommandAllocator* allocator)
	{
		if (FAILED(device->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			allocator,
			nullptr,
			IID_PPV_ARGS(graphics_commnadlist_.ReleaseAndGetAddressOf()))))
		{
			return false;
		}
		graphics_commnadlist_->SetName(L"DeviceContext::graphics_commandlist");

		graphics_commnadlist_->Close();

		return true;
	}











#pragma endregion

}
//Windows.h����Q�[���ł͎g��Ȃ��w�b�_��r������}�N��
#define	WIN32_LEAN_AND_MEAN
#define	NOMINMAX
#define	NODRAWTEXT
#define	NOGDI
#define	NOBITMAP
#define	NOMCX
#define	NOSERVICE
#define	NOHELP

//Windows API���g�����߂̃w�b�_�t�@�C��
#include<Windows.h>
#include<wrl.h>

//DEBUG�r���h���̃��������[�N�`�F�b�N
#if _DEBUG
#define _CRTDBG_MAP_ALLOC
#include<crtdbg.h>
#endif

/*----------------------------------------------*/
/*  C++�@�W�����C�u����                               */
/*----------------------------------------------*/
#include<string>

/*----------------------------------------------*/
/*  DirectX�֘A�w�b�_                                     */
/*----------------------------------------------*/
#include<d3d12.h>
#include<dxgi1_6.h>

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

/*----------------------------------------------*/
/*  DirectX�֘A�̃��C�u����                          */
/*----------------------------------------------*/
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"dxguid.lib")

//�������O���
namespace {

	//�E�B���h�E�N���X�l�[���̕����z��
	constexpr wchar_t kWindowClassName[]{ L"DirecrtX12WindowClass" };

	//�^�C�g���o�[�̕\������镶����
	constexpr wchar_t kWindowTitle[]{ L"DirectX12 Application" };

	//�f�t�H���g�̃N���C�A���g�̈�
	constexpr int kClienWidth = 800;
	constexpr int kClienHeight = 600;

	//ComPtr��namespace�������̂ŃG�C���A�X(�ʖ�)��ݒ�
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	/*---------------------------------------------------------------------*/
	/*  D�RD�I�u�W�F�N�g�֘A                                                                 */
	/*---------------------------------------------------------------------*/
	/*D3D12�f�o�C�X                                                                               */
	ComPtr<ID3D12Device> d3d12_device = nullptr;
	//WARP�A�_�v�^�d�l�t���O(�f�o�b�O�r���h�̂ݗL��)
	constexpr bool kUseWarpAdapter = true;
	//�o�b�N�o�b�t�@�̍ő�l
	constexpr int kMaxBackBufferSize = 3;


	//�R�}���h���X�g,GPU�ɑ΂��ď����菇�����X�g�Ƃ��Ď���
	ComPtr<ID3D12GraphicsCommandList> graphics_commandlist = nullptr;
	//�R�}���h�A���P�[�^, �R�}���h���쐬���邽�߂̃��������m�ۂ���I�u�W�F�N�g
	ComPtr<ID3D12CommandAllocator> command_allocators[kMaxBackBufferSize];
	//�R�}���h�L���[�C1�ȏ�̃R�}���h���X�g���L���[�ɐς��GPU�ɑ���
	ComPtr<ID3D12CommandQueue> command_queue = nullptr;


	//�����_�[�^�[�Q�b�g�̐ݒ�(Descriptor)��VRAM�ɕۑ����邽�ߕK�v
	ComPtr<ID3D12DescriptorHeap> rtv_descriptor_heap = nullptr;
	//��̃����_�[�^�[�Q�b�g�f�X�N���v�^��1������̃T�C�Y���o����
	UINT rtv_descriptor_size = 0;

	//GPU�������I����Ă��邩���m�F���邽�߂ɕK�v�ȃt�F���X�I�u�W�F�N�g
	ComPtr<ID3D12Fence> d3d12_fence = nullptr;
	//�t�F���X�ɏ������ޒl,�o�b�N�o�b�t�@�Ɠ�������p�ӂ��Ă���
	UINT64 fence_values[kMaxBackBufferSize]{};
	//CPU. GPU�̓����������y�ɂƂ邽�߂Ɏg��
	Microsoft::WRL::Wrappers::Event fence_event;

	/*---------------------------------------------------------------------*/
	/*  �o�b�N�o�b�t�@�֘A                                                                      */
	/*---------------------------------------------------------------------*/
	//�X���b�v�`�F�C���I�u�W�F�N�g. �`�挋�ʂ���ʂ֏o�͂��Ă����
	ComPtr<IDXGISwapChain4> swap_chain = nullptr;
	//�o�b�N�o�b�t�@�Ƃ��Ďg�������_�[�^�[�Q�b�g�z��
	ComPtr<ID3D12Resource> render_targets[kMaxBackBufferSize]{};
	//�o�b�N�o�b�t�@�e�N�X�`���̃t�H�[�}�b�g. 1px��RGBA�e8bit����32bit
	DXGI_FORMAT backbuffer_format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//�쐬����o�b�N�o�b�t�@��
	int backbuffer_size = 2;
	//���݂̃o�b�N�o�b�t�@�̃C���f�b�N�X
	int backbuffer_index = 0;

	/*-------------------------------------------*/
	/*  �v���g�^�C�v�錾                                   */
	/*-------------------------------------------*/
	LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	bool InitializeD3D(HWND hwnd);
	void FinalizeD3D();
	void WaitForGpu();
	void Render();
	void WaitForPreviousFrame();

	/*-------------------------------------------*/
	/*  �֐���`                                                 */
	/*-------------------------------------------*/
	/// @brief  �E�B���h�E�v���V�[�W��
	/// @param hwnd  �E�B���h�E�n���h��
	/// @param message ���b�Z�[�W�̎��
	/// @param wParam ���b�Z�[�W�̃p�����[�^
	/// @param lParam �@���b�Z�[�W�̃p�����[�^
	/// @return ���b�Z�[�W�̖߂�l
	LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_CLOSE:	//�v���O�����I���V�O�i��
			DestroyWindow(hwnd);
			break;

		case WM_DESTROY:	//�E�B���h�E�j���V�O�i��
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
			break;
		}
		return 0;
	}

	/// @brief D3D�I�u�W�F�N�g�̏�����
	/// @param hwnd	 �A�v���P�[�V�����̃E�B���h�E�n���h��
	/// @return  ���� true / ���s false
	bool InitializeD3D(HWND hwnd)
	{
		HRESULT hr = S_OK;

		//�t�@�N�g���쐬�t���O
		UINT dxgi_factory_flags = 0;

		//_DEBUG�r���h����D3D12�̃f�o�b�O�@�\�̗L��������
#if _DEBUG
		ComPtr<ID3D12Debug> debug;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
		{
			debug->EnableDebugLayer();
		}

		//DXGI�̃f�o�b�O�@�\�ݒ�
		ComPtr<IDXGIInfoQueue> info_queue;
		if (SUCCEEDED(DXGIGetDebugInterface1(
			0, IID_PPV_ARGS(info_queue.GetAddressOf()))))
		{
			dxgi_factory_flags = DXGI_CREATE_FACTORY_DEBUG;

			//DXGI�ɖ�肪���������Ƀv���O�������u���[�N����悤�ɂ���
			info_queue->SetBreakOnSeverity(
				DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
			info_queue->SetBreakOnSeverity(
				DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
		}
#endif

		//�A�_�v�^���擾���邽�߂̃t�@�N�g���쐬
		ComPtr<IDXGIFactory5> dxgi_factory;
		hr = CreateDXGIFactory2(dxgi_factory_flags,
			IID_PPV_ARGS(&dxgi_factory));

		if (FAILED(hr))
		{
			return 1;
		}

		//D3D12���g����A�_�v�^�̒T������
		ComPtr<IDXGIAdapter1> adapter;	//�A�_�v�^(�r�f�I�J�[�h)�I�u�W�F�N�g
		{
			UINT adapter_index = 0;

			//dxgi_factory->EnumAdapters1�ɂ��A�_�v�^�̐��\���`�F�b�N���Ă���
			while (DXGI_ERROR_NOT_FOUND
				!= dxgi_factory->EnumAdapters1(adapter_index, &adapter))
			{
				DXGI_ADAPTER_DESC1 desc{};
				adapter->GetDesc1(&desc);
				++adapter_index;

				if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				{
					continue;
				}

				//D3D12�͎g�p�\��
				hr = D3D12CreateDevice(adapter.Get(),
					D3D_FEATURE_LEVEL_11_0,
					__uuidof(ID3D12Device), nullptr);

				if (SUCCEEDED(hr))
				{
					break;	//�g����A�_�v�^���������ꍇ�T���𒆒f���Ď��̏�����
				}
			}
		}
#if _DEBUG
		//�K�؂ȃn�[�h�E�F�A�A�_�v�^���Ȃ��āAkUseWarpAdapter��true�Ȃ�
		//WARP�A�_�v�^���擾(WARP = Windows Advanced Rasterization + Platform)
		if (adapter == nullptr && kUseWarpAdapter)
		{
			//WARP��GPU�@�\�̈ꕔ��CPU���Ŏ��s���Ă����A�_�v�^
			//�����ȃ\�t�g�E�F�A��萏���������{�Ԃ̃Q�[���Ŏg���͖̂���
			hr = dxgi_factory->EnumWarpAdapter(
				IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf()));
			if (FAILED(hr))
			{
				return false;
			}
		}
#endif

		if (!adapter)
		{
			return false;
		}

		//�Q�[���Ŏg���f�o�C�X�쐬
		hr = D3D12CreateDevice(adapter.Get(),	//�f�o�C�X�����Ɏg���A�_�v�^
			D3D_FEATURE_LEVEL_11_0,					//D3D�̋@�\���x��
			IID_PPV_ARGS(&d3d12_device));			//�󂯎��ϐ�

		if (FAILED(hr))
		{
			return false;
		}
		d3d12_device->SetName(L"d3d12_device");

		//�R�}���h�L���[�̍쐬
		{
			//�L���[�̐ݒ�p�\����
			D3D12_COMMAND_QUEUE_DESC desc{};
			desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

			//�L���[�̍쐬
			hr = d3d12_device->CreateCommandQueue(
				&desc,
				IID_PPV_ARGS(command_queue.ReleaseAndGetAddressOf()));

			if (FAILED(hr))
			{
				return false;
			}
			command_queue->SetName(L"command_queue");
		}

		//�X���b�v�`�F�C���쐬
		{
			//�X���b�v�`�F�C���̐ݒ�p�\����
			DXGI_SWAP_CHAIN_DESC1 desc{};
			desc.BufferCount = backbuffer_size;
			desc.Width = kClienWidth;
			desc.Height = kClienHeight;
			desc.Format = backbuffer_format;
			desc.BufferUsage =
				DXGI_USAGE_RENDER_TARGET_OUTPUT;
			desc.Scaling = DXGI_SCALING_STRETCH;
			desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
			desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
			desc.Flags = 0;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;

			//�X���b�v�`�F�C���I�u�W�F�N�g�쐬
			ComPtr<IDXGISwapChain1> sc;
			hr = dxgi_factory->CreateSwapChainForHwnd(
				command_queue.Get(),
				hwnd,
				&desc,
				nullptr,
				nullptr,
				sc.GetAddressOf());
			if (FAILED(hr))
			{
				return false;
			}

			//IDXGISwapChain1::As�֐�����IDXGISwapChain4�C���^�[�t�F�[�X���擾

			hr = sc.As(&swap_chain);
			if (FAILED(hr))
			{
				return false;
			}
		}

		//�����_�[�^�[�Q�b�g�p�̃f�X�N���v�^�q�[�v�쐬
		{
			//�����_�[�^�[�Q�b�g�p�f�X�N���v�^�q�[�v�̐ݒ�\����
			D3D12_DESCRIPTOR_HEAP_DESC desc{};
			desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			desc.NumDescriptors = backbuffer_size;
			desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

			//desc�̓��e�Ń������𓮓I�Ɋm�ۂɊm�ۂ��Ă��炤
			hr = d3d12_device->CreateDescriptorHeap(
				&desc,
				IID_PPV_ARGS(rtv_descriptor_heap.ReleaseAndGetAddressOf()));
			if (FAILED(hr))
			{
				return false;
			}
			rtv_descriptor_heap->SetName(L"rtv_descriptor_heap");

			//�f�X�N���v�^�q�[�v�̃������T�C�Y���擾
			rtv_descriptor_size = d3d12_device->GetDescriptorHandleIncrementSize(
				D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		}

		//�����_�[�^�[�Q�b�g�r���[���쐬
		D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle =
			rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart();

		//�o�b�N�o�b�t�@�̐����������_�[�^�[�Q�b�g���쐬
		for (int i = 0; i < backbuffer_size; i++)
		{
			//�X���b�v�`�F�C������o�b�N�o�b�t�@���擾���Ă���
			hr = swap_chain->GetBuffer(i, IID_PPV_ARGS(render_targets[i].GetAddressOf()));
			if (FAILED(hr))
			{
				return false;
			}
			//render_targets�ɖ��O��ݒ肷�邽�߂̕�����ϐ�
			std::wstring name{ L"render_targets[" + std::to_wstring(i) + L"]" };
			render_targets[i]->SetName(name.c_str());

			//�����_�[�^�[�Q�b�g�r���[�̐ݒ�p�\����
			D3D12_RENDER_TARGET_VIEW_DESC desc = {};
			desc.Format = backbuffer_format;
			desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

			//RT�ɂ��Ă̏������������RT�r���[�쐬
			d3d12_device->CreateRenderTargetView(
				render_targets[i].Get(),
				&desc,
				rtv_handle);
			rtv_descriptor_heap->SetName(L"rtv_descriptor_heap");

			//���̃��[�v�ɔ����ď������ݐ�̃������ʒu���炸�炷
			rtv_handle.ptr = rtv_descriptor_size;
		}
		//����`��ɔ����āA���݂̃o�b�N�o�b�t�@�̃C���f�X�N�擾
		backbuffer_index = swap_chain->GetCurrentBackBufferIndex();

		//�R�}���h�A���P�[�^�쐬
		{
			//�g�p����o�b�N�o�b�t�@�Ɠ����������쐬���Ă݂�
			for (int i = 0; i < backbuffer_size; i++)
			{
				hr = d3d12_device->CreateCommandAllocator(
					D3D12_COMMAND_LIST_TYPE_DIRECT,
					IID_PPV_ARGS(command_allocators[i].ReleaseAndGetAddressOf()));
				if (FAILED(hr))
				{
					return false;
				}

				std::wstring name{ L"command_allocators[" + std::to_wstring(i) + L"]" };
				command_allocators[i]->SetName(name.c_str());
			}
		}
		//�R�}���h���X�g�쐬
		{
			hr = d3d12_device->CreateCommandList(
				0, D3D12_COMMAND_LIST_TYPE_DIRECT,
				command_allocators[backbuffer_index].Get(),
				nullptr,
				IID_PPV_ARGS(graphics_commandlist.ReleaseAndGetAddressOf()));
			if (FAILED(hr))
			{
				return false;
			}
			graphics_commandlist->SetName(L"graphics_commandlist");

			graphics_commandlist->Close();
		}

		//�t�F���X�I�u�W�F�N�g�쐬
		for (int i = 0; i < backbuffer_size; i++)
		{
			fence_values[i] = 0;	//�t�F���X�l���O�N���A
		}

		//�t�F���X�쐬�@
		hr = d3d12_device->CreateFence(
			fence_values[backbuffer_index],
			D3D12_FENCE_FLAG_NONE,
			IID_PPV_ARGS(d3d12_fence.ReleaseAndGetAddressOf()));
		if (FAILED(hr))
		{
			return false;
		}
		d3d12_fence->SetName(L"d3d12_fence");

		//���񓯊��̂��߂̃t�F���X�l��ݒ�
		++fence_values[backbuffer_index];

		//�t�F���X�̏�Ԃ��m�F����C���x���g�����
		fence_event.Attach(
			//CreateEventEx�֐�����Ɩ߂�l�ŃC�x���g�̃n���h�����Ⴆ��
			CreateEventEx(
				nullptr,
				nullptr,
				0,
				EVENT_MODIFY_STATE | SYNCHRONIZE)
		);

		//�C�x���g���������ݒ�ł������Ƃ��`�F�b�N
		if (!fence_event.IsValid())
		{
			return false;
		}

		return true;
	}


	/// @brief  D3D�I�u�W�F�N�g�������
	void FinalizeD3D()
	{
		WaitForGpu();
	}
	/// @brief GPU�̏����������m�F����
	void WaitForGpu()
	{
		//���݂̃t�F���X�l�����[�J���ɃR�s�[
		const UINT64 current_value = fence_values[backbuffer_index];

		//�L���[�������ɍX�V�����t�F���X�ƃt�F���X�l���Z�b�g
		if (FAILED(command_queue->Signal(d3d12_fence.Get(), current_value)))
		{
			return;
		}
		//�C�x���g���ł���܂ł��̃v���O������ҋ@��Ԃɂ���
		WaitForSingleObjectEx(fence_event.Get(), INFINITE, FALSE);

		//����̃t�F���X�p�Ƀt�F���X�l�X�V
		fence_values[backbuffer_index] = current_value + 1;
	}

	/// @brief �`�揈��
	void Render()
	{
		//�A���P�[�^�ƃR�}���h���X�g�����Z�b�g
		//�O�t���[���Ŏg�����f�[�^��Y��āA���������ė��p�\�ɂ���
		command_allocators[backbuffer_index]->Reset();
		graphics_commandlist->Reset(
			command_allocators[backbuffer_index].Get(),
			nullptr);

		//�����_�[�^�[�Q�b�g�ւ̃��\�[�X�o���A
		{
			//�o���A�p�f�[�^�쐬
			D3D12_RESOURCE_BARRIER barrier{};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.Subresource =
				D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			//��Ԃ��`�F�b�N���郊�\�[�X
			barrier.Transition.pResource
				= render_targets[backbuffer_index].Get();

			//�@�ۑO(Before)�̏�ԁ@
			barrier.Transition.StateBefore
				= D3D12_RESOURCE_STATE_PRESENT;

			//�@�ی�(After)�̏��
			barrier.Transition.StateAfter
				= D3D12_RESOURCE_STATE_RENDER_TARGET;

			//���\�[�X�o���A�R�}���h�쐬
			graphics_commandlist->ResourceBarrier(1, &barrier);
		}

		//�����_�[�^�[�Q�b�g�̃f�B�X�N���v�^�n���h�����擾
		D3D12_CPU_DESCRIPTOR_HANDLE rtv
			= rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
		rtv.ptr += static_cast<SIZE_T> (static_cast<INT64>(backbuffer_index) *
			static_cast<INT64>
			(rtv_descriptor_size));

		//�`��Ώۂ̃����_�[�^�[�Q�b�g��ݒ肷��R�}���h
		graphics_commandlist->OMSetRenderTargets(
			1,
			&rtv,
			FALSE,
			nullptr);

		//�h��Ԃ��F. float4�v�f��RGBA���w��
		float clear_color[4]{ 0,0.5f,1.0f,1.0f };

		//�w�肵�������_�[�^�[�Q�b�g���A����̐F�œh��Ԃ�
		graphics_commandlist->ClearRenderTargetView(
			rtv,
			clear_color,
			0,
			nullptr);

		/*-----------------------------------------*/
		/*  �Q�[���̕`�揈���J�n                        */
		/*-----------------------------------------*/


		/*-----------------------------------------*/
		/*  �Q�[���̕`�揈���I��                        */
		/*-----------------------------------------*/

		//�����_�[�^�[�Q�b�g���o�b�N�o�b�t�@�Ƃ��Ďg����悤�ɂ����ԑJ��
		{
			D3D12_RESOURCE_BARRIER barrier{};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.Subresource =
				D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrier.Transition.pResource
				= render_targets[backbuffer_index].Get();
			barrier.Transition.StateBefore
				= D3D12_RESOURCE_STATE_RENDER_TARGET;

			barrier.Transition.StateAfter
				= D3D12_RESOURCE_STATE_PRESENT;

			graphics_commandlist->ResourceBarrier(1, &barrier);
		}

		//�R�}���h�L�^����
		graphics_commandlist->Close();

		//�L���[��ID3D12CommandList * �z����󂯎�� , ����͗v�f��1�̔z��Ƃ��ēn��
		ID3D12CommandList* command_lists[]{ graphics_commandlist.Get() };

		//�L���[�ɂ���R�}���h���s����
		command_queue->ExecuteCommandLists(
			_countof(command_lists),
			command_lists);
	
	//�o�b�N�o�b�t�@���t�����g�o�b�t�@�ɓ���ւ��w��
		swap_chain->Present(1, 0);
		
		WaitForPreviousFrame();
	}

	/// @brief �`�搂����������
	void WaitForPreviousFrame()
	{
		//�L���[�ɃV�O�i���𑗂�
		const UINT64 current_value = fence_values[backbuffer_index];
		command_queue->Signal(d3d12_fence.Get(), current_value);

		//���t���[���̃o�b�N�o�b�t�@�C���f�b�N�X�����炤
		backbuffer_index = swap_chain->GetCurrentBackBufferIndex();

		//GetCompletedValue�Ńt�F���X�̌��ݒn���m�F����.
		if (d3d12_fence->GetCompletedValue() < current_value)
		{
			//�`�悪�I����ĂȂ��̂œ���
			d3d12_fence->SetEventOnCompletion(current_value, fence_event.Get());
			WaitForSingleObjectEx(fence_event.Get(), INFINITE, FALSE);
		}

		//���̃t���[���̂��߂Ƀt�F���X�l�X�V
		fence_values[backbuffer_index] = current_value + 1;
	
	}
}// namespace

/// @brief Windows �A�v���̃G���g���[�|�C���g
/// @param hInstance �C���X�^���X�n���h��,OS���猩���A�v���̊Ǘ��ԍ��݂����Ȃ���
/// @param hPrevInstance	 �s�g�p(�ߋ���API�Ƃ̐������ێ��p)
/// @param lpCmdLine �R�}���h���C���������n�����
/// @param nCmdShow �E�B���h�E�\���ݒ肪�n�����
/// @return �I���R�[�h,�ʏ��0
int WINAPI wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nCmdShow)
{

#if _DEBUG
	//���������[�N�̌��o
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	//COM�I�u�W�F�N�g���g���Ƃ��ɂ͌Ă�ł����K�v������
	if (FAILED(CoInitializeEx(NULL, COINIT_MULTITHREADED)))
	{
		return 1;
	}

	//�E�B���h�E�N���X
	WNDCLASSEX wc{};
	wc.cbSize = sizeof(wc);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIconW(hInstance, L"IDI_ICON");
	wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = kWindowClassName;

	//�E�B���h�E�N���X�o�^
	if (!RegisterClassExW(&wc))
	{
		return 1;
	}

	//�E�B���h�E�̕\���X�^�C���t���O,OVERLAPPEWINDOW�͈�ʓI�ȃX�^�C���ɂȂ�
	DWORD window_style = WS_OVERLAPPEDWINDOW;

	//�`��̈�Ƃ��ẴT�C�Y���w��
	RECT window_rect{
		0,
		0,
		kClienWidth,
		kClienHeight };

	//�E�B���h�E�X�^�C�����܂߂��E�B���h�E�T�C�Y���v�Z����
	AdjustWindowRect(&window_rect, window_style, FALSE);

	//�E�B���h�E�̎��T�C�Y���v�Z
	auto window_width = window_rect.right - window_rect.left;
	auto window_height = window_rect.bottom - window_rect.top;

	//�E�B���h�E�쐬
	HWND hwnd = CreateWindowExW(
		0, kWindowClassName,
		kWindowTitle,
		window_style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		window_width,
		window_height,
		nullptr,
		nullptr,
		hInstance,
		nullptr);

	if (!hwnd)
	{
		return 1;
	}

	//�E�B���h�E��\��
	ShowWindow(hwnd, nCmdShow);

	if (InitializeD3D(hwnd) == false)
	{
		return 1;
	}

	//���C�����[�v
	//WM_QUIT��������v���O�����̏I��
	for (MSG msg{}; WM_QUIT != msg.message;)
	{
		//MSG�\���̂��g���ă��b�Z�[�W���󂯎��
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			//���b�Z�[�W������Ή��̂Q�s�Ń��b�Z�[�W�v���V�[�W��
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			//�����ɃQ�[���̏���������
			Render();
		}
	}

	FinalizeD3D();
	CoUninitialize();

	return 0;
}
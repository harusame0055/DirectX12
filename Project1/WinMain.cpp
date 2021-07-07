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
	ComPtr<ID3D12DescriptorHeap> rtv_descriptr_heap = nullptr;
	//��̃����_�[�^�[�Q�b�g�f�X�N���v�^��1������̃T�C�Y���o����
	UINT rtv_descriptr_size = 0;

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

		return true;
	}

	/// @brief  D3D�I�u�W�F�N�g�������
	void FinalizeD3D()
	{

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
		}
	}

	FinalizeD3D();

	return 0;
}
#pragma once
#include<d3d12.h>
#include<dxgi1_6.h>
#include<dxcapi.h>
#include<wrl.h>

namespace ncc {

	/// @brief D3D12�f�o�C�X����эŒ���̕`��ɕK�v�Ȃ��̂��܂Ƃ߂��I�u�W�F�N�g
	class DeviceContext {
	public:
		/// @brief ������
		/// @param hwnd HWND
		/// @param backbuffer_width �o�b�N�o�b�t�@��
		/// @param backbuffer_height �o�b�N�o�b�t�@����
		/// @retval true ����
		/// @retval false ���s
		bool Initialize(HWND hwnd, int backbuffer_width, int backbuffer_height);

		/// @brief �I������
		void Finalize();

		/// @brief GPU�Ƃ̓���
		void WaitForGpu();

		/// @brief GPU�̕`�揈���Ƃ̓���
		void WaitForPreviousFrame();

		/// @brief �X���b�v�`�F�[���v���[���g
		void Present();

		/// @brief �f�o�C�X�擾
		/// @return ID3D12Device*
		ID3D12Device* device();

		/// @brief �X���b�v�`�F�[���擾
		/// @return IDXGISwapChain4*
		IDXGISwapChain4* swap_chain();

		/// @brief �O���t�B�b�N�X�R�}���h���X�g�擾
		/// @return ID3D12GraphicsCommandList*
		ID3D12GraphicsCommandList* command_list();

		/// @brief �J�����g�t���[���Ŏg���ׂ��A���P�[�^�擾
		/// @return ID3D12CommandAllocator*
		ID3D12CommandAllocator* current_command_allocator();

		/// @brief �C���f�b�N�X�Ŏw�肵���A���P�[�^�擾
		/// @param index 0 ~ backbuffer -1 �̊Ԃ̐��l
		/// @return ID3D12CommandAllocator*
		ID3D12CommandAllocator* command_allocator(int index);

		/// @brief �R�}���h�L���[
		/// @return ID3D12CommandQueue*
		ID3D12CommandQueue* command_queue();

		/// @brief �J�����g�t���[���Ŏg���郌���_�[�^�[�Q�b�g
		/// @return ID3D12Resource*
		ID3D12Resource* current_render_target();

		/// @brief �J�����g�t���[���Ŏg���郌���_�[�^�[�Q�b�g�r���[
		/// @return D3D12_CPU_DESCRIPTOR_HANDLE
		const D3D12_CPU_DESCRIPTOR_HANDLE current_rtv() const;

		/// @brief �f�v�X�X�e���V���r���[�擾
		/// @return D3D12_CPU_DESCRIPTOR_HANDLE
		const D3D12_CPU_DESCRIPTOR_HANDLE dsv() const;

		/// @brief �J�����g�t���[���ł̃o�b�N�o�b�t�@�C���f�b�N�X
		/// @return �o�b�N�o�b�t�@�C���f�b�N�X
		const int backbuffer_index() const;

		/// @brief �쐬���ꂽ�o�b�N�o�b�t�@���擾
		/// @return int
		const int backbuffer_size() const;

		/// @brief �o�b�N�o�b�t�@�̃r���[�|�[�g�擾
		/// @return �����̃r���[�|�[�g�̎Q��
		const D3D12_VIEWPORT& screen_viewport() const;

		/// @brief �V�U�[��`�擾
		/// @return �����̃V�U�[�̎Q��
		const D3D12_RECT& scissor_rect() const;

	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		bool CreateCommandAllocator(ID3D12Device* device);
		bool CreateCommandList(ID3D12Device* device, ID3D12CommandAllocator* allocator);
		bool CreateCommandQueue(ID3D12Device* device);
		bool CreateDepthStencil(ID3D12Device* device, int backbuffer_width, int backbuffer_height);
		bool CreateDevice(IDXGIFactory5* factory);
		bool CreateFactory();
		bool CreateFence(ID3D12Device* device);
		bool CreateRenderTarget(ID3D12Device* device, IDXGISwapChain4* swap_chain);
		bool CreateSwapChain(HWND hwnd, IDXGIFactory5* factory, int backbuffer_width, int backbuffer_height);

		// WARP�A�_�v�^�g�p�t���O(�f�o�b�O�r���h�̂ݗL��)
		static constexpr bool useWarpAdapter = true;

		// �o�b�N�o�b�t�@�ő�l
		static constexpr int maxBackBufferSize = 3;

		// �f�o�C�X
		ComPtr<ID3D12Device> device_ = nullptr;

		// �A�_�v�^���擾���邽�߂̃t�@�N�g���쐬
		ComPtr<IDXGIFactory5> dxgi_factory_;

		// �X���b�v�`�F�[��
		ComPtr<IDXGISwapChain4> swap_chain_ = nullptr;

		// �o�b�N�o�b�t�@�t�H�[�}�b�g
		const DXGI_FORMAT backbuffer_format_ = DXGI_FORMAT_R8G8B8A8_UNORM;
		// �쐬���鐔
		int backbuffer_size_ = 2;
		// ���݂̃o�b�N�o�b�t�@�C���f�b�N�X
		int backbuffer_index_ = 0;

		// RT�f�X�N���v�^�q�[�v
		ComPtr<ID3D12DescriptorHeap> rtv_descriptor_heap_ = nullptr;
		// RT�f�X�N���v�^��1������̃T�C�Y
		UINT rtv_descriptor_size_ = 0;
		// �o�b�N�o�b�t�@�Ƃ��Ďg�������_�[�^�[�Q�b�g�z��
		ComPtr<ID3D12CommandQueue> command_queue_ = nullptr;

		// �R�}���h���X�g�CGPU�ɑ΂��ď����菇�����X�g�Ƃ��Ď���
		ComPtr<ID3D12GraphicsCommandList> graphics_commnadlist_ = nullptr;
		// �R�}���h�A���P�[�^�C�R�}���h���쐬���邽�߂̃��������m�ۂ���I�u�W�F�N�g
		ComPtr<ID3D12CommandAllocator> command_allocators_[maxBackBufferSize];
		// �R�}���h�L���[�C1�ȏ�̃R�}���h���X�g���L���[�ɐς��GPU�ɑ���
		ComPtr<ID3D12CommandQueue> command_queue_ = nullptr;

		// �f�v�X�X�e���V���t�H�[�}�b�g
		DXGI_FORMAT depth_stencil_format_ = DXGI_FORMAT_D24_UNORM_S8_UINT;
		// �f�v�X�X�e���V���f�X�N���v�^�q�[�v
		ComPtr<ID3D12DescriptorHeap> dsv_descriptor_heap_ = nullptr;
		// �f�v�X�X�e���V���f�X�N���v�^�q�[�v�T�C�Y
		UINT dsv_descriptor_size_ = 0;
		// �f�v�X�X�e���V���o�b�t�@
		ComPtr<ID3D12Resource> depth_stencil_buffer_ = nullptr;

		// �t�F���X�I�u�W�F�N�g
		ComPtr<ID3D12Fence> d3d12_fence_ = nullptr;
		// �t�F���X�ɏ������ޒl.
		UINT fence_values_[maxBackBufferSize]{};
		// CPU. GPU�̓����������y�ɂƂ邽�߂Ɏg��
		Microsoft::WRL::Wrappers::Event fence_event_;

		// �r���[�|�[�g
		D3D12_VIEWPORT screen_viewport_{};
		// �V�U�[
		D3D12_RECT scissor_rect_{};
	};

}	// namespace nss
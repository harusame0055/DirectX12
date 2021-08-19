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
		const UINT64 current_value = fence_values_[backbuffer_index_];

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

#pragma endregion

}
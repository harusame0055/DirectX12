#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <d3d12.h>
#include <wrl.h>

#include "descriptor_heap.h"

namespace ncc {

class TextureResource;

bool LoadTextureFromFile(Microsoft::WRL::ComPtr<ID3D12Device> device, TextureResource* texture);

/// @brief �e�N�X�`�����\�[�X���Ǘ�����N���X
class TextureResource {
public:
  /// @brief �R���X�g���N�^
  /// @param name �e�N�X�`����
  TextureResource(const std::wstring& name)
    : name_(name)
  {}

  /// @brief �f�X�g���N�^
  ~TextureResource() = default;

  /// @brief �e�N�X�`���f�[�^��GPU�ɖ��A�b�v���[�h�ς݂�
  /// @return �A�b�v���[�h�ς݂Ȃ�true, �����łȂ����false.
  bool IsUploaded()
  {
    return !is_upload_wait_;
  }

  /// @brief ���\�[�X�Ƃ��ėL����
  /// @return �L���Ȃ�true, �����łȂ����false.
  bool IsValid()
  {
    if (resource_ == nullptr ||
        is_upload_wait_ == true ||
        is_enabled_ == false)
    {
      return false;
    }
    return true;
  }

  /// @brief ���݂̃e�N�X�`����������ĐV�����e�N�X�`�����\�[�X������悤�ɂ���
  /// @param name �V�����e�N�X�`����
  void Reset(const std::wstring& name)
  {
    Relase();
    name_ = name;
  }

  /// @brief �I�u�W�F�N�g�̃f�[�^���������
  void Relase()
  {
    name_.clear();

    is_upload_wait_ = false;
    is_enabled_ = false;

    subresource_ = D3D12_SUBRESOURCE_DATA{};
    data_.reset();;

    resource_.Reset();
  }

  /// @brief GPU�������ɍ쐬���ꂽ���\�[�X��Ԃ�
  /// @return �e�N�X�`�����\�[�X
  Microsoft::WRL::ComPtr<ID3D12Resource> resource()
  {
    return resource_;
  }

  /// @brief �e�N�X�`�����擾
  /// @return �e�N�X�`����
  const std::wstring& name()
  {
    return name_;
  }

private:
  friend bool LoadTextureFromFile(Microsoft::WRL::ComPtr<ID3D12Device> device, TextureResource* texture);
  friend class TextureUploadCommandList;

  Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
  std::wstring name_;
  bool is_upload_wait_ = false;
  bool is_enabled_ = false;
  D3D12_SUBRESOURCE_DATA subresource_{};
  std::unique_ptr<std::uint8_t[]> data_;
};

/// @brief�V�X�e���������ɂ���e�N�X�`���f�[�^��GPU��������
/// �A�b�v���[�h���邽�߂̏������s���N���X
class TextureUploadCommandList
{
public:
  TextureUploadCommandList() = default;
  ~TextureUploadCommandList() = default;

  bool BeginRecording(Microsoft::WRL::ComPtr<ID3D12Device> device);
  bool EndRecording();
  bool Execute();
  bool AddList(TextureResource* texture);

private:
  Microsoft::WRL::ComPtr<ID3D12Device> device_;
  Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator_;
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list_;

  std::vector<TextureResource*> upload_list_;
  std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> buffers_;

  bool is_begin_recording_ = false;
};


/// @brief �e�N�X�`�����\�[�X�̃V�F�[�_���\�[�X�r���[����������N���X
class TextureView {
public:
  TextureView() = default;
  ~TextureView() = default;

  bool Create(Microsoft::WRL::ComPtr<ID3D12Device> device,
              DescriptorHeap* srv_heap,
              TextureResource* texture);
  void Release();

  Microsoft::WRL::ComPtr<ID3D12Resource> resource()
  {
    return texture_->resource();
  }

  D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle()
  {
    return handle_.gpu_handle();
  }

  D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle()
  {
    return handle_.cpu_handle();
  }

private:
  TextureResource* texture_ = nullptr;
  DescriptorHandle handle_;
};

} //namespace ncc

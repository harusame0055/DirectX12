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

/// @brief テクスチャリソースを管理するクラス
class TextureResource {
public:
  /// @brief コンストラクタ
  /// @param name テクスチャ名
  TextureResource(const std::wstring& name)
    : name_(name)
  {}

  /// @brief デストラクタ
  ~TextureResource() = default;

  /// @brief テクスチャデータがGPUに未アップロード済みか
  /// @return アップロード済みならtrue, そうでなければfalse.
  bool IsUploaded()
  {
    return !is_upload_wait_;
  }

  /// @brief リソースとして有効か
  /// @return 有効ならtrue, そうでなければfalse.
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

  /// @brief 現在のテクスチャを解放して新しいテクスチャリソースを作れるようにする
  /// @param name 新しいテクスチャ名
  void Reset(const std::wstring& name)
  {
    Relase();
    name_ = name;
  }

  /// @brief オブジェクトのデータを解放する
  void Relase()
  {
    name_.clear();

    is_upload_wait_ = false;
    is_enabled_ = false;

    subresource_ = D3D12_SUBRESOURCE_DATA{};
    data_.reset();;

    resource_.Reset();
  }

  /// @brief GPUメモリに作成されたリソースを返す
  /// @return テクスチャリソース
  Microsoft::WRL::ComPtr<ID3D12Resource> resource()
  {
    return resource_;
  }

  /// @brief テクスチャ名取得
  /// @return テクスチャ名
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

/// @briefシステムメモリにあるテクスチャデータをGPUメモリに
/// アップロードするための処理を行うクラス
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


/// @brief テクスチャリソースのシェーダリソースビューを処理するクラス
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

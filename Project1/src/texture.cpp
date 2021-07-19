#include "texture.h"

#include "WICTextureLoader12.h"

using namespace Microsoft::WRL;

namespace ncc {

/// @brief 画像ファイルをシステムメモリにロードして
///   GPUメモリにリソース領域を確保する.データ転送はしないので注意.
/// @param device ID3D12Deviceポインタ
/// @param texture TextureResourceの参照
/// @return 成功ならtrue,失敗ならfalse.
bool LoadTextureFromFile(ComPtr<ID3D12Device> device, TextureResource* texture)
{
  // 不正な引数なら失敗
  if (device == nullptr || texture->name_.empty())
  {
    return false;
  }

  texture->data_.reset();

  // 画像ファイルをシステムメモリにロードしてGPUメモリにリソース領域を確保
  // GPUメモリへの転送は自分でやる必要がある
  if (FAILED(DirectX::LoadWICTextureFromFile(
    device.Get(),
    texture->name_.c_str(),
    &texture->resource_,
    texture->data_,
    texture->subresource_)))
  {
    return false;
  }

  // 未転送状態
  texture->is_upload_wait_ = true;

  return true;
}

//-------------------------------------------------------------------
// TextureUploadCommandList
//-------------------------------------------------------------------

/// @brief テクスチャアップロードコマンド記録開始
/// @param device ID3D12Deviceポインタ
/// @return 成功ならtrue,失敗ならfalse.
bool TextureUploadCommandList::BeginRecording(ComPtr<ID3D12Device> device)
{
  // 無効なデバイスを渡されるか、すでに記録開始状態ならば失敗
  if (device == nullptr || is_begin_recording_)
  {
    return false;
  }
  device_ = device;

  // コマンドアロケータは転送毎に使い捨てで作る
  if (FAILED(device->CreateCommandAllocator(
    D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator_))))
  {
    return false;
  }
  allocator_->SetName(L"TextureUploadCommandList");

  // コマンドリストも転送毎に使い捨てで作る
  if (FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
      allocator_.Get(), nullptr, IID_PPV_ARGS(&command_list_))))
  {
    return false;
  }
  command_list_->SetName(L"TextureUploadCommandList");

  is_begin_recording_ = true;  // コマンド記録開始状態にする

  return true;
}

/// @brief コマンド記録終了
/// @return 成功ならtrue,失敗ならfalse.
bool TextureUploadCommandList::EndRecording()
{
  // 不正な状態で呼ばれたら失敗
  if (is_begin_recording_ == false)
  {
    return false;
  }

  // コマンドリストを閉じておく
  if (FAILED(command_list_->Close()))
  {
    return false; // Close処理は失敗の可能性もある
  }

  is_begin_recording_ = false;  // 記録終了

  return true;
}

/// @brief バッファデータをGPUメモリに転送
/// @return 成功ならtrue,失敗ならfalse.
bool TextureUploadCommandList::Execute()
{
  // 不正な状態なら失敗
  if (is_begin_recording_)
  {
    return false;
  }

  // テクスチャを1個も転送しないなら処理終了
  if (upload_list_.size() == 0)
  {
    return true;  // 1個も転送しないだけで失敗ではない
  }

  // アップロード実行用のキューとフェンスは使い捨てで作る
  ComPtr<ID3D12CommandQueue> queue;
  ComPtr<ID3D12Fence> fence;

  // フェンス作成
  if (FAILED(device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))))
  {
    return false;
  }
  fence->SetName(L"TextureUploadCommandList");

  // キュー作成
  {
    D3D12_COMMAND_QUEUE_DESC desc{};
    desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; // 一般的な描画用はこれ
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; // 基本的にNONEでOK

    if (FAILED(device_->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue))))
    {
      return false;
    }
    queue->SetName(L"TextureUploadCommandList");
  }

  // フェンスを監視するイベント
  Microsoft::WRL::Wrappers::Event event;
  event.Attach(CreateEventEx(
    nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
  if (!event.IsValid())
  {
    return false;
  }

  // コマンド実行
  const UINT64 complete_value = 1;
  ID3D12CommandList* commands[] = { command_list_.Get() };
  queue->ExecuteCommandLists(_countof(commands), commands);
  if (FAILED(queue->Signal(fence.Get(), complete_value)))
  {
    return false;
  }

  if (SUCCEEDED(fence->SetEventOnCompletion(complete_value, event.Get())))
  {
    WaitForSingleObjectEx(event.Get(), INFINITE, FALSE);
  }

  // リソースとして使えるようになったので有効状態に設定
  for (int i = 0; i < upload_list_.size(); i++)
  {
    upload_list_[i]->is_upload_wait_ = false;
    upload_list_[i]->is_enabled_ = true;
  }

  // 転送完了後の後片付け
  upload_list_.clear();
  buffers_.clear();
  command_list_.Reset();
  allocator_.Reset();

  return true;
}

/// @brief GPUメモリにアップロードしたいTextureResourceをコマンドリストに追加する
/// @param texture 対象のテクスチャ
/// @return 成功ならtrue,失敗ならfalse.
bool TextureUploadCommandList::AddList(TextureResource* texture)
{
  // 不正な状態なら失敗
  if (texture->is_upload_wait_ == false ||
      texture->is_enabled_ == true ||
      is_begin_recording_ == false)
  {
    return false;
  }

  // ステージングバッファ作成
  // テクスチャリソースはCPUから直接書き込めないステートで作成されている
  // そこで Map可能 (CPUアクセス可能) なリソース (ステージングバッファ) にテクスチャデータを書き込む,
  // そしてステージングバッファのデータ "GPU" を使ってテクスチャリソースにコピーする
  ComPtr<ID3D12Resource> staging_buffer;

  D3D12_PLACED_SUBRESOURCE_FOOTPRINT layouts[1]{};  // サブリソース毎のメモリのデータ形式
  UINT num_rows[1]{};         // テクスチャ内のサブリソース毎の行数を受け取る
  UINT64 row_size_bytes[1]{}; // サブリソース毎の1行当たりのバイトサイズ
  UINT64 total_bytes = 0;     // テクスチャ全体のバイトサイズ
  {
    D3D12_RESOURCE_DESC resource_desc = texture->resource_->GetDesc();

    // バッファにコピーするときに必要になる情報を取得
    device_->GetCopyableFootprints(
      &resource_desc,
      0,
      1,
      0,
      layouts,
      num_rows,
      row_size_bytes,
      &total_bytes);

    // ステージングバッファ用のヒープ設定
    D3D12_HEAP_PROPERTIES buffer_heap{};
    buffer_heap.Type = D3D12_HEAP_TYPE_UPLOAD;  // CPUから書き込み可能にする
    buffer_heap.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    buffer_heap.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    buffer_heap.CreationNodeMask = 1;
    buffer_heap.VisibleNodeMask = 1;

    // ステージングバッファのリソース設定
    D3D12_RESOURCE_DESC buffer_desc{};
    buffer_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    buffer_desc.Alignment = 0;
    buffer_desc.Width = total_bytes;  // バッファ全体のサイズ
    buffer_desc.Height = 1;
    buffer_desc.DepthOrArraySize = 1;
    buffer_desc.MipLevels = 1;
    buffer_desc.Format = DXGI_FORMAT_UNKNOWN;
    buffer_desc.SampleDesc.Count = 1;
    buffer_desc.SampleDesc.Quality = 0;
    buffer_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    buffer_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    // ステージングバッファ作成
    if (FAILED(device_->CreateCommittedResource(
      &buffer_heap,
      D3D12_HEAP_FLAG_NONE,
      &buffer_desc,
      D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&staging_buffer))))
    {
      return false;
    }
  }

  // ステージングバッファへテクスチャデータをコピー
  {
    // バッファをMapして書込みアドレスをもらうよ
    void* buffer_ptr = nullptr;
    if (FAILED(staging_buffer->Map(
      0,
      nullptr, // D3D12_RANGE. 実は全体をMapするときはnullptrでOK
      &buffer_ptr)))
    {
      return false;
    }

    // コピー元のアドレス等を準備
    const std::uint8_t* src_data = static_cast<const std::uint8_t*>(texture->subresource_.pData);
    LONG_PTR src_row_pitch = texture->subresource_.RowPitch;
    UINT copy_size = static_cast<UINT>(row_size_bytes[0]);

    // システムメモリにある画像からバッファへ、1行毎にメモリコピー
    for (UINT row = 0; row < num_rows[0]; ++row)
    {
      void* dest_ptr = static_cast<std::uint8_t*>(buffer_ptr) + (layouts[0].Footprint.RowPitch * row);
      const void* src_ptr = src_data + (src_row_pitch * row);

      memcpy(dest_ptr, src_ptr, copy_size);
    }
    staging_buffer->Unmap(0, nullptr);  // コピー終わり
  }

  // ステージングバッファからリソースへのコピーコマンド作成
  {
    D3D12_TEXTURE_COPY_LOCATION src{};
    D3D12_TEXTURE_COPY_LOCATION dst{};

    // コピー元 (テクスチャリソース) の設定
    src.pResource = staging_buffer.Get();                 // コピー元のリソース
    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;  // バッファ側はこれ
    src.PlacedFootprint = layouts[0];                     // コピーするときに使うレイアウト

    // コピー先 (ステージングバッファ) の設定
    dst.pResource = texture->resource_.Get();               // コピー先のリソース
    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX; // コピー先はこれ
    dst.SubresourceIndex = 0;                             // コピー先のサブリソース番号

    // コピーコマンド
    command_list_->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

    // データを転送が完了したらテクスチャリソースはテクスチャとして使えるステートに遷移
    // ステート遷移なのでリソースバリアコマンドも必要
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = texture->resource_.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    // テクスチャリソースはロード後はコピー宛先状態 (COPY_DEST)になってる
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    // バッファからコピーが終わったら後はピクセルシェーダからみえる状態へ遷移
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    // バリアコマンド
    command_list_->ResourceBarrier(1, &barrier);
  }

  // 転送成功時にテクスチャリソースを有効化させるため覚えておく
  upload_list_.emplace_back(texture);
  // staging_bufferの中身はまだ未転送なので、消えないように覚えておく
  buffers_.emplace_back(staging_buffer);

  return true;
}

//-------------------------------------------------------------------
// TextureView
//-------------------------------------------------------------------
bool TextureView::Create(Microsoft::WRL::ComPtr<ID3D12Device> device,
                         DescriptorHeap* srv_heap, TextureResource* texture)
{
  if (device == nullptr ||
      srv_heap == nullptr ||
      texture == nullptr ||
      texture->IsValid() == false)
  {
    return false;
  }
  texture_ = texture;

  // ディスクリプタハンドル作成
  srv_heap->Allocate(&handle_);

  // SRV作成
  {
    const D3D12_RESOURCE_DESC texture_desc = texture->resource()->GetDesc();

    // シェーダリソースビュー (テクスチャのビュー)の設定
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
    // 2次元テクスチャとしての設定
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2次元テクスチャ
    // Shader4ComponentMappingはシェーダにデータを返すときのメモリの処理
    srv_desc.Shader4ComponentMapping =
      D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // 通常はこれでOK
      // 下の二つはテクスチャリソースと合わせておこう
    srv_desc.Format = texture_desc.Format;
    srv_desc.Texture2D.MipLevels = texture_desc.MipLevels;

    device->CreateShaderResourceView(
      texture->resource().Get(),
      &srv_desc,
      handle_.cpu_handle());
  }

  return true;
}

void TextureView::Release()
{

}

} // namespace ncc

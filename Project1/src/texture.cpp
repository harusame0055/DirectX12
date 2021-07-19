#include "texture.h"

#include "WICTextureLoader12.h"

using namespace Microsoft::WRL;

namespace ncc {

/// @brief �摜�t�@�C�����V�X�e���������Ƀ��[�h����
///   GPU�������Ƀ��\�[�X�̈���m�ۂ���.�f�[�^�]���͂��Ȃ��̂Œ���.
/// @param device ID3D12Device�|�C���^
/// @param texture TextureResource�̎Q��
/// @return �����Ȃ�true,���s�Ȃ�false.
bool LoadTextureFromFile(ComPtr<ID3D12Device> device, TextureResource* texture)
{
  // �s���Ȉ����Ȃ玸�s
  if (device == nullptr || texture->name_.empty())
  {
    return false;
  }

  texture->data_.reset();

  // �摜�t�@�C�����V�X�e���������Ƀ��[�h����GPU�������Ƀ��\�[�X�̈���m��
  // GPU�������ւ̓]���͎����ł��K�v������
  if (FAILED(DirectX::LoadWICTextureFromFile(
    device.Get(),
    texture->name_.c_str(),
    &texture->resource_,
    texture->data_,
    texture->subresource_)))
  {
    return false;
  }

  // ���]�����
  texture->is_upload_wait_ = true;

  return true;
}

//-------------------------------------------------------------------
// TextureUploadCommandList
//-------------------------------------------------------------------

/// @brief �e�N�X�`���A�b�v���[�h�R�}���h�L�^�J�n
/// @param device ID3D12Device�|�C���^
/// @return �����Ȃ�true,���s�Ȃ�false.
bool TextureUploadCommandList::BeginRecording(ComPtr<ID3D12Device> device)
{
  // �����ȃf�o�C�X��n����邩�A���łɋL�^�J�n��ԂȂ�Ύ��s
  if (device == nullptr || is_begin_recording_)
  {
    return false;
  }
  device_ = device;

  // �R�}���h�A���P�[�^�͓]�����Ɏg���̂Ăō��
  if (FAILED(device->CreateCommandAllocator(
    D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator_))))
  {
    return false;
  }
  allocator_->SetName(L"TextureUploadCommandList");

  // �R�}���h���X�g���]�����Ɏg���̂Ăō��
  if (FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
      allocator_.Get(), nullptr, IID_PPV_ARGS(&command_list_))))
  {
    return false;
  }
  command_list_->SetName(L"TextureUploadCommandList");

  is_begin_recording_ = true;  // �R�}���h�L�^�J�n��Ԃɂ���

  return true;
}

/// @brief �R�}���h�L�^�I��
/// @return �����Ȃ�true,���s�Ȃ�false.
bool TextureUploadCommandList::EndRecording()
{
  // �s���ȏ�ԂŌĂ΂ꂽ�玸�s
  if (is_begin_recording_ == false)
  {
    return false;
  }

  // �R�}���h���X�g����Ă���
  if (FAILED(command_list_->Close()))
  {
    return false; // Close�����͎��s�̉\��������
  }

  is_begin_recording_ = false;  // �L�^�I��

  return true;
}

/// @brief �o�b�t�@�f�[�^��GPU�������ɓ]��
/// @return �����Ȃ�true,���s�Ȃ�false.
bool TextureUploadCommandList::Execute()
{
  // �s���ȏ�ԂȂ玸�s
  if (is_begin_recording_)
  {
    return false;
  }

  // �e�N�X�`����1���]�����Ȃ��Ȃ珈���I��
  if (upload_list_.size() == 0)
  {
    return true;  // 1���]�����Ȃ������Ŏ��s�ł͂Ȃ�
  }

  // �A�b�v���[�h���s�p�̃L���[�ƃt�F���X�͎g���̂Ăō��
  ComPtr<ID3D12CommandQueue> queue;
  ComPtr<ID3D12Fence> fence;

  // �t�F���X�쐬
  if (FAILED(device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))))
  {
    return false;
  }
  fence->SetName(L"TextureUploadCommandList");

  // �L���[�쐬
  {
    D3D12_COMMAND_QUEUE_DESC desc{};
    desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; // ��ʓI�ȕ`��p�͂���
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; // ��{�I��NONE��OK

    if (FAILED(device_->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue))))
    {
      return false;
    }
    queue->SetName(L"TextureUploadCommandList");
  }

  // �t�F���X���Ď�����C�x���g
  Microsoft::WRL::Wrappers::Event event;
  event.Attach(CreateEventEx(
    nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
  if (!event.IsValid())
  {
    return false;
  }

  // �R�}���h���s
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

  // ���\�[�X�Ƃ��Ďg����悤�ɂȂ����̂ŗL����Ԃɐݒ�
  for (int i = 0; i < upload_list_.size(); i++)
  {
    upload_list_[i]->is_upload_wait_ = false;
    upload_list_[i]->is_enabled_ = true;
  }

  // �]��������̌�Еt��
  upload_list_.clear();
  buffers_.clear();
  command_list_.Reset();
  allocator_.Reset();

  return true;
}

/// @brief GPU�������ɃA�b�v���[�h������TextureResource���R�}���h���X�g�ɒǉ�����
/// @param texture �Ώۂ̃e�N�X�`��
/// @return �����Ȃ�true,���s�Ȃ�false.
bool TextureUploadCommandList::AddList(TextureResource* texture)
{
  // �s���ȏ�ԂȂ玸�s
  if (texture->is_upload_wait_ == false ||
      texture->is_enabled_ == true ||
      is_begin_recording_ == false)
  {
    return false;
  }

  // �X�e�[�W���O�o�b�t�@�쐬
  // �e�N�X�`�����\�[�X��CPU���璼�ڏ������߂Ȃ��X�e�[�g�ō쐬����Ă���
  // ������ Map�\ (CPU�A�N�Z�X�\) �ȃ��\�[�X (�X�e�[�W���O�o�b�t�@) �Ƀe�N�X�`���f�[�^����������,
  // �����ăX�e�[�W���O�o�b�t�@�̃f�[�^ "GPU" ���g���ăe�N�X�`�����\�[�X�ɃR�s�[����
  ComPtr<ID3D12Resource> staging_buffer;

  D3D12_PLACED_SUBRESOURCE_FOOTPRINT layouts[1]{};  // �T�u���\�[�X���̃������̃f�[�^�`��
  UINT num_rows[1]{};         // �e�N�X�`�����̃T�u���\�[�X���̍s�����󂯎��
  UINT64 row_size_bytes[1]{}; // �T�u���\�[�X����1�s������̃o�C�g�T�C�Y
  UINT64 total_bytes = 0;     // �e�N�X�`���S�̂̃o�C�g�T�C�Y
  {
    D3D12_RESOURCE_DESC resource_desc = texture->resource_->GetDesc();

    // �o�b�t�@�ɃR�s�[����Ƃ��ɕK�v�ɂȂ�����擾
    device_->GetCopyableFootprints(
      &resource_desc,
      0,
      1,
      0,
      layouts,
      num_rows,
      row_size_bytes,
      &total_bytes);

    // �X�e�[�W���O�o�b�t�@�p�̃q�[�v�ݒ�
    D3D12_HEAP_PROPERTIES buffer_heap{};
    buffer_heap.Type = D3D12_HEAP_TYPE_UPLOAD;  // CPU���珑�����݉\�ɂ���
    buffer_heap.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    buffer_heap.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    buffer_heap.CreationNodeMask = 1;
    buffer_heap.VisibleNodeMask = 1;

    // �X�e�[�W���O�o�b�t�@�̃��\�[�X�ݒ�
    D3D12_RESOURCE_DESC buffer_desc{};
    buffer_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    buffer_desc.Alignment = 0;
    buffer_desc.Width = total_bytes;  // �o�b�t�@�S�̂̃T�C�Y
    buffer_desc.Height = 1;
    buffer_desc.DepthOrArraySize = 1;
    buffer_desc.MipLevels = 1;
    buffer_desc.Format = DXGI_FORMAT_UNKNOWN;
    buffer_desc.SampleDesc.Count = 1;
    buffer_desc.SampleDesc.Quality = 0;
    buffer_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    buffer_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    // �X�e�[�W���O�o�b�t�@�쐬
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

  // �X�e�[�W���O�o�b�t�@�փe�N�X�`���f�[�^���R�s�[
  {
    // �o�b�t�@��Map���ď����݃A�h���X�����炤��
    void* buffer_ptr = nullptr;
    if (FAILED(staging_buffer->Map(
      0,
      nullptr, // D3D12_RANGE. ���͑S�̂�Map����Ƃ���nullptr��OK
      &buffer_ptr)))
    {
      return false;
    }

    // �R�s�[���̃A�h���X��������
    const std::uint8_t* src_data = static_cast<const std::uint8_t*>(texture->subresource_.pData);
    LONG_PTR src_row_pitch = texture->subresource_.RowPitch;
    UINT copy_size = static_cast<UINT>(row_size_bytes[0]);

    // �V�X�e���������ɂ���摜����o�b�t�@�ցA1�s���Ƀ������R�s�[
    for (UINT row = 0; row < num_rows[0]; ++row)
    {
      void* dest_ptr = static_cast<std::uint8_t*>(buffer_ptr) + (layouts[0].Footprint.RowPitch * row);
      const void* src_ptr = src_data + (src_row_pitch * row);

      memcpy(dest_ptr, src_ptr, copy_size);
    }
    staging_buffer->Unmap(0, nullptr);  // �R�s�[�I���
  }

  // �X�e�[�W���O�o�b�t�@���烊�\�[�X�ւ̃R�s�[�R�}���h�쐬
  {
    D3D12_TEXTURE_COPY_LOCATION src{};
    D3D12_TEXTURE_COPY_LOCATION dst{};

    // �R�s�[�� (�e�N�X�`�����\�[�X) �̐ݒ�
    src.pResource = staging_buffer.Get();                 // �R�s�[���̃��\�[�X
    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;  // �o�b�t�@���͂���
    src.PlacedFootprint = layouts[0];                     // �R�s�[����Ƃ��Ɏg�����C�A�E�g

    // �R�s�[�� (�X�e�[�W���O�o�b�t�@) �̐ݒ�
    dst.pResource = texture->resource_.Get();               // �R�s�[��̃��\�[�X
    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX; // �R�s�[��͂���
    dst.SubresourceIndex = 0;                             // �R�s�[��̃T�u���\�[�X�ԍ�

    // �R�s�[�R�}���h
    command_list_->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

    // �f�[�^��]��������������e�N�X�`�����\�[�X�̓e�N�X�`���Ƃ��Ďg����X�e�[�g�ɑJ��
    // �X�e�[�g�J�ڂȂ̂Ń��\�[�X�o���A�R�}���h���K�v
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = texture->resource_.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    // �e�N�X�`�����\�[�X�̓��[�h��̓R�s�[������ (COPY_DEST)�ɂȂ��Ă�
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    // �o�b�t�@����R�s�[���I��������̓s�N�Z���V�F�[�_����݂����Ԃ֑J��
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    // �o���A�R�}���h
    command_list_->ResourceBarrier(1, &barrier);
  }

  // �]���������Ƀe�N�X�`�����\�[�X��L���������邽�ߊo���Ă���
  upload_list_.emplace_back(texture);
  // staging_buffer�̒��g�͂܂����]���Ȃ̂ŁA�����Ȃ��悤�Ɋo���Ă���
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

  // �f�B�X�N���v�^�n���h���쐬
  srv_heap->Allocate(&handle_);

  // SRV�쐬
  {
    const D3D12_RESOURCE_DESC texture_desc = texture->resource()->GetDesc();

    // �V�F�[�_���\�[�X�r���[ (�e�N�X�`���̃r���[)�̐ݒ�
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
    // 2�����e�N�X�`���Ƃ��Ă̐ݒ�
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2�����e�N�X�`��
    // Shader4ComponentMapping�̓V�F�[�_�Ƀf�[�^��Ԃ��Ƃ��̃������̏���
    srv_desc.Shader4ComponentMapping =
      D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // �ʏ�͂����OK
      // ���̓�̓e�N�X�`�����\�[�X�ƍ��킹�Ă�����
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

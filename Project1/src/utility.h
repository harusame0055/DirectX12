#pragma once
#include<string>

#include<d3d12.h>
#include<wrl.h>

namespace ncc {

	/// @brief HLSL���R���p�C�����Ď��s�\�ȃI�u�W�F�N�g�����
	/// @param filename �R���p�C������HLSL�t�@�C���p�X
	/// @param profile �V�F�[�_�[�[�v���t�@�C��
	/// @param shader_blob �R���p�C�����HLSL�f�[�^���󂯎��
	/// @param error_blob ���s���͂������ɃG���[���I�u�W�F�N�g������
	/// @return ���������Ȃ�S_OK / �����łȂ����S_FALSE
	HRESULT CompileShaderFromFile(const std::wstring& filename,
		const std::wstring& profile,
		Microsoft::WRL::ComPtr<ID3DBlob>& shader_blob,
		Microsoft::WRL::ComPtr<ID3DBlob>& error_blob);

	/// @brief HLSL�t�@�C������Blob�I�u�W�F�N�g�����
	/// @param filename Blob������HLSL�t�@�C���p�X
	/// @param profile �V�F�[�_�[�v���t�@�C��
	/// @param shader_blob ���������Ȃ�true / �����łȂ����false
	/// @return 
	bool CreateShaderBlob(const std::wstring& filename,
		const std::wstring& profile,
		Microsoft::WRL::ComPtr<ID3DBlob>& shader_blob);

}

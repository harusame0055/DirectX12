#include"utility.h"

#include<filesystem>
#include<fstream>

#include<dxcapi.h>

namespace ncc {
	using namespace Microsoft::WRL;

	HRESULT CompileShaderFromFile(const std::wstring& filename,
		const std::wstring& profile,
		ComPtr<ID3DBlob>& shader_blob,
		ComPtr<ID3DBlob>& error_blob)
	{
		HRESULT hr = S_FALSE;

		// �t�@�C��������p�X�����
		std::filesystem::path file_path(filename);
		//�t�@�C����ǂݎ��I�u�W�F�N�g
		std::ifstream in_file(file_path);

		// �t�@�C�����Ȃ���Ύ��s
		if (!in_file)
		{
			return hr;
		}

		// �t�@�C�����������ޓ��I�z��
		std::vector<char> shader_src;
		// shader_src�̃T�C�Y���t�@�C���ɍ��킹��
		std::size_t file_size = static_cast<std::size_t>
			(in_file.seekg(0, in_file.end).tellg());
		shader_src.resize(file_size);

		// �t�@�C���S�̂�shader_src�ɓǂݍ���
		in_file.seekg(0, in_file.beg).read(shader_src.data(), shader_src.size());

		// �ǂݍ��񂾃t�@�C��(�o�C�g�f�[�^)���V�F�[�_�[�\�[�X�ɕϊ�����I�u�W�F�N�g
		ComPtr<IDxcLibrary> library;
		DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library));

		// �V�F�[�_�\�[�X���󂯎��I�u�W�F�N�g
		ComPtr<IDxcBlobEncoding> source;
		// �o�C�g�f�[�^����V�F�[�_�[�\�[�X�ɕϊ�
		library->CreateBlobWithEncodingFromPinned(
			shader_src.data(),
			static_cast<UINT32>(shader_src.size()),
			CP_UTF8,	// �t�@�C���̕����R�[�h, CP_UTF8 �� UTF-8
			&source);	// �󂯎��ϐ�

		//DXC�R���p�C���t���O
		LPCWSTR compiler_flags[]{
#if _DEBUG
			// �f�o�b�O�p�t���O
			L"/Zi",L"/O0",	// �f�o�b�O��񐶐��E�œK���Ȃ�
#else
			L"/O2"	// ���\�[�X�r���h�͍œK��
#endif
		};
		// �V�F�[�_�[����include���������Ă����
		ComPtr<IDxcIncludeHandler> include_handler;
		library->CreateIncludeHandler(include_handler.ReleaseAndGetAddressOf());

		// �R���p�C���̌��ʂ��󂯎��I�u�W�F�N�g
		ComPtr<IDxcOperationResult> result;

		// �V�F�[�_�[�R���p�C��
		ComPtr<IDxcCompiler> compiler;
		DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));

		// �V�F�[�_�[�f�[�^�����s�\�ȏ�ԂɃR���p�C��
		compiler->Compile(source.Get(),	//�V�F�[�_�[�\�[�X
			file_path.wstring().c_str(),		// �t�@�C���p�X
			L"main",	// �R���p�C���Ώۂ̃G���g���[�|�C���g
			profile.c_str(),		// �V�F�[�_�[�̃v���t�@�C��
			compiler_flags,	// �R���p�C���t���O
			_countof(compiler_flags),	// �t���O�̗v�f��
			nullptr,						//��{�I�ɂ�nullptr��OK
			0,							// 0��OK
			include_handler.Get(),	// IDxcIncludeHandler
			&result);					// �R���p�C������

		// �R���p�C���̌��ʂ��`�F�b�N
		result->GetStatus(&hr);
		if (SUCCEEDED(hr))
		{
			// �R���p�C������, shader_blob�ɃR���p�C����̃f�[�^������
			result->GetResult(
				reinterpret_cast<IDxcBlob**>(shader_blob.GetAddressOf()));
		}
		else
		{
			//�R���p�C�����s, �G���[��n��
			result->GetErrorBuffer(
				reinterpret_cast<IDxcBlobEncoding**>(error_blob.GetAddressOf()));
		}

		return hr;
	}

	bool CreateShaderBlob(const std::wstring& filename, const std::wstring& profile,
		ComPtr<ID3DBlob>& shader_blob)
	{
		bool ret = true;

		// �V�F�[�_�[�v���O�����͎��O�R���p�C�����Ă������@�����邪����͓s�x�R���p�C��
		ComPtr<ID3DBlob> error;
		// ��ō�����֐��f�R���p�C��
		auto hr = CompileShaderFromFile(filename, profile, shader_blob, error);

		// ���ʂ��`�F�b�N
		if (FAILED(hr))
		{
			OutputDebugStringA((const char*)error->GetBufferPointer());
			OutputDebugStringA("CompileShaderFromFile Failed.");
			ret = false;
		}
		return ret;
	}
}	// namespace ncc
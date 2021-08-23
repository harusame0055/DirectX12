//Windows.hからゲームでは使わないヘッダを排除するマクロ
#define	WIN32_LEAN_AND_MEAN
#define	NOMINMAX
#define	NODRAWTEXT
#define	NOGDI
#define	NOBITMAP
#define	NOMCX
#define	NOSERVICE
#define	NOHELP

//Windows APIを使うためのヘッダファイル
#include<Windows.h>

//DEBUGビルド時のメモリリークチェック
#if _DEBUG
#define _CRTDBG_MAP_ALLOC
#include<crtdbg.h>
#endif

//ComPtr
#include<wrl/client.h>
#include<wrl.h>

/*----------------------------------------------*/
/*  C++ 標準ライブラリ                             */
/*----------------------------------------------*/
#include<string>
#include<filesystem>
#include<fstream>
#include<vector>

/*----------------------------------------------*/
/*  DirectX関連ヘッダ                                 */
/*----------------------------------------------*/
#include<d3d12.h>
#include<dxgi1_6.h>
#include<DirectXMath.h>
#include<dxcapi.h>

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

/*----------------------------------------------*/
/*  DirectX関連のライブラリ                         */
/*----------------------------------------------*/
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"dxcompiler.lib")


/*----------------------------------------------------*/
/* 自作したヘッダーファイル                               */
/*----------------------------------------------------*/
#include"fps_timer.h"
#include"keyboard.h"
#include"device_context.h"
#include"sprite.h"
#include"game.h"


//無名名前空間
namespace {

	//ウィンドウクラスネームの文字配列
	constexpr wchar_t kWindowClassName[]{ L"DirecrtX12WindowClass" };

	//タイトルバーの表示される文字列
	constexpr wchar_t kWindowTitle[]{ L"DirectX12 Application" };

	//デフォルトのクライアント領域
	constexpr int clienWidth = 800;
	constexpr int clienHeight = 600;

	//ComPtrのnamespaceが長いのでエイリアス(別名)を設定
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	//usingディレクティブ，　名前空間　DirectXを省略可能にする
	using namespace DirectX;

	/*-------------------------------------------------*/
	/* その他、FPS管理・キー入力                         */
	/*-------------------------------------------------*/
	ncc::FpsTimer fps_timer;	// ゲームのフレームレート管理
	std::unique_ptr<DirectX::Keyboard> keyboard;	// キーボードの入力管理
	Keyboard::KeyboardStateTracker kb_tracker;		// 詳細なキーの状態が取れる

	/*--------------------- */
	/* D3D12デバイス     */
	/*----------------------*/
	ncc::DeviceContext device_context_;

	/*------------------------------*/
	/* ゲームクラス                    */
	/*------------------------------*/
	ncc::Game game;

	/*-------------------------------*/
	/* プロトタイプ宣言               */
	/*-------------------------------*/
	LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

	bool PrepareResources();
	void Render();
	void Update();

	/*---------------------------*/
	/* 関数定義                      */
	/*---------------------------*/

	/// @brief ウィンドウプロシージャ
	/// @param hwnd ウィンドウハンドル
	/// @param message メッセージの種類
	/// @param wparam メッセージのパラメータ
	/// @param lparam メッセージのパラメータ
	/// @return メッセージの戻り値
	LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
	{
		switch (message)
		{
		case WM_CLOSE:	//プログラムの終了シグナル
			// ウィンドウは気，WM_DESTORYが送出
			DestroyWindow(hwnd);
			break;

		case WM_DESTROY:	//ウィンドウの破棄シグナル
			// プログラムを終了する，WM_QUITが送出
			PostQuitMessage(0);
			break;

		case WM_ACTIVATEAPP:
			DirectX::Keyboard::ProcessMessage(message, wparam, lparam);
			break;

			// キーボード入力
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		case  WM_KEYUP:
		case WM_SYSKEYUP:
			DirectX::Keyboard::ProcessMessage(message, wparam, lparam);
			break;

		default:
			// DefWindowProcでメッセージに応じたデフォルト処理になる
			return DefWindowProc(hwnd, message, wparam, lparam);
			break;
		}
		return 0;
	}












}
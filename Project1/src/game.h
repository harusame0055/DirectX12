#pragma once
#include<vector>

#include<d3d12.h>
#include<wrl.h>

#include"collision.h"
#include"descriptor_heap.h"
#include"fps_timer.h"
#include"sprite_renderer.h"
#include"texture.h"

namespace DirectX {
	class Keyboard;
}

namespace ncc {

	class DeviceContext;

	/// @brief プレイヤー構造体
	struct Player {
		Sprite sprite{};
		CircleCollider collider{};

		// 移動速度
		float speed = 100.0f;
		// 回転速度
		float rotation_speed = DirectX::XM_PI;
	};

	/// @brief バレット構造体
	struct Bullet {
		Sprite sprite{};
		CircleCollider collider{};

		// 弾の速度
		float speed = 100.0f;
	};

	/// @brief 敵構造体
	struct Enemy {
		// 出現持フェードイン処理に使う時間
		static constexpr float fadeTime{ 2.0f };

		// 敵の状態
		enum class State {
			None = 0,
			Spawn,
			Battle,
			Dead,
			Size
		};

		Sprite sprite{};
		CircleCollider collider{};

		// この敵データが使われているかのフラグ
		bool is_active{ false };

		// 現在の状態
		State state{ Enemy::State::Spawn };

		// 汎用タイマー
		float timer = 0;
	};

	/// @brief メインとなる処理を集めたクラス
	class Game {
	public:
		/// @brief 初期化
		/// @param device_context DeviceContext*
		/// @param keyboard DirectX::Keyboard*
		/// @retval true 成功
		/// @retval false 失敗
		bool Initialize(ncc::DeviceContext* device_context,
			DirectX::Keyboard* keyboard);

		/// @brief 終了処理
		void Finalize();

		/// @brief 更新処理
		/// @param timer FpsTimer&
		void Update(ncc::FpsTimer& timer);

		/// @brief 描画処理
		void Render();

	private:
		void CheckCollision();
		bool CreateTexture();
		void MakeEnemySprite(Sprite& sprite);
		void MakeExplosionSprite(Sprite& sprite);
		void MakePlayer();
		void SpawnEnemy(float delta_time);
		void UpdatePlayer(float delta_time);
		void UpdateEnemy(float delta_time);

		// スプライトレンダラ
		SpriteRenderer sprite_renderer_;

		// プレイヤー
		Player player_;

		// 敵の配列
		std::vector<Enemy>enemys_;
		// 敵の最大数
		static constexpr int enemySize = 10;

		// 敵は一定時間旅に沸く、2.0fで発生
		static constexpr float respawnTime = 2.0f;

		// リスポーン時間を管理
		float respawn_timer_ = 0.0f;

		//テクスチャの種類
		enum class TextureType : int {
			SHIPANIMATED,
			SPACESHIP,
			EXPLOSION,
			SIZE
		};

		// ファイル名の配列
		const std::vector<std::wstring>texture_path_{
			L"asset/shipmated.png",
			L"asset/Spaceship.png",
			L"asset/Explosion.png",
			L"asset/Bullet.png",
		};

		// テクスチャリソース
		std::vector<TextureResource> textures_;
		// テクスチャビュー
		std::vector<TextureView>texture_views_;
		// テクスチャビューのヒープ
		DescriptorHeap srv_cbv_heap_;

		// デバイス
		DeviceContext* device_context_ = nullptr;
		// キーボード
		DirectX::Keyboard* keyboard_ = nullptr;

	};

}	// namespace ncc
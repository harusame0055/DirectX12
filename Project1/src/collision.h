#pragma once
#include<DirectXMath.h>

namespace ncc {

	/// @brief 円形コライダー
	struct CircleCollider {
		// 円の半径
		float radius = 0;
		// 円の原点
		DirectX::XMFLOAT2 center{};
	};

	/// @brief 円と円の衝突判定
	/// @param c1 コライダー１
	/// @param p1 コライダー１の座標
	/// @param c2 コライダー２
	/// @param p2 コライダー２の座標
	/// @retval true 衝突している
	/// @retval false 衝突してない
	bool CollisionCircleToCircle(const CircleCollider& c1, const DirectX::XMFLOAT3& p1, const CircleCollider& c2, const DirectX::XMFLOAT3& p2);
}
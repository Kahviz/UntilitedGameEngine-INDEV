#pragma once
#include "BoronMathLibrary.h"
#include "Widget.h"

namespace Borongui {
class Frame : public Widget {
public:
	void Render() override;
	bool setSize(const BML::Vec2& p_size);
	bool setPosition(const BML::Vec2& p_position);
	bool setColor(const BML::Vec3& p_color);
	const BML::Vec2& getPosition();
	const BML::Vec2& getSize();
	const BML::Vec3& getColor();
	const float getRounding();
private:
	float m_rounding = 10.0f;
	BML::Vec2 m_size = { 0,0 };
	BML::Vec2 m_position = { 0,0 };
	BML::Vec3 m_color = { 0,0,0 };
};
}
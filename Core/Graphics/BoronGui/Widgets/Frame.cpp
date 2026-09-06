#include "Frame.h"
#include "BoronGui.h"
#include "GLOBALS.h"

void Borongui::Frame::Render() {
	BoronGui::RenderAFrame(*this);
}

bool Borongui::Frame::setSize(const BML::Vec2& p_size) {
	m_size = p_size;

	return true;
}

bool Borongui::Frame::setPosition(const BML::Vec2& p_position) {
	m_position = p_position;

	return true;
}

bool Borongui::Frame::setColor(const BML::Vec3& p_color) {
	m_color = p_color;

	return true;
}

const BML::Vec2& Borongui::Frame::getPosition() {
	return m_position;
}

const BML::Vec2& Borongui::Frame::getSize() {
	return m_size;
}

const BML::Vec3& Borongui::Frame::getColor() {
	return m_color;
}

const float Borongui::Frame::getRounding() {
	return m_rounding;
}

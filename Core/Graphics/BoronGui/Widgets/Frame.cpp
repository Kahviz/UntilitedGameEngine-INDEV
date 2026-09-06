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

const BML::Vec2& Borongui::Frame::getPosition() const {
	return m_position;
}

const BML::Vec2& Borongui::Frame::getSize() const {
	return m_size;
}

const BML::Vec3& Borongui::Frame::getColor() const {
	return m_color;
}

const float Borongui::Frame::getRounding() const {
	return m_rounding;
}

const std::vector<uint32_t>& Borongui::Frame::getIndices() const {
	return m_indices;
}

const std::vector<Vertex2d>& Borongui::Frame::getVertices() const {
	return m_vertices;
}

void Borongui::Frame::setIndices(const std::vector<uint32_t>& p_indices) {
	m_indices = p_indices;
}

void Borongui::Frame::setVertices(const std::vector<Vertex2d>& p_vertices) {
	m_vertices = p_vertices;
}

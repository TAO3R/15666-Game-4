#include "Text.hpp"

#include <hb-ft.h>

#include <stdexcept>

TextRenderer::TextRenderer(std::string const &font_path, uint32_t pixel_size) {
	if (FT_Init_FreeType(&library)) throw std::runtime_error("failed to init freetype");
	if (FT_New_Face(library, font_path.c_str(), 0, &face)) throw std::runtime_error("failed to load font '" + font_path + "'");
	if (FT_Set_Pixel_Sizes(face, 0, pixel_size)) throw std::runtime_error("failed to set font pixel size");
	font = hb_ft_font_create_referenced(face);
}

TextRenderer::~TextRenderer() {
	for (auto const &[index, glyph] : cache) {
		if (glyph.tex) glDeleteTextures(1, &glyph.tex);
	}
	if (font) hb_font_destroy(font);
	if (face) FT_Done_Face(face);
	if (library) FT_Done_FreeType(library);
}

float TextRenderer::line_height() const {
	return face->size->metrics.height / 64.0f;
}

TextRenderer::Glyph const &TextRenderer::get(uint32_t glyph_index) {
	auto found = cache.find(glyph_index);
	if (found != cache.end()) return found->second;

	if (FT_Load_Glyph(face, glyph_index, FT_LOAD_RENDER)) throw std::runtime_error("failed to render glyph");
	FT_Bitmap const &bitmap = face->glyph->bitmap;

	Glyph glyph;
	glyph.size = glm::ivec2(bitmap.width, bitmap.rows);
	glyph.bearing = glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top);

	if (bitmap.width && bitmap.rows) {
		glGenTextures(1, &glyph.tex);
		glBindTexture(GL_TEXTURE_2D, glyph.tex);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1); //rows of an 8-bit bitmap aren't 4-byte aligned
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, bitmap.width, bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap.buffer);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		GLint swizzle[4] = { GL_ONE, GL_ONE, GL_ONE, GL_RED }; //coverage shows up as alpha
		glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	return cache.emplace(glyph_index, glyph).first->second;
}

std::vector< TextRenderer::Quad > TextRenderer::shape(std::string const &utf8) {
	hb_buffer_t *buf = hb_buffer_create();
	hb_buffer_add_utf8(buf, utf8.c_str(), int(utf8.size()), 0, int(utf8.size()));
	hb_buffer_guess_segment_properties(buf); //direction / script / language from the text itself
	hb_shape(font, buf, nullptr, 0);

	unsigned int count = 0;
	hb_glyph_info_t const *infos = hb_buffer_get_glyph_infos(buf, &count);
	hb_glyph_position_t const *positions = hb_buffer_get_glyph_positions(buf, &count);

	std::vector< Quad > quads;
	quads.reserve(count);
	glm::vec2 pen = glm::vec2(0.0f);
	for (unsigned int i = 0; i < count; ++i) {
		Glyph const &glyph = get(infos[i].codepoint); //after shaping, 'codepoint' is a glyph index
		if (glyph.tex) {
			Quad quad;
			quad.tex = glyph.tex;
			quad.min = pen
				+ glm::vec2(positions[i].x_offset, positions[i].y_offset) / 64.0f
				+ glm::vec2(float(glyph.bearing.x), float(glyph.bearing.y - glyph.size.y));
			quad.max = quad.min + glm::vec2(glyph.size);
			quads.emplace_back(quad);
		}
		pen += glm::vec2(positions[i].x_advance, positions[i].y_advance) / 64.0f;
	}

	hb_buffer_destroy(buf);
	return quads;
}

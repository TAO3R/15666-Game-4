#pragma once

#include "GL.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>

#include <glm/glm.hpp>

#include <string>
#include <unordered_map>
#include <vector>

//Shapes utf8 text with harfbuzz, rasterizes glyphs with freetype.
//one GL texture per glyph, cached by glyph index; pack into an atlas if draw calls ever matter.
struct TextRenderer {
	TextRenderer(std::string const &font_path, uint32_t pixel_size);
	~TextRenderer();

	struct Quad {
		GLuint tex = 0; //R8 coverage, swizzled to (1,1,1,coverage) so ColorTextureProgram works as-is
		glm::vec2 min = glm::vec2(0.0f); //lower-left, in pixels; baseline is y == 0, pen starts at x == 0
		glm::vec2 max = glm::vec2(0.0f); //upper-right
	};

	//shape one line of utf8 text into positioned glyph quads:
	std::vector< Quad > shape(std::string const &utf8);

	float line_height() const; //pixels between baselines

	FT_Library library = nullptr;
	FT_Face face = nullptr;
	hb_font_t *font = nullptr;

	struct Glyph {
		GLuint tex = 0; //0 for blank glyphs (space)
		glm::ivec2 size = glm::ivec2(0);
		glm::ivec2 bearing = glm::ivec2(0); //left, top -- relative to pen on the baseline
	};
	std::unordered_map< uint32_t, Glyph > cache;
	Glyph const &get(uint32_t glyph_index);
};

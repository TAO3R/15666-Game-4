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

	//quads to vertices, then to the GPU via color_texture_program.
	// clip_from_pixel maps quad pixel coordinates into clip space
	// (quad y is up, baseline sits at y == 0, pen starts at x == 0).
	void draw(std::vector< Quad > const &quads, glm::mat4 const &clip_from_pixel,
	glm::u8vec4 const &color = glm::u8vec4(0xff));
};

//pixel -> clip matrix for a drawable_size window.
// origin is where the pen starts, in pixels from the lower-left corner
// (it lands on the text's baseline, so leave room below for descenders).
inline glm::mat4 clip_from_pixel(glm::uvec2 const &drawable_size, glm::vec2 const &origin = glm::vec2(0.0f)) {
      float w = float(drawable_size.x);
      float h = float(drawable_size.y);
      //glm::mat4 takes *columns*:
      return glm::mat4(
              2.0f / w, 0.0f, 0.0f, 0.0f,
              0.0f, 2.0f / h, 0.0f, 0.0f,
              0.0f, 0.0f, 1.0f, 0.0f,
              2.0f * origin.x / w - 1.0f, 2.0f * origin.y / h - 1.0f, 0.0f, 1.0f
      );
}

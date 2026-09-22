#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint hexapod_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("hexapod.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > hexapod_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("hexapod.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = hexapod_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

Load< Sound::Sample > dusty_floor_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("dusty-floor.opus"));
});


Load< Sound::Sample > honk_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("honk.wav"));
});


//the whole UI is two colors; the selected choice just swaps them:
static glm::u8vec4 const Background = glm::u8vec4(0x73, 0xff, 0x00, 0xff);
static glm::u8vec4 const Foreground = glm::u8vec4(0x00, 0x00, 0x00, 0xff);

PlayMode::PlayMode() : scene(*hexapod_scene) {
	//get pointers to leg for convenience:
	for (auto &transform : scene.transforms) {
		if (transform.name == "Hip.FL") hip = &transform;
		else if (transform.name == "UpperLeg.FL") upper_leg = &transform;
		else if (transform.name == "LowerLeg.FL") lower_leg = &transform;
	}
	if (hip == nullptr) throw std::runtime_error("Hip not found.");
	if (upper_leg == nullptr) throw std::runtime_error("Upper leg not found.");
	if (lower_leg == nullptr) throw std::runtime_error("Lower leg not found.");

	hip_base_rotation = hip->rotation;
	upper_leg_base_rotation = upper_leg->rotation;
	lower_leg_base_rotation = lower_leg->rotation;

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	//start music loop playing:
	// (note: position will be over-ridden in update())
	leg_tip_loop = Sound::loop_3D(*dusty_floor_sample, 1.0f, get_leg_tip_position(), 10.0f);

	//story
	story = make_story();
	story_stage = 0;
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_EVENT_KEY_DOWN) {
		size_t choice_count = story[story_stage].choices.size();
		if (evt.key.key == SDLK_UP) {
			if (choice_count) selected = uint8_t((selected + choice_count - 1) % choice_count);
			return true;
		} else if (evt.key.key == SDLK_DOWN) {
			if (choice_count) selected = uint8_t((selected + 1) % choice_count);
			return true;
		} else if (evt.key.key == SDLK_RETURN || evt.key.key == SDLK_KP_ENTER) {
			if (selected < choice_count) {
				story_stage = story[story_stage].choices[selected].next;
				selected = 0; //the new node's choices are a different list
			}
			return true;
		}

		if (evt.key.key == SDLK_ESCAPE) {
			SDL_SetWindowRelativeMouseMode(Mode::window, false);
			return true;
		} else if (evt.key.key == SDLK_A) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_D) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_W) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_S) {
			down.downs += 1;
			down.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_SPACE) {
			if (honk_oneshot) honk_oneshot->stop();
			honk_oneshot = Sound::play_3D(*honk_sample, 0.3f, glm::vec3(4.6f, -7.8f, 6.9f)); //hardcoded position of front of car, from blender
		}
	} else if (evt.type == SDL_EVENT_KEY_UP) {
		if (evt.key.key == SDLK_A) {
			left.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_D) {
			right.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_W) {
			up.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_S) {
			down.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
		if (SDL_GetWindowRelativeMouseMode(Mode::window) == false) {
			SDL_SetWindowRelativeMouseMode(Mode::window, true);
			return true;
		}
	} else if (evt.type == SDL_EVENT_MOUSE_MOTION) {
		if (SDL_GetWindowRelativeMouseMode(Mode::window) == true) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			camera->transform->rotation = glm::normalize(
				camera->transform->rotation
				* glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f))
				* glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
			);
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	//slowly rotates through [0,1):
	wobble += elapsed / 10.0f;
	wobble -= std::floor(wobble);

	hip->rotation = hip_base_rotation * glm::angleAxis(
		glm::radians(5.0f * std::sin(wobble * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 1.0f, 0.0f)
	);
	upper_leg->rotation = upper_leg_base_rotation * glm::angleAxis(
		glm::radians(7.0f * std::sin(wobble * 2.0f * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 0.0f, 1.0f)
	);
	lower_leg->rotation = lower_leg_base_rotation * glm::angleAxis(
		glm::radians(10.0f * std::sin(wobble * 3.0f * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 0.0f, 1.0f)
	);

	//move sound to follow leg tip position:
	leg_tip_loop->set_position(get_leg_tip_position(), 1.0f / 60.0f);

	//move camera:
	{

		//combine inputs into a move:
		constexpr float PlayerSpeed = 30.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x =-1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y =-1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		glm::mat4x3 frame = camera->transform->make_parent_from_local();
		glm::vec3 frame_right = frame[0];
		//glm::vec3 up = frame[1];
		glm::vec3 frame_forward = -frame[2];

		camera->transform->position += move.x * frame_right + move.y * frame_forward;
	}

	{ //update listener to camera position:
		glm::mat4x3 frame = camera->transform->make_parent_from_local();
		glm::vec3 frame_right = frame[0];
		glm::vec3 frame_at = frame[3];
		Sound::listener.set_position_right(frame_at, frame_right, 1.0f / 60.0f);
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(Background.r / 255.0f, Background.g / 255.0f, Background.b / 255.0f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	// scene.draw(*camera);

	// { //use DrawLines to overlay some text:
	// 	glDisable(GL_DEPTH_TEST);
	// 	float aspect = float(drawable_size.x) / float(drawable_size.y);
	// 	DrawLines lines(glm::mat4(
	// 		1.0f / aspect, 0.0f, 0.0f, 0.0f,
	// 		0.0f, 1.0f, 0.0f, 0.0f,
	// 		0.0f, 0.0f, 1.0f, 0.0f,
	// 		0.0f, 0.0f, 0.0f, 1.0f
	// 	));

	// 	constexpr float H = 0.09f;
	// 	lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
	// 		glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
	// 		glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
	// 		glm::u8vec4(0x00, 0x00, 0x00, 0x00));
	// 	float ofs = 2.0f / drawable_size.y;
	// 	lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
	// 		glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
	// 		glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
	// 		glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	// }
	// GL_ERRORS();

	{	//use text renderer to overlay some text:
		// title.draw("ESCAPE WILL MAKE ME GOD", drawable_size, glm::vec2(20.0f, 20.0f));
		// body.draw("DEATH IS INEVITABLE", drawable_size, glm::vec2(20.0f, 180.0f));
		draw_story_node(story[story_stage], drawable_size);
	}
	GL_ERRORS();
}

glm::vec3 PlayMode::get_leg_tip_position() {
	//the vertex position here was read from the model in blender:
	return lower_leg->make_world_from_local() * glm::vec4(-1.26137f, -11.861f, 0.0f, 1.0f);
}

void PlayMode::draw_story_node(Node &n, glm::uvec2 const &drawable_size)
{
	//layout knobs, in multiples of the relevant font's line height:
	constexpr float BlockGap = 1.5f; //title -> body, body -> first choice
	constexpr float ChoiceGap = 1.4f; //between choices -- keep above 1.0 + 2*HighlightPad/choice.line_height() so the boxes don't touch

	constexpr float HighlightPad = 8.0f; //pixels of fill around the selected choice, same on all four sides

	//horizontally center one line on its own measured width:
	auto centered = [&drawable_size](TextRenderer &font, std::string const &text, float baseline_y, glm::u8vec4 const &color) {
		float x = 0.5f * (float(drawable_size.x) - font.measure(text));
		font.draw(text, drawable_size, glm::vec2(x, baseline_y), color);
	};

	//measure the whole block first so it can be centered vertically.
	// (this mirrors the baseline advances in the drawing code below -- change one, change the other)
	float advance = BlockGap * title.line_height(); //title baseline -> body baseline
	float bottom_descender = body.descender();
	if (!n.choices.empty()) {
		advance += BlockGap * body.line_height(); //body -> first choice
		advance += float(n.choices.size() - 1) * ChoiceGap * choice.line_height();
		bottom_descender = choice.descender();
	}
	float block_height = title.ascender() + advance + bottom_descender;

	//baseline walks down from the top of the block (origin is at the bottom, so y decreases):
	float y = 0.5f * (float(drawable_size.y) + block_height) - title.ascender();

	centered(title, n.title, y, Foreground);
	y -= BlockGap * title.line_height();

	centered(body, n.body, y, Foreground);
	y -= BlockGap * body.line_height();

	for (size_t i = 0; i < n.choices.size(); ++i) {
		bool is_selected = (i == size_t(selected));

		if (is_selected) { //fill the row with Foreground, then draw the text in Background
			float w = choice.measure(n.choices[i].text);
			float x = 0.5f * (float(drawable_size.x) - w);

			//the text's typographic box, padded equally on all four sides.
			// using ascender/descender (not the string's own ink) keeps every choice the same height:
			GLint x0 = GLint(std::floor(x - HighlightPad));
			GLint y0 = GLint(std::floor(y - choice.descender() - HighlightPad));
			GLint x1 = GLint(std::ceil(x + w + HighlightPad));
			GLint y1 = GLint(std::ceil(y + choice.ascender() + HighlightPad));

			//scissor + clear paints a solid rect without needing any geometry:
			glEnable(GL_SCISSOR_TEST);
			glScissor(x0, y0, GLsizei(x1 - x0), GLsizei(y1 - y0));
			glClearColor(Foreground.r / 255.0f, Foreground.g / 255.0f, Foreground.b / 255.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT); //color only -- clearing depth here would wipe the scene
			glDisable(GL_SCISSOR_TEST);
		}

		centered(choice, n.choices[i].text, y, (is_selected ? Background : Foreground));
		y -= ChoiceGap * choice.line_height();
	}
}

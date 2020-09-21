#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <filesystem>
#include <string>
#include <map>
#include <fstream>
#include <vector>
#include <set>
namespace fs = std::filesystem;

struct img_data
{
	struct size_data
	{
		size_t x, y, width, height;
		std::string texture;
	};

	size_data regular, small, tiny;
};

struct init_data
{
	std::string name;
	size_t x, y;
};

struct output_data
{
	using img_data_map = std::map<std::string, img_data>;
	std::vector<std::string> errors;
	std::map<std::string, img_data_map> result;
};

void copy_to_texture(uint8_t *texture, size_t texture_x, size_t texture_y, size_t sprite_count_x, size_t sprite_size, const uint8_t *input_data, size_t input_width, size_t input_height)
{
	static const stbir_edge edge = stbir_edge::STBIR_EDGE_CLAMP;
	static const stbir_filter filter = stbir_filter::STBIR_FILTER_BOX;
	static const stbir_colorspace colorspace = stbir_colorspace::STBIR_COLORSPACE_LINEAR;

	size_t source_stride = input_width * 3;
	size_t target_stride = sprite_count_x * sprite_size * 3;
	stbir_resize_uint8(input_data, input_width, input_height, source_stride, texture + texture_y * sprite_size * target_stride + texture_x * sprite_size * 3, sprite_size, sprite_size, target_stride, 3);
}

void write_to_texture_file(const std::string &set_name, size_t sprite_count_x, size_t sprite_count_y, size_t sprite_size, const std::string &texture_id, const uint8_t *texture, std::vector<std::string> &errors)
{
	std::string file_name = std::string("img/sprite/") + set_name + texture_id + ".png";

	fs::path parent_folder = fs::path(file_name).parent_path();
	if (fs::exists(parent_folder) == false)
		fs::create_directories(parent_folder);

	if (stbi_write_png(file_name.c_str(), sprite_size * sprite_count_x, sprite_size * sprite_count_y, 3, texture, 0) == 0)
		errors.push_back("Unable to write file '" + file_name + "'");
}

void write_image_data(img_data::size_data &size_data, const std::string &texture_name, size_t sprite_size, size_t texture_x, size_t texture_y, size_t texture_id)
{
	size_data.texture = texture_name + std::to_string(texture_id);
	size_data.x = texture_x * sprite_size;
	size_data.y = texture_y * sprite_size;
	size_data.width = size_data.height = sprite_size;
}

void write_image(const fs::path root, std::map<std::string, img_data> &image_data, const char *set_name, size_t sprite_count_x, size_t sprite_count_y, std::vector<std::string> &errors)
{
	static const size_t sprite_size = 48;
	static const size_t small_sprite_size = 36;
	static const size_t tiny_sprite_size = 24;

	fs::path set_path = root / fs::path(std::string("img/") + set_name);
	if (fs::exists(set_path) == false)
	{
		errors.push_back("Dragon-spriter is expecting a path at '" + fs::absolute(set_path).generic_string() + "', but it did not exist!");
		return;
	}

	std::vector<uint8_t> texture(sprite_size * sprite_size * sprite_count_x * sprite_count_y * 3);
	std::vector<uint8_t> small_texture(small_sprite_size * small_sprite_size * sprite_count_x * sprite_count_y * 3);
	std::vector<uint8_t> tiny_texture(tiny_sprite_size * tiny_sprite_size * sprite_count_x * sprite_count_y * 3);
	size_t texture_x = 0;
	size_t texture_y = 0;
	size_t texture_id = 0;

	std::string texture_name = set_name;
	std::string id_name = std::to_string(texture_id);
	std::string small_texture_name = "small_" + texture_name;
	std::string tiny_texture_name = "tiny_" + texture_name;

	std::set<fs::directory_entry> sorted_by_name;
	for (auto &entry : fs::directory_iterator(set_path))
		sorted_by_name.insert(entry);

	for (auto &file : sorted_by_name)
	{
		if (file.is_regular_file() == false)
			continue;

		// Load texture
		int input_width, input_height, bpp;
		uint8_t *input_data = stbi_load(file.path().generic_string().c_str(), &input_width, &input_height, &bpp, 3);
		if (input_data == nullptr)
		{
			errors.push_back("Unable to read '" + file.path().generic_string() + "'.");
			continue;
		}

		// Resize
		copy_to_texture(texture.data(), texture_x, texture_y, sprite_count_x, sprite_size, input_data, input_width, input_height);
		copy_to_texture(small_texture.data(), texture_x, texture_y, sprite_count_x, small_sprite_size, input_data, input_width, input_height);
		copy_to_texture(tiny_texture.data(), texture_x, texture_y, sprite_count_x, tiny_sprite_size, input_data, input_width, input_height);

		std::string champion_name = file.path().stem().generic_string();
		write_image_data(image_data[champion_name].regular, texture_name, sprite_size, texture_x, texture_y, texture_id);
		write_image_data(image_data[champion_name].small, small_texture_name, sprite_size, texture_x, texture_y, texture_id);
		write_image_data(image_data[champion_name].tiny, tiny_texture_name, sprite_size, texture_x, texture_y, texture_id);

		texture_x++;
		if (texture_x == sprite_count_x)
		{
			texture_x = 0;
			texture_y++;

			if (texture_y == sprite_count_y)
			{
				write_to_texture_file(texture_name, sprite_count_x, sprite_count_y, sprite_size, id_name, texture.data(), errors);
				write_to_texture_file(small_texture_name, sprite_count_x, sprite_count_y, small_sprite_size, id_name, small_texture.data(), errors);
				write_to_texture_file(tiny_texture_name, sprite_count_x, sprite_count_y, tiny_sprite_size, id_name, tiny_texture.data(), errors);

				texture_id++;
				texture_x = 0;
				texture_y = 0;
				// memset(texture.data(), 0, texture.size());

				texture_name = set_name;
				id_name = std::to_string(texture_id);
				small_texture_name = "small_" + texture_name;
				tiny_texture_name = "tiny_" + texture_name;
			}
		}

		stbi_image_free(input_data);
	}

	// Write out rest of elements
	if (texture_x != 0 || texture_y != 0)
	{
		std::string name = set_name;
		std::string id = std::to_string(texture_id);

		write_to_texture_file(name, sprite_count_x, texture_y + 1, sprite_size, id, texture.data(), errors);
		write_to_texture_file("small_" + name, sprite_count_x, texture_y + 1, small_sprite_size, id, small_texture.data(), errors);
		write_to_texture_file("tiny_" + name, sprite_count_x, texture_y + 1, tiny_sprite_size, id, tiny_texture.data(), errors);
	}
}

void to_json(std::string &out, const std::string &in)
{
	out += "\"" + in + "\"";
}

void to_json(std::string &out, size_t in)
{
	out += std::to_string(in);
}

template <typename T>
void to_json(std::string &out, const std::vector<T> &array)
{
	out += "[";

	bool first = true;
	for (auto &entry : array)
	{
		if (first)
			first = false;
		else
			out += ",";

		to_json(out, entry);
	}

	out += "]";
}

template <typename T>
void to_json_keyval(std::string &out, const std::string &key, const T &value, bool last)
{
	out += "\"" + key + "\":";

	to_json(out, value);

	if (last == false)
		out += ",";
}

void to_json(std::string &out, const img_data::size_data &in)
{
	out += "{";

	to_json_keyval(out, "x", in.x, false);
	to_json_keyval(out, "y", in.y, false);
	to_json_keyval(out, "width", in.width, false);
	to_json_keyval(out, "height", in.height, false);
	to_json_keyval(out, "texture", in.texture, true);

	out += "}";
}

void to_json(std::string &out, const img_data &in)
{
	out += "{";

	to_json_keyval(out, "regular", in.regular, false);
	to_json_keyval(out, "small", in.small, false);
	to_json_keyval(out, "tiny", in.tiny, true);

	out += "}";
}

template <typename T>
void to_json(std::string &out, const std::map<std::string, T> &map)
{
	out += "{";

	bool first = true;
	for (auto &pair : map)
	{
		if (first)
			first = false;
		else
			out += ",";

		to_json_keyval(out, pair.first, pair.second, true);
	}

	out += "}";
}

void to_json(std::string &out, const output_data &in)
{
	out += "{";

	to_json_keyval(out, "errors", in.errors, false);
	to_json_keyval(out, "result", in.result, true);

	out += "}";
}

int main(int argc, char **argv)
{
	fs::path root = (argc > 1) ? argv[1] : "/";

	output_data output;
	init_data datas[] =
		{
			{"champion", 10, 3},
			{"item", 10, 10},
			{"map", 6, 6},		 // Don't know the Y of this, so I'm assuming it's simply square.
			{"mission", 10, 20}, // Possibly infinite Y?
			{"passive", 10, 3},
			{"profileicon", 10, 300}, //Seems to be infinite Y
			{"spell", 10, 4}};

	for (auto &init : datas)
		write_image(root, output.result[init.name], init.name.c_str(), init.x, init.y, output.errors);

	std::string json;
	to_json(json, output);
	std::ofstream f(root / "spriter_output.json");
	f.write(json.c_str(), json.length());

	return output.errors.size() ? -1 : 0;
}

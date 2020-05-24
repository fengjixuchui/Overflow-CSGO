#include "../features.hpp"
#include "../../../source-sdk/math/vector3d.hpp"

constexpr float SPEED_FREQ = 255 / 1.0f;

struct box {
	int x, y, w, h;
	box() = default;
	box(int x, int y, int w, int h) {
		this->x = x;
		this->y = y;
		this->w = w;
		this->h = h;
	}
};

std::string clean_item_name(std::string name) {
	std::string Name = name;

	auto weapon_start = Name.find("weapon");
	if (weapon_start != std::string::npos)
		Name.erase(Name.begin() + weapon_start, Name.begin() + weapon_start + 6);

	if (Name[0] == '_')
		Name.erase(Name.begin());

	if (Name[0] == 'c') //optional for dropped weapons - designer
		Name.erase(Name.begin());

	return Name;
}

bool get_playerbox(player_t* ent, box& in) {
	vec3_t origin, min, max, flb, brt, blb, frt, frb, brb, blt, flt;
	float left, top, right, bottom;

	origin = ent->abs_origin();
	min = ent->collideable()->mins() + origin;
	max = ent->collideable()->maxs() + origin;

	vec3_t points[] = {
		vec3_t(min.x, min.y, min.z),
		vec3_t(min.x, max.y, min.z),
		vec3_t(max.x, max.y, min.z),
		vec3_t(max.x, min.y, min.z),
		vec3_t(max.x, max.y, max.z),
		vec3_t(min.x, max.y, max.z),
		vec3_t(min.x, min.y, max.z),
		vec3_t(max.x, min.y, max.z)
	};

	if (!interfaces::debug_overlay->world_to_screen(points[3], flb) || !interfaces::debug_overlay->world_to_screen(points[5], brt)
		|| !interfaces::debug_overlay->world_to_screen(points[0], blb) || !interfaces::debug_overlay->world_to_screen(points[4], frt)
		|| !interfaces::debug_overlay->world_to_screen(points[2], frb) || !interfaces::debug_overlay->world_to_screen(points[1], brb)
		|| !interfaces::debug_overlay->world_to_screen(points[6], blt) || !interfaces::debug_overlay->world_to_screen(points[7], flt))
		return false;

	vec3_t arr[] = { flb, brt, blb, frt, frb, brb, blt, flt };

	left = flb.x;
	top = flb.y;
	right = flb.x;
	bottom = flb.y;

	for (int i = 1; i < 8; i++) {
		if (left > arr[i].x)
			left = arr[i].x;
		if (bottom < arr[i].y)
			bottom = arr[i].y;
		if (right < arr[i].x)
			right = arr[i].x;
		if (top > arr[i].y)
			top = arr[i].y;
	}

	in.x = (int)left;
	in.y = (int)top;
	in.w = int(right - left);
	in.h = int(bottom - top);

	return true;
}

void features::visuals::render_esp()
{
	if (menu.config.anti_screenshot && interfaces::engine->is_taking_screenshot())
		return;

	auto local_player = reinterpret_cast<player_t*>(interfaces::entity_list->get_client_entity(interfaces::engine->get_local_player()));

	for (int i = 1; i <= interfaces::globals->max_clients; i++) 
	{
		auto entity = reinterpret_cast<player_t*>(interfaces::entity_list->get_client_entity(i));

		if (!entity || entity == local_player)
			continue;

		if (!entity->is_alive() || !entity->is_player() || entity->health() <= 0)
			continue;

		if (entity->dormant())
			continue;

		if (entity->team() == local_player->team() && !menu.config.team_check)
			continue;

		box bbox;
		if (!get_playerbox(entity, bbox))
			continue;

		if (menu.config.box)
		{
			if (menu.config.box_outline)
				render::draw_outline(bbox.x - 1, bbox.y - 1, bbox.w + 2, bbox.h + 2, (entity->team() == local_player->team() ? color(10, 255, 10) : menu.config.box_outline_clr));
			render::draw_rect(bbox.x, bbox.y, bbox.w, bbox.h, (entity->team() == local_player->team() ? color(10, 255, 10) : menu.config.box_clr));
			if (menu.config.box_outline)
				render::draw_outline(bbox.x + 1, bbox.y + 1, bbox.w - 2, bbox.h-+ 2, (entity->team() == local_player->team() ? color(10, 255, 10) : menu.config.box_outline_clr));
		}

		if (menu.config.corner_box)
		{
			if (menu.config.corner_box_outline)
				render::draw_corner_box(bbox.x - 1, bbox.y - 1, bbox.w + 1, bbox.h + 1, menu.config.corner_outline_clr);
			render::draw_corner_box(bbox.x, bbox.y, bbox.w, bbox.h, (entity->team() == local_player->team() ? color(10, 255, 10) : menu.config.corner_box_clr));
			if (menu.config.corner_box_outline)
				render::draw_corner_box(bbox.x + 1, bbox.y + 1, bbox.w - 2, bbox.h - 2, menu.config.corner_outline_clr);
		}

		if (menu.config.health)
		{
			int health = entity->health();
			box temp(bbox.x - 5, bbox.y + (bbox.h - bbox.h * (math::clamp_value<int>(entity->health(), 0, 100.f) / 100.f)), 1, bbox.h * (math::clamp_value<int>(entity->health(), 0, 100) / 100.f) - (entity->health() >= 100 ? 0 : -1));
			box temp_bg(bbox.x - 5, bbox.y, 1, bbox.h);
			render::draw_filled_rect(temp_bg.x - 1, temp_bg.y - 1, temp_bg.w + 2, temp_bg.h + 2, color(0, 0, 0));
			render::draw_filled_rect(temp.x, temp.y, temp.w, temp.h, menu.config.health_clr);

			std::string s_health(std::to_string(health));
			int right, bottom;
			const auto converted_text = std::wstring(s_health.begin(), s_health.end());
			interfaces::surface->get_text_size(render::fonts::verdana_font_small, converted_text.c_str(), right, bottom);
			if (menu.config.animated_health)
			{
				int health_height = health * bbox.h / 100;
				int add_space = bbox.h - health_height;
				render::draw_text_string(bbox.x - 20 - (right / 2), bbox.y + add_space - (bottom / 2) + 3, render::fonts::verdana_font_small, s_health, false, color(255, 255, 255));
			}
			else if (menu.config.health_text)
				render::draw_text_string(bbox.x - 20 - (right / 2), bbox.y - 1.5, render::fonts::verdana_font_small, s_health, false, color(255, 255, 255));
		}

		if (!entity)
			return;

		if (menu.config.name)
		{
			player_info_t info;
			wchar_t buffer[36];
			interfaces::engine->get_player_info(entity->index(), &info);
			std::string print(info.fakeplayer ? std::string("bot ").append(info.name).c_str() : info.name);
			std::transform(print.begin(), print.end(), print.begin(), ::tolower);
			menu.config.name_clr.a = 255;
			if (MultiByteToWideChar(CP_UTF8, 0, print.c_str(), -1, buffer, 36) > 0)
				render::draw_text_wchar(bbox.x + (bbox.w / 2), bbox.y - 13, render::fonts::verdana_font, buffer, true, menu.config.name_clr);
			else
				render::draw_text_string(bbox.x + (bbox.w / 2), bbox.y - 13, render::fonts::verdana_font, print, true, menu.config.name_clr);
		}
		
		if (menu.config.weapon_icon)
		{
			auto wep = entity->active_weapon();
			if (wep)
			{
				auto wep_name = wep->get_icon();
				render::draw_text_string(bbox.x + (bbox.w / 2), bbox.h + bbox.y + 5, render::fonts::weapon_font, wep_name, true, menu.config.wep_icon_clr);
			}
		}

		if (menu.config.gun)
		{
			int height = bbox.h + bbox.y + 2;
			if (menu.config.weapon_icon)
				height = bbox.h + bbox.y + 25;
			auto wep = entity->active_weapon();
			if (wep)
			{
				auto wep_name = wep->get_weapon_data()->m_szWeaponName;
				render::draw_text_string(bbox.x + (bbox.w / 2), height, render::fonts::verdana_font, clean_item_name(wep_name), true, menu.config.wep_name_clr);
			}
		}

		std::vector<std::pair<std::string, color>> flags;

		if (menu.config.flashed && entity->is_flashed())
			flags.push_back(std::pair<std::string, color>("flashed", color(255, 255, 255)));

		if (menu.config.money && entity->money())
			flags.push_back(std::pair<std::string, color>(std::to_string(entity->money()).insert(0, "$"), color(255, 255, 255)));

		if (menu.config.armor && entity->has_helmet() && entity->armor() > 0)
			flags.push_back(std::pair<std::string, color>("HK", color(255, 255, 255)));
		else if (menu.config.armor && !entity->has_helmet() && entity->armor() > 0)
			flags.push_back(std::pair<std::string, color>("K", color(255, 255, 255)));

		//if (menu.config.bomb && entity->has_c4())
		//	flags.push_back(std::pair<std::string, color>("bomb", color(255, 255, 255)));

		if (menu.config.scoped && entity->is_scoped())
			flags.push_back(std::pair<std::string, color>("scoped", color(255, 255, 255)));

		if (menu.config.defuse_kit && entity->has_defuser())
			flags.push_back(std::pair<std::string, color>("defuse kit", color(255, 255, 255)));

		if (menu.config.defusing && entity->is_defusing())
			flags.push_back(std::pair<std::string, color>("defusing", color(255, 255, 255)));

		auto postion = 0;
		for (auto flag : flags)
		{
			int right, bottom;
			const auto converted_text = std::wstring(flag.first.begin(), flag.first.end());
			interfaces::surface->get_text_size(render::fonts::verdana_font_small, converted_text.c_str(), right, bottom);

			render::draw_text_string(bbox.x + bbox.w + 10 + (right/ 2), bbox.y + postion, render::fonts::verdana_font_small, flag.first, true, flag.second);
			postion += 10;
		}
	}
}

bool nightmode_state;
void nightmode()
{
	if (nightmode_state != menu.config.night_mode)
	{
		interfaces::console->get_convar("r_DrawSpecificStaticProp")->set_value(!menu.config.night_mode);

		for (MaterialHandle_t i = interfaces::material_system->first_material(); i != interfaces::material_system->invalid_material_handle(); i = interfaces::material_system->next_material(i))
		{
			i_material* pMaterial = interfaces::material_system->get_material(i);

			if (!pMaterial)
				continue;

			if (strstr(pMaterial->GetTextureGroupName(), "World") || strstr(pMaterial->GetTextureGroupName(), "StaticProp"))
			{
				if (menu.config.night_mode)
				{
					if (strstr(pMaterial->GetTextureGroupName(), "StaticProp"))
					{
						pMaterial->color_modulate(0.4f, 0.4f, 0.4f);
					}
					else
					{
						pMaterial->color_modulate(0.15f, 0.15f, 0.15f);
					}
				}
				else
				{
					pMaterial->color_modulate(1.0f, 1.0f, 1.0f);
				}
			}
		}
		nightmode_state = menu.config.night_mode;
	}
}

void update_spectators()
{
	std::vector<std::string> Name;

	if (!csgo::local_player->is_alive() || !interfaces::engine->is_in_game())
	{
		menu.spectators.clear();
		return;
	}

	for (int i = 1; i < interfaces::globals->max_clients; i++)
	{
		if (csgo::local_player)
		{
			auto entity = reinterpret_cast<player_t*>(interfaces::entity_list->get_client_entity(i));
			if (entity && entity != csgo::local_player && !entity->is_alive())
			{
				player_info_t info;
				interfaces::engine->get_player_info(i, &info);

				DWORD obs = entity->observer_target();
				if (obs)
				{
					player_t* spectator = (player_t*)interfaces::entity_list->get_client_entity_handle(obs);
					if (spectator == csgo::local_player)
					{
						player_info_t info_entity;
						interfaces::engine->get_player_info(i, &info_entity);
						Name.push_back(info_entity.name);
					}
				}
			}
		}
	}

	if (Name.empty())
		Name.clear();

	menu.spectators = Name;
}

void bomb_esp(player_t* entity)
{
	if (entity->client_class()->class_id == class_ids::cplantedc4 && entity->c4_is_ticking() && !entity->c4_is_defused())
	{
		auto explode_time = entity->c4_blow_time() - (interfaces::globals->interval_per_tick * csgo::local_player->get_tick_base()); // 40 - 0
		auto defuse_countdown = entity->c4_defuse_countdown() - (interfaces::globals->interval_per_tick * csgo::local_player->get_tick_base()); // 10 - 0 or 5 - 0

		char time_to_explode[64]; sprintf_s(time_to_explode, "%.1f", explode_time);
		char time_to_defuse[64]; sprintf_s(time_to_defuse, "%.1f", defuse_countdown);

		if (explode_time > 0)
		{
			auto ratio_explode = (explode_time / entity->c4_timer_length()) * menu.screen_x;
			auto lenght = entity->has_defuser() ? 5 : 10;
			auto ratio_defuse = (float)(defuse_countdown / lenght) * (float)menu.screen_x;

			render::draw_filled_rect(0, 0, ratio_explode, 10, color(255, 35, 205));
			render::draw_text_string(menu.screen_x / 2, -2, render::fonts::verdana_font, time_to_explode, true, color(255, 255, 255));

			if (!entity)
				return;

			if (entity->c4_gets_defused() > 0)
			{
				render::draw_filled_rect(0, 10, ratio_defuse, 10, color(38, 255, 205));
				render::draw_text_string(menu.screen_x / 2, 8, render::fonts::verdana_font, time_to_defuse, true, color(255, 255, 255));
			}
		}
	}
}

void dopped_weapons(player_t* entity)
{
	if (entity->is_player())
		return;

	if (!entity->client_class()->class_id)
		return;

	auto model_name = interfaces::model_info->get_model_name(entity->model());
	if (strstr(model_name, "models/weapons/w_") && strstr(model_name, "_dropped.mdl"))
	{
		auto pos = entity->abs_origin();
		vec3_t pos_2d = {};
		if (interfaces::debug_overlay->world_to_screen(pos, pos_2d))
		{
			if (menu.config.dropped_weapon_esp_icon)
				render::draw_text_string(pos_2d.x, pos_2d.y, render::fonts::weapon_font, reinterpret_cast<weapon_t*>(entity)->get_icon(), true, menu.config.dropped_weapon_icon_color_clr);
			else
				render::draw_text_string(pos_2d.x, pos_2d.y, render::fonts::verdana_font_small, clean_item_name(reinterpret_cast<weapon_t*>(entity)->get_weapon_data()->m_szWeaponName), true, color(255, 255, 255));

			if (menu.config.dropped_weapon_esp_distance)
			{
				auto distance = std::to_string(csgo::local_player->origin().distance_to(entity->origin()));
				render::draw_text_string(pos_2d.x, pos_2d.y + 25, render::fonts::verdana_font_small, distance.substr(0, distance.find(".") + 2), true, color(255, 255, 255));
			}
		}
	}
}

void chicken_esp(player_t* entity)
{
	if (entity->is_player())
		return;

	if (entity->client_class()->class_id != class_ids::cchicken)
		return;

	vec3_t pos_2d;
	if (interfaces::debug_overlay->world_to_screen(entity->abs_origin(), pos_2d))
	{
		box bbox;
		if (!get_playerbox(entity, bbox))
			return;

		auto origin = entity->abs_origin();
		origin.x -= (bbox.w / 2);
		origin.y -= (bbox.w / 2);
		origin.z -= bbox.h - 50;
		
		static float rainbow;
		rainbow += 0.005f;

		if (rainbow > 1.f) 
			rainbow = 0.f;

		static color clr;
		clr.random_color((int)(csgo::local_player->get_tick_base() * interfaces::globals->interval_per_tick * 2.f));
		render::draw_3d_cube(10, csgo::local_player->eye_angles(), entity->abs_origin(), clr);
		render::draw_text_string(pos_2d.x, pos_2d.y+15, render::fonts::verdana_font_small, "chicken", true, clr);
	}
}

void features::visuals::render_visuals()
{
	if (!csgo::local_player)
		return;

	if (menu.config.spectator_list)
		update_spectators();

	if (menu.config.show_fov && csgo::local_player->is_alive())
	{
		float radius = tan(DEG2RAD(menu.config.legit_fov / 2)) / tan(DEG2RAD(menu.config.actual_fov / 2)) * (menu.screen_x / 2);
		render::draw_circle(menu.screen_x / 2, menu.screen_y / 2, radius, 50, menu.config.fov_clr);
	}

	if (menu.config.show_spread && csgo::local_player->is_alive())
	{
		auto weapon = csgo::local_player->active_weapon();
		if (weapon)
		{
			if (weapon->get_weapon_data()->m_iWeaponType != WEAPONTYPE_KNIFE && weapon->get_weapon_data()->m_iWeaponType != WEAPONTYPE_GRENADE && weapon->get_weapon_data()->m_iWeaponType != WEAPONTYPE_C4)
			{
				auto spread = weapon->inaccuracy() * 550.f;
				if (spread != 0.f)
					render::draw_circle(menu.screen_x / 2, menu.screen_y / 2, spread, 60, menu.config.spread_clr);
			}
		}
	}
	
	for (int i = 1; i < interfaces::entity_list->get_highest_index(); i++)
	{
		auto entity = reinterpret_cast<player_t*>(interfaces::entity_list->get_client_entity(i));

		if (!entity || entity == csgo::local_player)
			continue;

		if (menu.config.bomb_esp)
			bomb_esp(entity);

		if (menu.config.dropped_weapon_esp)
			dopped_weapons(entity);		

		if (menu.config.dropped_weapon_esp)
			dopped_weapons(entity);

		if (menu.config.chicken_esp)
			chicken_esp(entity);
	}

	nightmode();
}
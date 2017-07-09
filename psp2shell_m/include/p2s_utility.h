/*
	PSP2SHELL
	Copyright (C) 2016, Cpasjuste

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef P2S_UTILITY_H
#define P2S_UTILITY_H

int p2s_reset_running_app();

int p2s_launch_app_by_uri(const char *tid);

SceUID p2s_get_running_app_pid();

SceUID p2s_get_running_app_id();

int p2s_get_running_app_name(char *name);

int p2s_get_running_app_title_id(char *title_id);

#endif // P2S_UTILITY_H

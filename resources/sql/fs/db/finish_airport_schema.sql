-- *****************************************************************************
-- Copyright 2015-2019 Alexander Barthel albar965@mailbox.org
--
-- This program is free software: you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation, either version 3 of the License, or
-- (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this program.  If not, see <http://www.gnu.org/licenses/>.
-- ****************************************************************************/

-- *************************************************************
-- Create application indexes after all post processes
-- *************************************************************

-- Create indexes on columns that are potentially used in searches


create index if not exists idx_airport_name on airport(name);
create index if not exists idx_airport_country on airport(country);
create index if not exists idx_airport_state on airport(state);
create index if not exists idx_airport_city on airport(city);
create index if not exists idx_airport_region on airport(region);

create index if not exists idx_airport_has_avgas on airport(has_avgas);
create index if not exists idx_airport_has_jetfuel on airport(has_jetfuel);

create index if not exists idx_airport_tower_frequency on airport(tower_frequency);

create index if not exists idx_airport_is_closed on airport(is_closed);
create index if not exists idx_airport_is_military on airport(is_military);
create index if not exists idx_airport_is_addon on airport(is_addon);

create index if not exists idx_airport_num_parking_cargo   on airport(num_parking_cargo);
create index if not exists idx_airport_num_parking_mil_cargo on airport(num_parking_mil_cargo);
create index if not exists idx_airport_num_parking_mil_combat  on airport(num_parking_mil_combat);

create index if not exists idx_airport_num_approach on airport(num_approach);

create index if not exists idx_airport_num_runway_hard on airport(num_runway_hard);
create index if not exists idx_airport_num_runway_soft on airport(num_runway_soft);
create index if not exists idx_airport_num_runway_water on airport(num_runway_water);
create index if not exists idx_airport_num_runway_light on airport(num_runway_light);

create index if not exists idx_airport_num_helipad on airport(num_helipad);
create index if not exists idx_airport_longest_runway_length on airport(longest_runway_length);
create index if not exists idx_airport_largest_parking_ramp on airport(largest_parking_ramp);
create index if not exists idx_airport_largest_parking_gate on airport(largest_parking_gate);
create index if not exists idx_airport_rating on airport(rating);
create index if not exists idx_airport_3d on airport(is_3d);

create index if not exists idx_airport_scenery_local_path on airport(scenery_local_path);
create index if not exists idx_airport_bgl_filename on airport(bgl_filename);

create index if not exists idx_airport_left_lonx on airport(left_lonx);
create index if not exists idx_airport_top_laty on airport(top_laty);
create index if not exists idx_airport_right_lonx on airport(right_lonx);
create index if not exists idx_airport_bottom_laty on airport(bottom_laty);

create index if not exists idx_airport_altitude on airport(altitude);
create index if not exists idx_airport_lonx on airport(lonx);
create index if not exists idx_airport_laty on airport(laty);


-- *****************************************************************************
-- Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
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

-- Drop temporary tables
drop table if exists airway_temp;
drop table if exists airway_point;
drop table if exists tmp_waypoint_ap;
drop table if exists tmp_ndb_ap;
drop table if exists tmp_vor_ap;

-- Create indexes on columns that are potentially used in searches
create index if not exists idx_ils_ident on ils(ident);

create index if not exists idx_airway_left_lonx on airway(left_lonx);
create index if not exists idx_airway_top_laty on airway(top_laty);
create index if not exists idx_airway_right_lonx on airway(right_lonx);
create index if not exists idx_airway_bottom_laty on airway(bottom_laty);

create index if not exists idx_airway_from_lonx on airway(from_lonx);
create index if not exists idx_airway_from_laty on airway(from_laty);
create index if not exists idx_airway_to_lonx on airway(to_lonx);
create index if not exists idx_airway_to_laty on airway(to_laty);
create index if not exists idx_airway_name on airway(airway_name);

-------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------
-- Fill overview airport tables

delete from airport_large;

insert into airport_large select
  airport_id, ident, name,
  has_avgas, has_jetfuel, tower_frequency,
  is_closed, is_military, is_addon, is_3d,
  num_runway_hard, num_runway_soft, num_runway_water, num_helipad,
  longest_runway_length, longest_runway_heading, rating,
  left_lonx, top_laty, right_lonx, bottom_laty,
  mag_var,
  lonx, laty
from airport
where longest_runway_length > 8000 and num_runway_hard > 0;

create index if not exists idx_airport_large_ident on airport_large(ident);

create index if not exists idx_airport_large_left_lonx on airport_large(left_lonx);
create index if not exists idx_airport_large_top_laty on airport_large(top_laty);
create index if not exists idx_airport_large_right_lonx on airport_large(right_lonx);
create index if not exists idx_airport_large_bottom_laty on airport_large(bottom_laty);

create index if not exists idx_airport_large_lonx on airport_large(lonx);
create index if not exists idx_airport_large_laty on airport_large(laty);

create index if not exists idx_airport_large_longest_runway_length on airport_large(longest_runway_length);
create index if not exists idx_airport_large_rating on airport_large(rating);
create index if not exists idx_airport_large_is_addon on airport_large(is_addon);
create index if not exists idx_airport_large_is_3d on airport_large(is_3d);

--------------------

delete from airport_medium;

insert into airport_medium select
  airport_id, ident, name,
  has_avgas, has_jetfuel, tower_frequency,
  is_closed, is_military, is_addon, is_3d,
  num_runway_hard, num_runway_soft, num_runway_water, num_helipad,
  longest_runway_length, longest_runway_heading, rating,
  left_lonx, top_laty, right_lonx, bottom_laty,
  mag_var,
  lonx, laty
from airport
where longest_runway_length > 4000;

create index if not exists idx_airport_medium_ident on airport_medium(ident);

create index if not exists idx_airport_medium_left_lonx on airport_medium(left_lonx);
create index if not exists idx_airport_medium_top_laty on airport_medium(top_laty);
create index if not exists idx_airport_medium_right_lonx on airport_medium(right_lonx);
create index if not exists idx_airport_medium_bottom_laty on airport_medium(bottom_laty);

create index if not exists idx_airport_medium_lonx on airport_medium(lonx);
create index if not exists idx_airport_medium_laty on airport_medium(laty);

create index if not exists idx_airport_medium_longest_runway_length on airport_medium(longest_runway_length);
create index if not exists idx_airport_medium_rating on airport_medium(rating);
create index if not exists idx_airport_medium_is_addon on airport_medium(is_addon);
create index if not exists idx_airport_medium_is_3d on airport_medium(is_3d);

-------------------------------------------------------------------------------------------

create index if not exists idx_taxi_path_type on taxi_path(type);

create index if not exists idx_route_node_airway_type on route_node_airway(type);
create index if not exists idx_route_node_airway_nav_id on route_node_airway(nav_id);
create index if not exists idx_route_node_airway_lonx on route_node_airway(lonx);
create index if not exists idx_route_node_airway_laty on route_node_airway(laty);

create index if not exists idx_route_edge_airway_type on route_edge_airway(type);
create index if not exists idx_route_edge_airway_dir on route_edge_airway(direction);
create index if not exists idx_route_edge_airway_min_alt on route_edge_airway(minimum_altitude);
create index if not exists idx_route_edge_airway_max_alt on route_edge_airway(maximum_altitude);
create index if not exists idx_route_edge_airway_from_node_type on route_edge_airway(from_node_type);
create index if not exists idx_route_edge_airway_to_node_type on route_edge_airway(to_node_type);


create index if not exists idx_route_node_radio_nav_id on route_node_radio(nav_id);
create index if not exists idx_route_node_radio_lonx on route_node_radio(lonx);
create index if not exists idx_route_node_radio_laty on route_node_radio(laty);
create index if not exists idx_route_node_radio_type on route_node_radio(type);
create index if not exists idx_route_node_radio_range on route_node_radio(range);
create index if not exists idx_route_edge_radio_from_node_type on route_edge_radio(from_node_type);
create index if not exists idx_route_edge_radio_to_node_type on route_edge_radio(to_node_type);

create index if not exists idx_route_edge_radio_from_node_id on route_edge_radio(from_node_id);
create index if not exists idx_route_edge_radio_to_node_id on route_edge_radio(to_node_id);

-- Collect table and index statistics
-- Disabled for now since it can cause queries to freeze
-- analyze;

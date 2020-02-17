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

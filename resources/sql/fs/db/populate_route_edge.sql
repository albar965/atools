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
-- Fill the route_edge_airway table with airway segments.
-- route_edge_radio is filled in class RouteEdgeWriter
-- *************************************************************

-- Populate route_edge_airway table with airway segments -----------------------------------
delete from route_edge_airway;

insert into route_edge_airway (airway_id, from_node_id, from_node_type, to_node_id, to_node_type, type,
                               direction, minimum_altitude, maximum_altitude, airway_name)
select a.airway_id, n1.node_id as from_node_id,
  n1.type as from_node_type,
  n2.node_id as to_node_id,
  n2.type as to_node_type,
case
  when a.airway_type = 'V' then 5
  when a.airway_type = 'J' then 6
  when a.airway_type = 'B' then 7
  else 0
end as type,
case
  -- 0 = both, 1 = forward only (from -> to), 2 = backward only (to -> from)
  -- when a.direction = 'N' then 0
  when a.direction = 'F' then 1
  when a.direction = 'B' then 2
  else 0
end as direction,
a.minimum_altitude,
a.maximum_altitude,
a.airway_name as airway_name
from airway a
join route_node_airway n1 on a.from_waypoint_id = n1.nav_id
join route_node_airway n2 on a.to_waypoint_id = n2.nav_id;



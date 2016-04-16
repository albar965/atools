-- *****************************************************************************
-- Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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

-- Populate route_node table with waypoints -----------------------------------

insert into route_node (nav_id, type, num_victor_airway, num_jet_airway, lonx, laty)
select waypoint_id as nav_id, 4 as type, num_victor_airway, num_jet_airway, lonx, laty
from waypoint
where num_victor_airway > 0 or num_jet_airway > 0;

-- Populate route_edge table with airways -----------------------------------

insert into route_edge (from_node_id,  to_node_id, type, minimum_altitude)
select n1.node_id as from_node_id, n2.node_id as to_node_id,
case
  when a.airway_type = 'VICTOR' then 1
  when a.airway_type = 'JET' then 2
  when a.airway_type = 'BOTH' then 2
  else 0
end as type,
a.minimum_altitude
from airway a join route_node n1 on a.from_waypoint_id = n1.nav_id
join route_node n2 on a.to_waypoint_id = n2.nav_id
where n1.type = 4 and n2.type = 4;



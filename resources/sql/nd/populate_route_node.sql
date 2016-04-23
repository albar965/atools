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

----------------------------------------------------------------
-- Populate route_node_radio table with VOR and NDB -----------------------------------

delete from route_node_radio;

insert into route_node_radio (nav_id, type, range, lonx, laty)
select vor_id as nav_id,
case
  when dme_only = 1 then 2
  when dme_altitude is null then 0
  else 1
end as type,
(range * 1852.216) as range, lonx, laty
from vor;

insert into route_node_radio (nav_id, type, range, lonx, laty)
select ndb_id as nav_id, 3 as type, (range  * 1852.216) as range, lonx, laty
from ndb;

create index if not exists idx_route_node_radio_lonx on route_node_radio(lonx);
create index if not exists idx_route_node_radio_laty on route_node_radio(laty);

-- Populate route_node_airway table with waypoints -----------------------------------

insert into route_node_airway (nav_id, num_victor_airway, num_jet_airway, lonx, laty)
select waypoint_id as nav_id, num_victor_airway, num_jet_airway, lonx, laty
from waypoint
where num_victor_airway > 0 or num_jet_airway > 0;

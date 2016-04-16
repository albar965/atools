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
-- Populate route_node table -----------------------------------

delete from route_node;

insert into route_node (nav_id, type, range, lonx, laty)
select vor_id as nav_id,
case
  when dme_only = 1 then 2
  when dme_altitude is null then 0
  else 1
end as type,
range, lonx, laty
from vor;

insert into route_node (nav_id, type, range, lonx, laty)
select ndb_id as nav_id, 3 as type, range, lonx, laty
from ndb;

insert into route_node (nav_id, type, num_victor_airway, num_jet_airway, lonx, laty)
select waypoint_id as nav_id, 4 as type, num_victor_airway, num_jet_airway, lonx, laty
from waypoint
where num_victor_airway > 0 or num_jet_airway > 0;

create index if not exists idx_route_node_nav_id on route_node(nav_id);
create index if not exists idx_route_node_type on route_node(type);
create index if not exists idx_route_node_num_victor on route_node(num_victor_airway);
create index if not exists idx_route_node_num_jet on route_node(num_jet_airway);

create index if not exists idx_route_node_range on route_node(range);
create index if not exists idx_route_node_lonx on route_node(lonx);
create index if not exists idx_route_node_laty on route_node(laty);

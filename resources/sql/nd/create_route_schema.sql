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

drop table if exists route_node;

create table route_node
(
  node_id integer primary key,
  nav_id integer not null,
  type integer not null, -- 0 = VOR, 1 = VORDME, 2 = DME, 3 = NDB, 4 = WAYPOINT
  range integer,
  num_victor_airway integer,
  num_jet_airway integer,
  lonx double not null,
  laty double not null
);


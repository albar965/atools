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

drop table if exists route_node_radio;

create table route_node_radio
(
  node_id integer primary key,
  nav_id integer not null,
  type integer not null, -- 0 = VOR, 1 = VORDME, 2 = DME, 3 = NDB
  range integer,
  lonx double not null,
  laty double not null
);

-- **************************************************

drop table if exists route_edge_radio;

create table route_edge_radio
(
  edge_id integer primary key,
  from_node_id integer not null,
  from_node_type integer not null,
  to_node_id integer not null,
  to_node_type integer not null,
  distance integer not null,
foreign key(from_node_id) references route_node_radio(node_id),
foreign key(to_node_id) references route_node_radio(node_id)
);

create index if not exists idx_route_edge_radio_from_node_id on route_edge_radio(from_node_id);
create index if not exists idx_route_edge_radio_to_node_id on route_edge_radio(to_node_id);


-- **************************************************

drop table if exists route_node_airway;

create table route_node_airway
(
  node_id integer primary key,
  nav_id integer not null,
  type integer, -- 4 = Victor, 5 = Jet, 6 = Both
  lonx double not null,
  laty double not null
);

-- **************************************************

drop table if exists route_edge_airway;

create table route_edge_airway
(
  edge_id integer primary key,
  airway_id integer not null,
  from_node_id integer not null,
  from_node_type integer not null,
  to_node_id integer not null,
  to_node_type integer not null,
  type integer, -- 4 = Victor, 5 = Jet, 6 = Both
  minimum_altitude integer,
foreign key(airway_id) references airway(airway_id),
foreign key(from_node_id) references route_node_airway(node_id),
foreign key(to_node_id) references route_node_airway(node_id)
);

create index if not exists idx_route_edge_airway_id on route_edge_airway(airway_id);
create index if not exists idx_route_edge_airway_from_node_id on route_edge_airway(from_node_id);
create index if not exists idx_route_edge_airway_to_node_id on route_edge_airway(to_node_id);

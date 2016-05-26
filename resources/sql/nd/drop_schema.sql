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

-- Order is important to avoid fk conflicts

drop view if exists v_airport_delete;
drop view if exists v_airport_approach;
drop view if exists v_airport_com;
drop view if exists v_airport_runway;
drop view if exists v_airport_scenery;

drop table if exists route_edge_radio;
drop table if exists route_edge_airway;
drop table if exists route_node_radio;
drop table if exists route_node_airway;
drop table if exists nav_search;

drop table if exists airway;
drop table if exists airway_point;
drop table if exists ils;
drop table if exists marker;
drop table if exists ndb;
drop table if exists vor;
drop table if exists waypoint;

drop table if exists delete_airport;
drop table if exists parking;
drop table if exists taxi_path;
drop table if exists fence;
drop table if exists apron_light;
drop table if exists apron;
drop table if exists start;
drop table if exists helipad;
drop table if exists com;

drop table if exists transition_leg;
drop table if exists approach_leg;
drop table if exists transition;
drop table if exists approach;

drop table if exists runway;
drop table if exists runway_end;

drop table if exists airport;
drop table if exists airport_medium;
drop table if exists airport_large;

drop table if exists boundary_line;
drop table if exists boundary;

drop table if exists bgl_file;
drop table if exists scenery_area;
drop table if exists metadata;


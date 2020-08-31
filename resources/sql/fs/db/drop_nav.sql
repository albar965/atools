-- *****************************************************************************
-- Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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
-- Drop all tables used for navigation aids
-- *************************************************************

-- Order is important to avoid fk conflicts

-- drop nav
drop table if exists airway;
drop table if exists tmp_airway_point;
drop table if exists tmp_waypoint;
drop table if exists airway_temp;
drop table if exists tmp_waypoint;
drop table if exists tmp_waypoint_dfd;
drop table if exists ils;
drop table if exists marker;
drop table if exists ndb;
drop table if exists vor;
drop table if exists waypoint;
drop table if exists boundary;
drop table if exists mora_grid;


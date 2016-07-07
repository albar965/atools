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

-- *************************************************************
-- Remove any duplicates that are resulting from buggy BGL files
-- *************************************************************

delete from ndb where ndb_id in (
select n1.ndb_id
from ndb n1 join ndb n2 on n1.ident = n2.ident and n1.frequency = n2.frequency and
  n1.type = n2.type and n1.region = n2.region and n1.lonx = n2.lonx and n1.laty = n2.laty
where n1.ndb_id < n2.ndb_id and n1.airport_id is null);

delete from ndb where ndb_id in (
select n1.ndb_id
from ndb n1 join ndb n2 on n1.ident = n2.ident and n1.frequency = n2.frequency and
  n1.type = n2.type and n1.region = n2.region and n1.lonx = n2.lonx and n1.laty = n2.laty
where n1.ndb_id > n2.ndb_id and n1.airport_id is null);

/*
delete from waypoint where waypoint_id in (
select n1.waypoint_id from waypoint n1 join waypoint n2 on n1.ident = n2.ident and
  n1.type = n2.type and n1.region = n2.region and n1.lonx = n2.lonx and n1.laty = n2.laty
where n1.waypoint_id < n2.waypoint_id and n1.airport_id is null and
n1.waypoint_id not in (select waypoint_id from airway_point)) ;

delete from waypoint where waypoint_id in (
select n1.waypoint_id from waypoint n1 join waypoint n2 on n1.ident = n2.ident and
  n1.type = n2.type and n1.region = n2.region and n1.lonx = n2.lonx and n1.laty = n2.laty
where n1.waypoint_id > n2.waypoint_id and n1.airport_id is null and
n1.waypoint_id not in (select waypoint_id from airway_point)) ;
*/

delete from marker where marker_id in (
select n1.marker_id
from marker n1 join marker n2 on n1.type = n2.type and n1.heading = n2.heading and
  n1.lonx = n2.lonx and n1.laty = n2.laty
where n1.marker_id < n2.marker_id );


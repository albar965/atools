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
-- Remove any duplicates that are resulting from add-on BGL files.
-- Some add-on airport BGL files contain additional navaids that are
-- not (and cannot be) covered by the delete processor. These will
-- be removed here by keeping only the duplicate with the highest id.
-- This means stock/default/oldest are removed and add-on are kept.
-- The manhattan distance using deg is sufficient for a crude distance estimation
-- *************************************************************

-- Print duplicate airports to the log
select airport_id, ident, name, scenery_local_path, bgl_filename from airport
where airport_id not in (select max(airport_id) from airport group by ident);

-- Delete duplicate airports with the lowest id (stock)
delete from airport
where airport_id not in (select max(airport_id) from airport group by ident);

delete from approach where airport_id not in (select airport_id from airport);
delete from approach_leg where approach_id not in (select approach_id from approach);
delete from transition where approach_id not in (select approach_id from approach);
delete from transition_leg where transition_id not in (select transition_id from transition);


-- Delete duplicate NDBs
delete from ndb where ndb_id in (
select distinct w1.ndb_id
from ndb w1
join ndb w2 on w1.ident = w2.ident and  w1.frequency = w2.frequency and  w1.region = w2.region
where
w1.ndb_id < w2.ndb_id and
(abs(w1.lonx - w2.lonx) + abs(w1.laty - w2.laty)) < 0.1);

-- Delete duplicate VORs
delete from vor where vor_id in (
select distinct w1.vor_id
from vor w1
join vor w2 on w1.ident = w2.ident and  w1.frequency = w2.frequency and
  w1.type = w2.type and  w1.region = w2.region and  w1.dme_only = w2.dme_only and
 (w1.dme_altitude is null) = (w2.dme_altitude is null)
where
w1.vor_id < w2.vor_id and
(abs(w1.lonx - w2.lonx) + abs(w1.laty - w2.laty)) < 0.1);

-- Delete duplicate markers
delete from marker where marker_id in (
select distinct w1.marker_id
from marker w1
join marker w2 on w1.heading = w2.heading and w1.type = w2.type
where
w1.marker_id < w2.marker_id and
(abs(w1.lonx - w2.lonx) + abs(w1.laty - w2.laty)) < 0.01);

-- Delete duplicate ils same name and same type
delete from ils where ils_id in (
select distinct w1.ils_id
from ils w1
join ils w2 on w1.ident = w2.ident and w1.name = w2.name
where
w1.ils_id < w2.ils_id and
(abs(w1.lonx - w2.lonx) + abs(w1.laty - w2.laty)) < 0.1);

-- Delete duplicate ils same name and close by
delete from ils where ils_id in (
select distinct w1.ils_id
from ils w1
join ils w2 on w1.ident = w2.ident
where
w1.ils_id < w2.ils_id and
(abs(w1.lonx - w2.lonx) + abs(w1.laty - w2.laty)) < 0.01);

-- Delete all duplicate waypoints that are close together having same name, region and type
-- 0.1 deg manhattan distance about 6 nm at the equator
delete from waypoint where waypoint_id in (
select distinct w1.waypoint_id
from waypoint w1
join waypoint w2 on  w1.ident = w2.ident and w1.region = w2.region and w1.type = w2.type
where
w1.waypoint_id < w2.waypoint_id and
(abs(w1.lonx - w2.lonx) + abs(w1.laty - w2.laty)) < 0.1);

delete from airway_point
where airway_point_id not in (
  select max(airway_point_id)
  from airway_point
  group by name, type, mid_type, mid_ident, mid_region,
      next_type, next_ident, next_region, previous_type, previous_ident, previous_region
);



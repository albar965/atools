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
-- Remove any duplicates that are resulting from add-on BGL files.
-- Some add-on airport BGL files contain additional navaids that are
-- not (and cannot be) covered by the delete processor. These will
-- be removed here by keeping only the duplicate with the highest id.
-- This means stock/default are removed and add-on are kept.
-- *************************************************************

-- Delete duplicate NDBs
delete from ndb where ndb_id in (
select distinct w1.ndb_id
from ndb w1
join ndb w2 on w1.ident = w2.ident and  w1.frequency = w2.frequency and  w1.region = w2.region
where
w1.ndb_id < w2.ndb_id and
(abs(w1.lonx - w2.lonx) + abs(w1.laty - w2.laty)) < 0.01);

-- Delete duplicate VORs
delete from vor where vor_id in (
select distinct w1.vor_id
from vor w1
join vor w2 on w1.ident = w2.ident and  w1.frequency = w2.frequency and
  w1.type = w2.type and  w1.region = w2.region and  w1.dme_only = w2.dme_only and
 (w1.dme_altitude is null) = (w2.dme_altitude is null)
where
w1.vor_id < w2.vor_id and
(abs(w1.lonx - w2.lonx) + abs(w1.laty - w2.laty)) < 0.01);

-- Delete duplicate markers
delete from marker where marker_id in (
select distinct w1.marker_id
from marker w1
join marker w2 on w1.heading = w2.heading and w1.type = w2.type
where
w1.marker_id < w2.marker_id and
(abs(w1.lonx - w2.lonx) + abs(w1.laty - w2.laty)) < 0.01);

-- Delete all duplicate waypoints that are close together
-- 0.1 deg manhattan distance about 6 nm at the equator
delete from waypoint where waypoint_id in (
select distinct w1.waypoint_id
from waypoint w1
join waypoint w2 on  w1.ident = w2.ident and w1.region = w2.region --and w1.type = w2.type
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

update airway_point set waypoint_id = (
select w.waypoint_id from waypoint w where mid_type = w.type and mid_ident = w.ident and mid_region = w.region
);

update waypoint set num_victor_airway = (
select count(1) from airway_point ap
where ap.waypoint_id = waypoint.waypoint_id and ap.type in ('VICTOR', 'BOTH'));

update waypoint set num_jet_airway = (
select count(1) from airway_point ap
where ap.waypoint_id = waypoint.waypoint_id and ap.type in ('JET', 'BOTH'));

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
-- X-Plane specific
-- *************************************************************

-- Create the missing NDB waypoints for airway intersections
-- Needed to get a closed network
insert into waypoint (file_id, ident, region, artificial, type, num_victor_airway, num_jet_airway, mag_var, lonx, laty)
select file_id, ident, region, 1 as artificial, 'N' as type, 0 as num_victor_airway, 0 as num_jet_airway, mag_var, lonx, laty from (
  select previous_ident as ident, previous_region as region, ndb.file_id, ndb.lonx, ndb.laty, ndb.mag_var
  from airway_temp join ndb on previous_ident = ndb.ident and previous_region = ndb.region
  where previous_type = 2
union
  select next_ident as ident, next_region as region, ndb.file_id, ndb.lonx, ndb.laty, ndb.mag_var
  from airway_temp join ndb on next_ident = ndb.ident and next_region = ndb.region
  where next_type = 2);

-- Create the missing VOR waypoints for airway intersections
insert into waypoint (file_id, ident, region, artificial, type, num_victor_airway, num_jet_airway, mag_var, lonx, laty)
select file_id, ident, region, 1 as artificial, 'V' as type, 0 as num_victor_airway, 0 as num_jet_airway, mag_var, lonx, laty from (
  select previous_ident as ident, previous_region as region, vor.file_id, vor.lonx, vor.laty, vor.mag_var
  from airway_temp join vor on previous_ident = vor.ident and previous_region = vor.region
  where previous_type = 3
union
  select next_ident as ident, next_region as region, vor.file_id, vor.lonx, vor.laty, vor.mag_var
  from airway_temp join vor on next_ident = vor.ident and next_region = vor.region
  where next_type = 3);

-- Combine duplicate airway segments with type 1 and 2 to type 3 (both)
-- Set first of each victor/jet airway pair to both
update airway_temp set type = 3 where airway_temp_id in (
select min(airway_temp_id)
from airway_temp
group by name, direction, previous_type, previous_ident, previous_region, next_type, next_ident, next_region
having count(1) = 2);

-- Delete the second of the victor/jet airway pairs
delete from airway_temp where airway_temp_id in (
select max(airway_temp_id)
from airway_temp
group by name, direction, previous_type, previous_ident, previous_region, next_type, next_ident, next_region
having count(1) = 2);

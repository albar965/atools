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
-- Update navigation references for waypoint -------------------
-- *************************************************************

update waypoint set nav_id =
(
  select v.vor_id
  from vor v
  where waypoint.type = 'V' and waypoint.ident = v.ident and waypoint.region = v.region and
  (abs(v.lonx - waypoint.lonx) + abs(v.laty - waypoint.laty)) < 0.01
)
where waypoint.type = 'V';

update waypoint set nav_id =
(
  select n.ndb_id
  from ndb n
  where waypoint.type = 'N' and waypoint.ident = n.ident and waypoint.region = n.region and
  (abs(n.lonx - waypoint.lonx) + abs(n.laty - waypoint.laty)) < 0.01
)
where waypoint.type = 'N';

-- Update airway counts ------------------------------------------------------------
update waypoint set num_victor_airway = (
select count(1) from airway ap
where (ap.from_waypoint_id = waypoint.waypoint_id or ap.to_waypoint_id = waypoint.waypoint_id) and
       ap.airway_type in ('V', 'B'));

update waypoint set num_jet_airway = (
select count(1) from airway ap
where (ap.from_waypoint_id = waypoint.waypoint_id or ap.to_waypoint_id = waypoint.waypoint_id) and
       ap.airway_type in ('J', 'B'));

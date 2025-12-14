-- *****************************************************************************
-- Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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
-- Remove any duplicates that are resulting from BGL files.
-- *************************************************************

-- Delete all waypoint duplicates not having airways and no airport ident attached
delete from waypoint where waypoint_id in (
select distinct w1.waypoint_id
from waypoint w1
join waypoint w2 on  w1.ident = w2.ident
where
w1.waypoint_id < w2.waypoint_id and w1.num_victor_airway = 0 and w1.num_jet_airway = 0 and w1.airport_ident is null and
(abs(w1.lonx - w2.lonx) + abs(w1.laty - w2.laty)) < 0.00000001 order by w1.ident);

-- Delete remaining other waypoint duplicates not having airways and no airport ident attached
delete from waypoint where waypoint_id in (
select distinct w1.waypoint_id
from waypoint w1
join waypoint w2 on  w1.ident = w2.ident
where
w1.waypoint_id > w2.waypoint_id and w1.num_victor_airway = 0 and w1.num_jet_airway = 0 and w1.airport_ident is null and
(abs(w1.lonx - w2.lonx) + abs(w1.laty - w2.laty)) < 0.00000001 order by w1.ident);

-- Delete all waypoint duplicates having the same airport ident not having airways attached
delete from waypoint where waypoint_id in (
select distinct w1.waypoint_id
from waypoint w1
join waypoint w2 on  w1.ident = w2.ident and w1.airport_ident = w2.airport_ident
where
w1.waypoint_id < w2.waypoint_id and w1.num_victor_airway = 0 and w1.num_jet_airway = 0 and
(abs(w1.lonx - w2.lonx) + abs(w1.laty - w2.laty)) < 0.00000001 order by w1.ident);

-- Delete remaining other waypoint duplicates having the same airport ident not having airways attached
delete from waypoint where waypoint_id in (
select distinct w1.waypoint_id
from waypoint w1
join waypoint w2 on  w1.ident = w2.ident and w1.airport_ident = w2.airport_ident
where
w1.waypoint_id > w2.waypoint_id and w1.num_victor_airway = 0 and w1.num_jet_airway = 0 and
(abs(w1.lonx - w2.lonx) + abs(w1.laty - w2.laty)) < 0.00000001 order by w1.ident);

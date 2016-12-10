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

-- Delete stock NDB is there is an add-on duplicate
delete from ndb
where ndb_id not in (
  select max(ndb_id)
  from ndb
  group by ident, frequency, region, lonx, laty
);

-- Delete stock VOR is there is an add-on duplicate
delete from vor
where vor_id not in (
  select max(vor_id)
  from vor
  group by ident, frequency, type, region, lonx, laty
);

-- Delete stock marker is there is an add-on duplicate
delete from marker
where marker_id not in (
  select max(marker_id)
  from marker
  group by type, heading, lonx, laty
);

-- Delete duplicate add-on waypoints since the stock ones keep the airway connections
delete from waypoint
where waypoint_id not in (
  select max(waypoint_id)
  from waypoint
  group by ident, type, region, lonx, laty
) and num_victor_airway = 0 and num_jet_airway = 0;



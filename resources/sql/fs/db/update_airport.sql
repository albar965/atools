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
-- Update airport region
-- *************************************************************

update airport set region = (
  select w.region from waypoint w where airport.airport_id = w.airport_id)
where airport.region is null;

update airport set region = (
  select v.region from vor v where airport.airport_id = v.airport_id)
where airport.region is null;

update airport set region = (
  select n.region from ndb n where airport.airport_id = n.airport_id)
where airport.region is null;


-- *****************************************************************************
-- Copyright 2015-2021 Alexander Barthel alex@littlenavmap.org
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
-- Update airport references for navaids -------------------
-- *************************************************************

update waypoint set airport_id = (
  select a.airport_id from airport a where waypoint.airport_ident = a.ident
);

update ndb set airport_id = (
  select a.airport_id from airport a where ndb.airport_ident = a.ident
);

update vor set airport_id = (
select a.airport_id from airport a where vor.airport_ident = a.ident
);


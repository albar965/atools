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
-- Update ILS runway ids
-- *************************************************************

update ils set loc_runway_end_id = (
  select runway_end_id
  from runway_end e
  join runway r on e.runway_end_id = r.primary_end_id
  join airport a on r.airport_id = a.airport_id
  where e.name = ils.loc_runway_name and a.ident = ils.loc_airport_ident
) where loc_runway_end_id is null;

update ils set loc_runway_end_id = (
  select runway_end_id
  from runway_end e
  join runway r on e.runway_end_id = r.secondary_end_id
  join airport a on r.airport_id = a.airport_id
  where e.name = ils.loc_runway_name and a.ident = ils.loc_airport_ident
) where loc_runway_end_id is null;

-- *****************************************************************************
-- Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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
-- Update runway references for approaches where missing
-- *************************************************************

update approach set runway_end_id = (
  select runway_end_id
  from airport a
  join runway r on r.airport_id = a.airport_id
  join runway_end e on r.primary_end_id = e.runway_end_id
  where e.name = approach.runway_name and a.ident = approach.airport_ident
) where approach.runway_end_id is null;

update approach set runway_end_id = (
  select runway_end_id
  from airport a
  join runway r on r.airport_id = a.airport_id
  join runway_end e on r.secondary_end_id = e.runway_end_id
  where e.name = approach.runway_name and a.ident = approach.airport_ident
) where approach.runway_end_id is null;


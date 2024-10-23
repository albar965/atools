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
-- Update ILS runway ids - only FSX and P3D
-- *************************************************************

-- Set runway end reference
update ils set loc_runway_end_id = (
  select runway_end_id
  from runway_end e
  where e.ils_ident = ils.ident and
  (abs(e.lonx - ils.lonx) + abs(e.laty - ils.laty)) < 0.5
);

update ils set loc_airport_ident = null;

-- Update airport ident according to runway end
update ils set loc_airport_ident = (
  select a.ident from airport a join runway r on a.airport_id = r.airport_id
  where r.primary_end_id = ils.loc_runway_end_id
  union
  select a.ident from airport a join runway r on a.airport_id = r.airport_id
  where r.secondary_end_id = ils.loc_runway_end_id
) where loc_airport_ident is null;

update ils set loc_runway_name = (
select e.name from runway_end e where ils.loc_runway_end_id = e.runway_end_id
);


-- *****************************************************************************
-- Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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
-- Update runway references for approaches
-- *************************************************************

create index if not exists idx_approach_fix_type on approach(fix_type);
create index if not exists idx_approach_fix_ident on approach(fix_ident);
create index if not exists idx_approach_fix_region on approach(fix_region);
create index if not exists idx_approach_fix_fix_airport_ident on approach(fix_airport_ident);

update approach set runway_end_id = (
  select runway_end_id
  from airport a
  join runway r on r.airport_id = a.airport_id
  join runway_end e on r.primary_end_id = e.runway_end_id
  where e.name = approach.runway_name and a.ident = approach.airport_ident
);

update approach set runway_end_id = (
  select runway_end_id
  from airport a
  join runway r on r.airport_id = a.airport_id
  join runway_end e on r.secondary_end_id = e.runway_end_id
  where e.name = approach.runway_name and a.ident = approach.airport_ident
) where approach.runway_end_id is null;

-- *************************************************************
-- Update navaid references for approach
-- *************************************************************

update approach set fix_nav_id =
(
  select v.vor_id
  from vor v
  where approach.fix_type = 'V' and approach.fix_ident = v.ident and approach.fix_region = v.region
) where approach.fix_type = 'V' and approach.fix_nav_id is null;

update approach set fix_nav_id =
(
  select n.ndb_id
  from ndb n
  where approach.fix_type = 'N' and approach.fix_ident = n.ident and approach.fix_region = n.region
) where approach.fix_type = 'N' and approach.fix_nav_id is null;

update approach set fix_nav_id =
(
  select w.waypoint_id
  from waypoint w
  where approach.fix_type = 'W' and approach.fix_ident = w.ident and approach.fix_region = w.region
) where approach.fix_type = 'W' and approach.fix_nav_id is null;

-- Terminals -----------------------------------
update approach set fix_nav_id =
(
  select n.ndb_id
  from ndb n join airport a on n.airport_id = a.airport_id
  where approach.fix_type = 'TN' and approach.fix_ident = n.ident and
  approach.fix_region = n.region and a.ident = approach.fix_airport_ident
) where approach.fix_type = 'TN' and approach.fix_nav_id is null;

update approach set fix_nav_id =
(
  select w.waypoint_id
  from waypoint w join airport a on w.airport_id = a.airport_id
  where approach.fix_type = 'TW' and approach.fix_ident = w.ident and
  approach.fix_region = w.region and a.ident = approach.fix_airport_ident
) where approach.fix_type = 'TW' and approach.fix_nav_id is null;


drop index if exists idx_approach_fix_type;
drop index if exists idx_approach_fix_ident;
drop index if exists idx_approach_fix_regi;
drop index if exists idx_approach_fix_fix_airport_ident;

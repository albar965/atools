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


create index if not exists idx_approach_leg_fix_type on approach_leg(fix_type);
create index if not exists idx_approach_leg_fix_ident on approach_leg(fix_ident);
create index if not exists idx_approach_leg_fix_region on approach_leg(fix_region);
create index if not exists idx_approach_leg_fix_fix_airport_ident on approach_leg(fix_airport_ident);

create index if not exists idx_approach_leg_recommended_fix_type on approach_leg(recommended_fix_type);
create index if not exists idx_approach_leg_recommended_fix_ident on approach_leg(recommended_fix_ident);
create index if not exists idx_approach_leg_recommended_fix_region on approach_leg(recommended_fix_region);


----------------------------------------------------------------
-- Update navigation references for approach legs --------------

update approach_leg set fix_nav_id =
(
  select v.vor_id
  from vor v
  where approach_leg.fix_type = 'V' and approach_leg.fix_ident = v.ident and approach_leg.fix_region = v.region
) where approach_leg.fix_type = 'V' and approach_leg.fix_nav_id is null;

update approach_leg set fix_nav_id =
(
  select n.ndb_id
  from ndb n
  where approach_leg.fix_type = 'N' and approach_leg.fix_ident = n.ident and approach_leg.fix_region = n.region
) where approach_leg.fix_type = 'N' and approach_leg.fix_nav_id is null;

update approach_leg set fix_nav_id =
(
  select w.waypoint_id
  from waypoint w
  where approach_leg.fix_type = 'W' and approach_leg.fix_ident = w.ident and approach_leg.fix_region = w.region
) where approach_leg.fix_type = 'W' and approach_leg.fix_nav_id is null;

-- Terminals -----------------------------------
update approach_leg set fix_nav_id =
(
  select n.ndb_id
  from ndb n join airport a on n.airport_id = a.airport_id
  where approach_leg.fix_type = 'TN' and approach_leg.fix_ident = n.ident and
  approach_leg.fix_region = n.region and a.ident = approach_leg.fix_airport_ident
) where approach_leg.fix_type = 'TN' and approach_leg.fix_nav_id is null;

update approach_leg set fix_nav_id =
(
  select w.waypoint_id
  from waypoint w join airport a on w.airport_id = a.airport_id
  where approach_leg.fix_type = 'TW' and approach_leg.fix_ident = w.ident and
  approach_leg.fix_region = w.region and a.ident = approach_leg.fix_airport_ident
) where approach_leg.fix_type = 'TW' and approach_leg.fix_nav_id is null;

---------------------------------------------------------------
-- Update navigation references for approach legs --------------

update approach_leg set recommended_fix_nav_id =
(
  select v.vor_id
  from vor v
  where approach_leg.recommended_fix_type = 'V' and approach_leg.recommended_fix_ident = v.ident and
  approach_leg.recommended_fix_region = v.region
) where approach_leg.recommended_fix_type = 'V' and approach_leg.recommended_fix_nav_id is null;

update approach_leg set recommended_fix_nav_id =
(
  select n.ndb_id
  from ndb n
  where approach_leg.recommended_fix_type = 'N' and approach_leg.recommended_fix_ident = n.ident and
  approach_leg.recommended_fix_region = n.region
) where approach_leg.recommended_fix_type = 'N' and approach_leg.recommended_fix_nav_id is null;

update approach_leg set recommended_fix_nav_id =
(
  select w.waypoint_id
  from waypoint w
  where approach_leg.recommended_fix_type = 'W' and approach_leg.recommended_fix_ident = w.ident and
  approach_leg.recommended_fix_region = w.region
) where approach_leg.recommended_fix_type = 'W' and approach_leg.recommended_fix_nav_id is null;

-- Terminals -----------------------------------
update approach_leg set recommended_fix_nav_id =
(
  select n.ndb_id
  from ndb n join airport a on n.airport_id = a.airport_id
  where approach_leg.recommended_fix_type = 'TN' and approach_leg.recommended_fix_ident = n.ident and
  approach_leg.recommended_fix_region = n.region and a.ident = approach_leg.fix_airport_ident
) where approach_leg.recommended_fix_type = 'TN' and approach_leg.recommended_fix_nav_id is null;

update approach_leg set recommended_fix_nav_id =
(
  select w.waypoint_id
  from waypoint w join airport a on w.airport_id = a.airport_id
  where approach_leg.recommended_fix_type = 'TW' and approach_leg.recommended_fix_ident = w.ident and
  approach_leg.recommended_fix_region = w.region and a.ident = approach_leg.fix_airport_ident
) where approach_leg.recommended_fix_type = 'TW' and approach_leg.recommended_fix_nav_id is null;

update approach_leg set recommended_fix_nav_id =
(
  select i.ils_id
  from ils i, approach a
  where approach_leg.recommended_fix_type = 'L' and approach_leg.recommended_fix_ident = i.ident and
  i.loc_airport_ident = a.airport_ident and approach_leg.approach_id = a.approach_id
) where approach_leg.recommended_fix_type = 'L' and approach_leg.recommended_fix_nav_id is null;


drop index if exists idx_approach_leg_fix_type;
drop index if exists idx_approach_leg_fix_ident;
drop index if exists idx_approach_leg_fix_regi;
drop index if exists idx_approach_leg_fix_fix_airport_ident;
drop index if exists idx_approach_leg_recommended_fix_type;
drop index if exists idx_approach_leg_recommended_fix_ident;
drop index if exists idx_approach_leg_recommended_fix_regi;

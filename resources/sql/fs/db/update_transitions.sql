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

create index if not exists idx_transition_fix_type on transition(fix_type);
create index if not exists idx_transition_fix_ident on transition(fix_ident);
create index if not exists idx_transition_fix_region on transition(fix_region);
create index if not exists idx_transition_fix_fix_airport_ident on transition(fix_airport_ident);

create index if not exists idx_transition_dme_ident on transition(dme_ident);
create index if not exists idx_transition_dme_region on transition(dme_region);
create index if not exists idx_transition_dme_airport_ident on transition(dme_airport_ident);

create index if not exists idx_transition_leg_fix_type on transition_leg(fix_type);
create index if not exists idx_transition_leg_fix_ident on transition_leg(fix_ident);
create index if not exists idx_transition_leg_fix_region on transition_leg(fix_region);
create index if not exists idx_transition_leg_fix_fix_airport_ident on transition_leg(fix_airport_ident);

create index if not exists idx_transition_leg_recommended_fix_type on transition_leg(recommended_fix_type);
create index if not exists idx_transition_leg_recommended_fix_ident on transition_leg(recommended_fix_ident);
create index if not exists idx_transition_leg_recommended_fix_region on transition_leg(recommended_fix_region);

----------------------------------------------------------------
-- Update navigation references for transition -------------------

update transition set fix_nav_id =
(
  select v.vor_id
  from vor v
  where transition.fix_type = 'V' and transition.fix_ident = v.ident and transition.fix_region = v.region
) where transition.fix_type = 'V' and transition.fix_nav_id is null;

update transition set fix_nav_id =
(
  select n.ndb_id
  from ndb n
  where transition.fix_type = 'N' and transition.fix_ident = n.ident and transition.fix_region = n.region
) where transition.fix_type = 'N' and transition.fix_nav_id is null;

update transition set fix_nav_id =
(
  select v.vor_id
  from vor v
  where transition.fix_type = 'V' and substr(transition.fix_ident, 1, 3) = v.ident and transition.fix_region = v.region
) where transition.fix_type = 'V' and transition.fix_nav_id is null;

update transition set fix_nav_id =
(
  select n.ndb_id
  from ndb n
  where transition.fix_type = 'N' and substr(transition.fix_ident, 1, 3) = n.ident and transition.fix_region = n.region
) where transition.fix_type = 'N' and transition.fix_nav_id is null;


update transition set fix_nav_id =
(
  select w.waypoint_id
  from waypoint w
  where transition.fix_type = 'W' and transition.fix_ident = w.ident and transition.fix_region = w.region
) where transition.fix_type = 'W' and transition.fix_nav_id is null;

-- Terminals -----------------------------------

update transition set fix_nav_id =
(
  select n.ndb_id
  from ndb n join airport a on n.airport_id = a.airport_id
  where transition.fix_type = 'TN' and transition.fix_ident = n.ident and
  transition.fix_region = n.region and a.ident = transition.fix_airport_ident
) where transition.fix_type = 'TN' and transition.fix_nav_id is null;

update transition set fix_nav_id =
(
  select w.waypoint_id
  from waypoint w join airport a on w.airport_id = a.airport_id
  where transition.fix_type = 'TW' and transition.fix_ident = w.ident and
  transition.fix_region = w.region and a.ident = transition.fix_airport_ident
) where transition.fix_type = 'TW' and transition.fix_nav_id is null;

-- DME -----------------------------------

update transition set dme_nav_id =
(
  select v.vor_id from vor v
  where transition.dme_ident = v.ident and transition.dme_region = v.region
) where transition.dme_ident is not null and transition.dme_nav_id is null;

----------------------------------------------------------------
-- Update navigation references for transition legs ------------

update transition_leg set fix_nav_id =
(
  select v.vor_id
  from vor v
  where transition_leg.fix_type = 'V' and transition_leg.fix_ident = v.ident and transition_leg.fix_region = v.region
) where transition_leg.fix_type = 'V' and transition_leg.fix_nav_id is null;

update transition_leg set fix_nav_id =
(
  select n.ndb_id
  from ndb n
  where transition_leg.fix_type = 'N' and transition_leg.fix_ident = n.ident and transition_leg.fix_region = n.region
) where transition_leg.fix_type = 'N' and transition_leg.fix_nav_id is null;

update transition_leg set fix_nav_id =
(
  select w.waypoint_id
  from waypoint w
  where transition_leg.fix_type = 'W' and transition_leg.fix_ident = w.ident and transition_leg.fix_region = w.region
) where transition_leg.fix_type = 'W' and transition_leg.fix_nav_id is null;

-- Terminals -----------------------------------

update transition_leg set fix_nav_id =
(
  select n.ndb_id
  from ndb n join airport a on n.airport_id = a.airport_id
  where transition_leg.fix_type = 'TN' and transition_leg.fix_ident = n.ident and
  transition_leg.fix_region = n.region and a.ident = transition_leg.fix_airport_ident
) where transition_leg.fix_type = 'TN' and transition_leg.fix_nav_id is null;

update transition_leg set fix_nav_id =
(
  select w.waypoint_id
  from waypoint w join airport a on w.airport_id = a.airport_id
  where transition_leg.fix_type = 'TW' and transition_leg.fix_ident = w.ident and
  transition_leg.fix_region = w.region and a.ident = transition_leg.fix_airport_ident
) where transition_leg.fix_type = 'TW' and transition_leg.fix_nav_id is null;

----------------------------------------------------------------
-- Update navigation references for transition legs ------------

update transition_leg set recommended_fix_nav_id =
(
  select v.vor_id
  from vor v
  where transition_leg.recommended_fix_type = 'V' and transition_leg.recommended_fix_ident = v.ident and
  transition_leg.recommended_fix_region = v.region
) where transition_leg.recommended_fix_type = 'V' and transition_leg.recommended_fix_nav_id is null;

update transition_leg set recommended_fix_nav_id =
(
  select n.ndb_id
  from ndb n
  where transition_leg.recommended_fix_type = 'N' and transition_leg.recommended_fix_ident = n.ident and
  transition_leg.recommended_fix_region = n.region
) where transition_leg.recommended_fix_type = 'N' and transition_leg.recommended_fix_nav_id is null;

update transition_leg set recommended_fix_nav_id =
(
  select w.waypoint_id
  from waypoint w
  where transition_leg.recommended_fix_type = 'W' and transition_leg.recommended_fix_ident = w.ident and
  transition_leg.recommended_fix_region = w.region
) where transition_leg.recommended_fix_type = 'W' and transition_leg.recommended_fix_nav_id is null;

-- Terminals -----------------------------------

update transition_leg set recommended_fix_nav_id =
(
  select n.ndb_id
  from ndb n join airport a on n.airport_id = a.airport_id
  where transition_leg.recommended_fix_type = 'TN' and transition_leg.recommended_fix_ident = n.ident and
  transition_leg.recommended_fix_region = n.region and a.ident = transition_leg.fix_airport_ident
) where transition_leg.recommended_fix_type = 'TN' and transition_leg.recommended_fix_nav_id is null;

update transition_leg set recommended_fix_nav_id =
(
  select w.waypoint_id
  from waypoint w join airport a on w.airport_id = a.airport_id
  where transition_leg.recommended_fix_type = 'TW' and transition_leg.recommended_fix_ident = w.ident and
  transition_leg.recommended_fix_region = w.region and a.ident = transition_leg.fix_airport_ident
) where transition_leg.recommended_fix_type = 'TW' and transition_leg.recommended_fix_nav_id is null;


drop index if exists idx_transition_fix_type;
drop index if exists idx_transition_fix_ident;
drop index if exists idx_transition_fix_region;
drop index if exists idx_transition_fix_fix_airport_ident;
drop index if exists idx_transition_dme_ident;
drop index if exists idx_transition_dme_region;
drop index if exists idx_transition_dme_airport_ident;
drop index if exists idx_transition_leg_fix_type;
drop index if exists idx_transition_leg_fix_ident;
drop index if exists idx_transition_leg_fix_region;
drop index if exists idx_transition_leg_fix_fix_airport_ident;
drop index if exists idx_transition_leg_recommended_fix_type;
drop index if exists idx_transition_leg_recommended_fix_ident;
drop index if exists idx_transition_leg_recommended_fix_region;

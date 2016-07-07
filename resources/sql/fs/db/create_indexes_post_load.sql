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
-- This script create all indexes after loading the data.
-- The indexes are needed for post process steps
-- *************************************************************

create index if not exists idx_waypoint_type on waypoint(type);
create index if not exists idx_waypoint_region on waypoint(region);
create index if not exists idx_waypoint_ident on waypoint(ident);
create index if not exists idx_waypoint_num_vairway on waypoint(num_victor_airway);
create index if not exists idx_waypoint_num_rairway on waypoint(num_jet_airway);

create index if not exists idx_vor_type on vor(type);
create index if not exists idx_vor_region on vor(region);

create index if not exists idx_ndb_type on ndb(type);
create index if not exists idx_ndb_region on ndb(region);

create index if not exists idx_approach_fix_type on approach(fix_type);
create index if not exists idx_approach_fix_ident on approach(fix_ident);
create index if not exists idx_approach_fix_region on approach(fix_region);
create index if not exists idx_approach_fix_fix_airport_ident on approach(fix_airport_ident);

create index if not exists idx_approach_leg_fix_type on approach_leg(fix_type);
create index if not exists idx_approach_leg_fix_ident on approach_leg(fix_ident);
create index if not exists idx_approach_leg_fix_region on approach_leg(fix_region);
create index if not exists idx_approach_leg_fix_fix_airport_ident on approach_leg(fix_airport_ident);

create index if not exists idx_approach_leg_recommended_fix_type on approach_leg(recommended_fix_type);
create index if not exists idx_approach_leg_recommended_fix_ident on approach_leg(recommended_fix_ident);
create index if not exists idx_approach_leg_recommended_fix_region on approach_leg(recommended_fix_region);

create index if not exists idx_transition_fix_type on transition(fix_type);
create index if not exists idx_transition_fix_ident on transition(fix_ident);
create index if not exists idx_transition_fix_region on transition(fix_region);
create index if not exists idx_transition_fix_fix_airport_ident on transition(fix_airport_ident);

create index if not exists idx_transition_leg_fix_type on transition_leg(fix_type);
create index if not exists idx_transition_leg_fix_ident on transition_leg(fix_ident);
create index if not exists idx_transition_leg_fix_region on transition_leg(fix_region);
create index if not exists idx_transition_leg_fix_fix_airport_ident on transition_leg(fix_airport_ident);

create index if not exists idx_transition_leg_recommended_fix_type on transition_leg(recommended_fix_type);
create index if not exists idx_transition_leg_recommended_fix_ident on transition_leg(recommended_fix_ident);
create index if not exists idx_transition_leg_recommended_fix_region on transition_leg(recommended_fix_region);
